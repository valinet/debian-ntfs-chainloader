typedef unsigned char       u8;
typedef unsigned int        u32;
typedef int                 s32;
typedef unsigned long long  u64;
typedef long long           s64;
typedef unsigned long       ulong;

/* ---- syscalls (syscall.S) ---- */
long sys_open(const char *path, long flags, long mode);
long sys_close(long fd);
long sys_read(long fd, void *buf, ulong n);
long sys_write(long fd, const void *buf, ulong n);
long sys_lseek(long fd, long off, long whence);
long sys_fstat(long fd, void *st);
long sys_ioctl(long fd, ulong req, void *arg);
long sys_umount2(const char *target, long flags);
long sys_mkdir(const char *path, long mode);
long sys_mknod(const char *path, long mode, u64 dev);
long sys_mmap(void *addr, ulong len, long prot, long flags, long fd, long off);
void sys_exit(long code);

#define O_RDONLY   0
#define O_RDWR     2
#define S_IFBLK    0060000
#define SEEK_SET   0
#define PROT_RW    3
#define MAP_ANON_P 0x22

/* ---- kernel structs ---- */
struct kstat {
	u64 st_dev, st_ino, st_nlink;
	u32 st_mode, st_uid, st_gid, __pad0;
	u64 st_rdev;
	s64 st_size, st_blksize, st_blocks;
	u64 t[6];
	s64 __unused[3];
};

struct fiemap_extent {
	u64 fe_logical, fe_physical, fe_length;
	u64 fe_reserved64[2];
	u32 fe_flags, fe_reserved[3];
};

struct fiemap {
	u64 fm_start, fm_length;
	u32 fm_flags, fm_mapped_extents, fm_extent_count, fm_reserved;
	struct fiemap_extent fm_extents[];
};

#define DM_NAME_LEN        128
#define DM_UUID_LEN        129
#define DM_MAX_TYPE_NAME   16

struct dm_ioctl {
	u32 version[3];
	u32 data_size, data_start, target_count;
	s32 open_count;
	u32 flags, event_nr, padding;
	u64 dev;
	char name[DM_NAME_LEN];
	char uuid[DM_UUID_LEN];
	char data[7];
};

struct dm_target_spec {
	u64 sector_start, length;
	s32 status;
	u32 next;
	char target_type[DM_MAX_TYPE_NAME];
};

/* _IOWR(type, nr, size) = (3<<30)|(size<<16)|(type<<8)|nr */
#define IOWR(t, nr, sz)  (0xC0000000u | ((sz) << 16) | ((t) << 8) | (nr))
#define FS_IOC_FIEMAP    IOWR('f', 11, 32)          /* sizeof(struct fiemap) */
#define DM_DEV_CREATE    IOWR(0xfd, 3,  312)        /* sizeof(struct dm_ioctl) */
#define DM_DEV_SUSPEND   IOWR(0xfd, 6,  312)
#define DM_TABLE_LOAD    IOWR(0xfd, 9,  312)
#define DM_COOKIE_MAGIC              0x0D4D
#define DM_UDEV_FLAGS_SHIFT          16
#define DM_UDEV_PRIMARY_SOURCE_FLAG  0x0040

#define FIEMAP_FLAG_SYNC       0x0001
#define FIEMAP_MAX_OFFSET      (~0ULL)
#define FE_LAST                0x0001
#define FE_BAD                 (0x0002 | 0x0004 | 0x0008 | 0x0100 | \
                                0x0200 | 0x0400 | 0x0800)

#define MAXEXT  8192
#define PRMMAX  64
#define STEP    ((sizeof(struct dm_target_spec) + PRMMAX + 7) & ~7UL)

/* ---- freestanding helpers (gcc emits calls to these) ---- */
void *memset(void *d, int c, ulong n)
{
	u8 *p = d;
	while (n--) *p++ = (u8)c;
	return d;
}

void *memcpy(void *d, const void *s, ulong n)
{
	u8 *a = d;
	const u8 *b = s;
	while (n--) *a++ = *b++;
	return d;
}

static void scpy(char *d, const char *s, ulong max)
{
	ulong i = 0;
	while (s[i] && i < max - 1) { d[i] = s[i]; i++; }
	d[i] = 0;
}

static char *app(char *p, const char *s) { while (*s) *p++ = *s++; return p; }

static char *appu(char *p, u64 v)
{
	char t[24];
	int i = 0;
	if (!v) t[i++] = '0';
	while (v) { t[i++] = '0' + (char)(v % 10); v /= 10; }
	while (i) *p++ = t[--i];
	return p;
}

static void note(const char *m)
{
	char b[128], *p = b;

	p = app(p, "dmimage: ");
	p = app(p, m);
	p = app(p, "\n");
	sys_write(2, b, (ulong)(p - b));
}

static void fail(const char *m, long err)
{
	char b[128], *p = b;

	p = app(p, "dmimage: ");
	p = app(p, m);
	if (err < 0) { p = app(p, ": errno "); p = appu(p, (u64)(-err)); }
	p = app(p, "\n");
	sys_write(2, b, (ulong)(p - b));
	sys_exit(1);
}

#define bail(m) fail((m), 0)

static u64 S[16][MAXEXT], L[16][MAXEXT], P[16][MAXEXT];
int main(int argc, char **argv)
{
	const char *umnt = argv[1], *bdev = argv[2];
	int ns[16];

	for (int v = 2; v < argc - 1; v = v + 2) {
		const char *path = argv[1 + v];
		note(path);

		long r, fd = sys_open(path, O_RDONLY, 0);
		if (fd < 0) {
			ns[v] = -1;
			continue;
		}

		note(path);

		struct kstat st;
		r = sys_fstat(fd, &st);
		if (r < 0) fail("fstat", r);

		u64 size = (u64)st.st_size;
		if (!size || (size & 511)) bail("image size is not a multiple of 512");

		ulong fmsz = sizeof(struct fiemap) + MAXEXT * sizeof(struct fiemap_extent);
		struct fiemap *fm = (struct fiemap *)
			sys_mmap(0, fmsz, PROT_RW, MAP_ANON_P, -1, 0);
		if ((long)fm < 0) fail("mmap", (long)fm);

		fm->fm_start = 0;
		fm->fm_length = FIEMAP_MAX_OFFSET;
		fm->fm_flags = FIEMAP_FLAG_SYNC;
		fm->fm_extent_count = MAXEXT;
		r = sys_ioctl(fd, FS_IOC_FIEMAP, fm);
		if (r < 0) fail("FIEMAP", r);
		if (!fm->fm_mapped_extents) bail("no extents");
		if (!(fm->fm_extents[fm->fm_mapped_extents - 1].fe_flags & FE_LAST))
			bail("too many extents");

		u32 n = 0, i;
		u64 next = 0;
		for (i = 0; i < fm->fm_mapped_extents && next < size; i++) {
			struct fiemap_extent *e = &fm->fm_extents[i];
			u64 len;

			if (e->fe_flags & FE_BAD) bail("sparse, compressed or unwritten extent");
			if (e->fe_logical != next) bail("hole in file");
			if ((e->fe_logical | e->fe_physical | e->fe_length) & 511)
				bail("extent is not 512-aligned");

			len = e->fe_length;
			if (e->fe_logical + len > size) len = size - e->fe_logical;

			S[v][n] = e->fe_logical / 512;
			L[v][n] = len / 512;
			P[v][n] = e->fe_physical / 512;
			n++;
			next = e->fe_logical + len;
		}
		if (next != size) bail("extents do not cover the image");

		/* release the image before anything claims the backing device */
		sys_close(fd);

		ns[v] = n;
	}

	if (umnt) {
		int r = sys_umount2(umnt, 0);
		if (r < 0) fail("umount", r);
	}

	for (int v = 2; v < argc - 1; v = v + 2) {
		if (ns[v] == -1)
			continue;

		const char *path = argv[1 + v], *name = argv[2 + v];
		note(path);

		u32 cookie = (DM_COOKIE_MAGIC << DM_UDEV_FLAGS_SHIFT)
			| DM_UDEV_PRIMARY_SOURCE_FLAG;

		long cfd = sys_open("/dev/mapper/control", O_RDWR, 0);
		if (cfd < 0) fail("open /dev/mapper/control (is dm_mod loaded?)", cfd);

		ulong sz = sizeof(struct dm_ioctl) + (ulong)ns[v] * STEP;
		char *buf = (char *)sys_mmap(0, sz, PROT_RW, MAP_ANON_P, -1, 0);
		if ((long)buf < 0) fail("mmap", (long)buf);
		struct dm_ioctl *dmi = (struct dm_ioctl *)buf;

		memset(dmi, 0, sizeof *dmi);
		dmi->version[0] = 4;
		dmi->data_size = sizeof *dmi;
		dmi->data_start = sizeof *dmi;
		scpy(dmi->name, name, DM_NAME_LEN);
		int r = sys_ioctl(cfd, DM_DEV_CREATE, dmi);
		if (r < 0) fail("DM_DEV_CREATE", r);
		u64 dev = dmi->dev;      /* already in new_encode_dev form, as mknod wants */

		memset(dmi, 0, sizeof *dmi);
		dmi->version[0] = 4;
		dmi->data_size = (u32)sz;
		dmi->data_start = sizeof *dmi;
		dmi->target_count = ns[v];
		dmi->event_nr = cookie;
		scpy(dmi->name, name, DM_NAME_LEN);

		char *p = buf + sizeof *dmi;
		for (int i = 0; i < ns[v]; i++) {
			struct dm_target_spec *ts = (struct dm_target_spec *)p;
			char *q = p + sizeof *ts;

			ts->sector_start = S[v][i];
			ts->length = L[v][i];
			ts->status = 0;
			ts->next = (u32)STEP;
			scpy(ts->target_type, "linear", DM_MAX_TYPE_NAME);

			q = app(q, bdev);
			q = app(q, " ");
			q = appu(q, P[v][i]);
			*q = 0;
			p += STEP;
		}
		r = sys_ioctl(cfd, DM_TABLE_LOAD, dmi);
		if (r < 0) fail("DM_TABLE_LOAD", r);

		memset(dmi, 0, sizeof *dmi);
		dmi->version[0] = 4;
		dmi->data_size = sizeof *dmi;
		dmi->data_start = sizeof *dmi;
		scpy(dmi->name, name, DM_NAME_LEN);
		dmi->event_nr = cookie;
		r = sys_ioctl(cfd, DM_DEV_SUSPEND, dmi);
		if (r < 0) fail("resume", r);
		sys_close(cfd);

		char node[DM_NAME_LEN + 16], *np;
		sys_mkdir("/dev/mapper", 0755);
		np = app(node, "/dev/mapper/");
		np = app(np, name);
		*np = 0;
		r = sys_mknod(node, S_IFBLK | 0600, dev);
		if (r < 0) fail("mknod", r);

		{
			char msg[128], *m = app(msg, "dmimage: ");
			m = app(m, name);
			m = app(m, ": ");
			m = appu(m, ns[v]);
			m = app(m, " extents mapped\n");
			sys_write(2, msg, (ulong)(m - msg));
		}
	}
	return 0;
}
