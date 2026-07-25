typedef unsigned long size_t;
typedef long ssize_t;
typedef long off_t;
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef signed char int8_t;
#define NULL ((void *)0)

ssize_t sys_write(int fd, const void *buf, size_t count);
ssize_t sys_read(int fd, void *buf, size_t count);
int sys_open(const char *pathname, int flags, int mode);
int sys_close(int fd);
int sys_mkdir(const char *pathname, int mode);
int sys_reboot(int magic1, int magic2, int cmd, void *arg);
long sys_kexec_file_load(int kernel_fd, int initrd_fd, unsigned long cmdline_len, const char *cmdline, unsigned long flags);
void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int sys_munmap(void *addr, size_t len);
int sys_fstat(int fd, void *statbuf);
int sys_fsync(int fd);

#define O_RDONLY    0
#define O_WRONLY    1
#define O_CREAT     0100
#define O_TRUNC     01000

#define PROT_READ   1
#define PROT_WRITE  2

#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED    ((void *)-1)

#define LINUX_REBOOT_MAGIC1         0xfee1dead
#define LINUX_REBOOT_MAGIC2         0x28121969
#define LINUX_REBOOT_CMD_KEXEC      0x45584543

#define KEXEC_ARCH_X86_64   (62 << 16)

#define CPIO_HEADER_SIZE 110

#define KEXEC_FILE_NO_INITRAMFS 0x00000004

struct stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_nlink;
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t __pad0;
    uint64_t st_rdev;
    int64_t  st_size;
    int64_t  st_blksize;
    int64_t  st_blocks;
    uint64_t st_atime;
    uint64_t st_atime_nsec;
    uint64_t st_mtime;
    uint64_t st_mtime_nsec;
    uint64_t st_ctime;
    uint64_t st_ctime_nsec;
    int64_t  __unused[3];
};

struct dos_header {
    uint16_t e_magic;
    uint8_t  pad[58];
    uint32_t e_lfanew;
};

struct pe_header {
    uint32_t signature;
    uint16_t machine;
    uint16_t number_of_sections;
    uint32_t timestamp;
    uint32_t symbol_table_ptr;
    uint32_t symbol_count;
    uint16_t optional_header_size;
    uint16_t characteristics;
};

struct pe_section {
    char     name[8];
    uint32_t virtual_size;
    uint32_t virtual_addr;
    uint32_t raw_size;
    uint32_t raw_ptr;
    uint32_t reloc_ptr;
    uint32_t linenum_ptr;
    uint16_t reloc_count;
    uint16_t linenum_count;
    uint32_t characteristics;
};

struct uki_sections {
    void   *linux_data;
    size_t  linux_size;
    void   *initrd_data;
    size_t  initrd_size;
    void   *cmdline_data;
    size_t  cmdline_size;
};

// https://github.com/valinet/libvalinet/blob/master/valinet/utility/memmem.h
const char* memmem(const char* text, size_t haystacklen, const char* pattern, size_t needlelen) {
    const char* rv = NULL;

    size_t out[needlelen];
    for (int i = 0; i < needlelen; ++i)
        out[i] = 0;
    size_t j, i;

    j = 0, i = 1;
    while (i < needlelen) {
        if (text[j] != text[i]) {
            if (j > 0) {
                j = out[j - 1];
                continue;
            }
            else j--;
        }
        j++;
        out[i] = j;
        i++;
    }

    i = 0, j = 0;
    for (i = 0; i <= haystacklen; i++) {
        if (text[i] == pattern[j]) {
            j++;
            if (j == needlelen) {
                rv = text + (size_t)(i - needlelen + 1); //match++; j = out[j - 1];
                break;
            }
        } else {
            if (j != 0) {
                j = out[j - 1];
                i--;
            }
        }
    }

    return rv;
}

static size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

static void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = s1, *b = s2;
    while (n--) {
        if (*a != *b) return *a - *b;
        a++; b++;
    }
    return 0;
}

static char *strcat(char *dest, const char *src) {
    char *d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

static void print(const char *s) {
    sys_write(1, s, strlen(s));
    sys_fsync(1);
}

static void print_num(long n) {
    char buf[24];
    char out[24];
    int i = 0;
    int j = 0;
    int neg = 0;
    
    if (n < 0) {
        neg = 1;
        n = -n;
    }
    if (n == 0) {
        buf[i++] = '0';
    } else {
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
    }
    if (neg) out[j++] = '-';
    while (i > 0) out[j++] = buf[--i];
    out[j] = 0;
    print(out);
}

static void print_hex(unsigned long n) {
    const char *hex = "0123456789abcdef";
    char buf[19];
    int i;
    
    buf[0] = '0';
    buf[1] = 'x';
    for (i = 0; i < 16; i++) {
        buf[2 + i] = hex[(n >> (60 - i * 4)) & 0xf];
    }
    buf[18] = 0;
    print(buf);
}

static void println(const char *s) {
    print(s);
    print("\n");
}

static void print_ok(const char *msg) {
    print("[  OK  ] ");
    println(msg);
}

static void print_fail(const char *msg, long err) {
    print("[FAILED] ");
    print(msg);
    print(" (err=");
    print_num(err);
    println(")");
}

static void print_info(const char *msg) {
    print("[ INFO ] ");
    println(msg);
}

static void *read_file(const char *path, size_t *size_out) {
    int fd;
    struct stat st;
    void *buf;
    ssize_t nread;
    
    print("[ INFO ] Reading file: ");
    println(path);
    
    fd = sys_open(path, O_RDONLY, 0);
    if (fd < 0) {
        print_fail("open file", fd);
        return NULL;
    }
    
    if (sys_fstat(fd, &st) < 0) {
        print_fail("fstat", -1);
        sys_close(fd);
        return NULL;
    }
    
    print("[ INFO ] File size: ");
    print_num(st.st_size);
    println(" bytes");
    
    buf = sys_mmap(NULL, st.st_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
        print_fail("mmap", -1);
        sys_close(fd);
        return NULL;
    }
    
    nread = sys_read(fd, buf, st.st_size);
    if (nread != st.st_size) {
        print_fail("read", nread);
        sys_munmap(buf, st.st_size);
        sys_close(fd);
        return NULL;
    }
    
    sys_close(fd);
    *size_out = st.st_size;
    
    print_ok("File read successfully");
    return buf;
}

static int parse_uki(void *data, size_t size, struct uki_sections *out) {
    struct dos_header *dos = data;
    struct pe_header *pe;
    struct pe_section *sections;
    int i, num_sections;
    
    memset(out, 0, sizeof(*out));
    
    /* Check DOS header */
    if (dos->e_magic != 0x5A4D) {  /* "MZ" */
        print_fail("Not a valid PE file (bad MZ)", dos->e_magic);
        return -1;
    }
    
    print("[ INFO ] PE header offset: 0x");
    print_hex(dos->e_lfanew);
    println("");
    
    /* Find PE header */
    pe = (struct pe_header *)((char *)data + dos->e_lfanew);
    
    if (pe->signature != 0x00004550) {  /* "PE\0\0" */
        print_fail("Bad PE signature", pe->signature);
        return -1;
    }
    print_ok("Valid PE signature");
    
    num_sections = pe->number_of_sections;
    print("[ INFO ] Number of sections: ");
    print_num(num_sections);
    println("");
    
    /* Sections follow optional header */
    sections = (struct pe_section *)((char *)pe + 24 + pe->optional_header_size);
    
    /* Find .linux, .initrd, .cmdline sections */
    for (i = 0; i < num_sections; i++) {
        print("[ INFO ] Section: ");
        /* Print section name (max 8 chars) */
        char secname[9];
        memcpy(secname, sections[i].name, 8);
        secname[8] = 0;
        print(secname);
        print(" raw_ptr=0x");
        print_hex(sections[i].raw_ptr);
        print(" raw_size=0x");
        print_hex(sections[i].raw_size);
        println("");
        
        if (memcmp(sections[i].name, ".linux", 6) == 0) {
            out->linux_data = (char *)data + sections[i].raw_ptr;
            out->linux_size = sections[i].raw_size;
            print_ok("Found .linux section");
        }
        else if (memcmp(sections[i].name, ".initrd", 7) == 0) {
            out->initrd_data = (char *)data + sections[i].raw_ptr;
            out->initrd_size = sections[i].raw_size;
            print_ok("Found .initrd section");
        }
        else if (memcmp(sections[i].name, ".cmdline", 8) == 0) {
            out->cmdline_data = (char *)data + sections[i].raw_ptr;
            out->cmdline_size = sections[i].raw_size;
            print_ok("Found .cmdline section");
        }
    }
    
    if (!out->linux_data) {
        print_fail("Missing .linux section", 0);
        return -1;
    }
    if (!out->initrd_data) {
        print_fail("Missing .initrd section", 0);
        return -1;
    }
    
    return 0;
}

static size_t pad4(size_t n) {
    return (4 - (n & 3)) & 3;
}

#define CPIO_MODE_DIR   0x000041ED  /* 040755  */
#define CPIO_MODE_FILE  0x000081ED  /* 0100755 */
#define CPIO_MODE_LINK  0x0000A1FF  /* 0120777 */

static void put_hex8(char *buf, uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    int i;
    for (i = 7; i >= 0; i--)
        buf[7 - i] = hex[(val >> (i * 4)) & 0xf];
}

static void write_cpio_header_mode(char *buf, const char *name,
                                   size_t filesize, uint32_t mode) {
    size_t namelen = strlen(name) + 1;  /* includes NUL */
    memcpy(buf, "070701", 6);
    put_hex8(buf + 6,   1);          /* ino       */
    put_hex8(buf + 14,  mode);       /* mode      */
    put_hex8(buf + 22,  0);          /* uid       */
    put_hex8(buf + 30,  0);          /* gid       */
    put_hex8(buf + 38,  1);          /* nlink     */
    put_hex8(buf + 46,  0);          /* mtime     */
    put_hex8(buf + 54,  filesize);   /* filesize  */
    put_hex8(buf + 62,  0);          /* devmajor  */
    put_hex8(buf + 70,  0);          /* devminor  */
    put_hex8(buf + 78,  0);          /* rdevmajor */
    put_hex8(buf + 86,  0);          /* rdevminor */
    put_hex8(buf + 94,  namelen);    /* namesize  */
    put_hex8(buf + 102, 0);          /* check     */
}

/* Emit one newc entry (header + name + pad + body + pad). Returns bytes written.
   For symlinks pass the target as data with filesize = strlen(target) (no NUL). */
static size_t cpio_emit(char *p, const char *name, const void *data,
                        size_t filesize, uint32_t mode) {
    size_t pos = 0;
    size_t namelen = strlen(name) + 1;
    size_t padlen;

    write_cpio_header_mode(p + pos, name, filesize, mode);
    pos += CPIO_HEADER_SIZE;

    memcpy(p + pos, name, namelen);
    pos += namelen;
    padlen = pad4(CPIO_HEADER_SIZE + namelen);
    memset(p + pos, 0, padlen);
    pos += padlen;

    if (filesize) {
        memcpy(p + pos, data, filesize);
        pos += filesize;
        padlen = pad4(filesize);
        memset(p + pos, 0, padlen);
        pos += padlen;
    }
    return pos;
}

#define cpio_link(p, name, target) \
    cpio_emit((p), (name), (target), strlen(target), CPIO_MODE_LINK)

static size_t emit_host_file(char *p, const char *cpio_name, const char *host_path) {
    size_t sz, n;
    void *data = read_file(host_path, &sz);
    if (!data) {
        print_fail("read host file", 0);
        return 0;
    }
    n = cpio_emit(p, cpio_name, data, sz, CPIO_MODE_FILE);
    sys_munmap(data, sz);
    return n;
}

static size_t build_loop_setup_cpio(void *buf) {
    char *p = buf;
    size_t pos = 0;

    pos += cpio_emit(p + pos, "lnc",     NULL, 0, CPIO_MODE_DIR);
    pos += emit_host_file(p + pos, "lnc/init", "/init");
    pos += emit_host_file(p + pos, "lnc/musl.so", "/lnc/musl.so");
    pos += emit_host_file(p + pos, "lnc/busybox", "/lnc/busybox");
    pos += emit_host_file(p + pos, "lnc/setdio", "/lnc/setdio");
    pos += cpio_emit(p + pos, "TRAILER!!!", NULL, 0, CPIO_MODE_FILE);

    return pos;
}

static int write_file(const char *path, void *data, size_t size) {
    int fd = sys_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return fd;
    
    ssize_t written = 0;
    while (written < (ssize_t)size) {
        ssize_t n = sys_write(fd, (char *)data + written, size - written);
        if (n <= 0) {
            sys_close(fd);
            return -1;
        }
        written += n;
    }
    sys_close(fd);
    return 0;
}

static int do_kexec(void *kernel, size_t kernel_size,
                    void *initrd, size_t initrd_size,
                    const char *cmdline) {
    int kernel_fd, initrd_fd;
    long ret;
    
    print_info("Preparing kexec...");
    
    print("[ INFO ] Kernel: ");
    print_num(kernel_size);
    println(" bytes");
    
    print("[ INFO ] Initrd: ");
    print_num(initrd_size);
    println(" bytes");
    
    print("[ INFO ] Cmdline: ");
    println(cmdline);
    
    /* Write kernel to temp file */
    print_info("Writing kernel to /tmp/kernel...");
    sys_mkdir("/tmp", 0755);
    ret = write_file("/tmp/kernel", kernel, kernel_size);
    if (ret < 0) {
        print_fail("write kernel", ret);
        return ret;
    }
    print_ok("Kernel written");
    
    /* Write initrd to temp file */
    print_info("Writing initrd to /tmp/initrd...");
    ret = write_file("/tmp/initrd", initrd, initrd_size);
    if (ret < 0) {
        print_fail("write initrd", ret);
        return ret;
    }
    print_ok("Initrd written");
    
    /* Open files for kexec_file_load */
    kernel_fd = sys_open("/tmp/kernel", O_RDONLY, 0);
    if (kernel_fd < 0) {
        print_fail("open kernel", kernel_fd);
        return kernel_fd;
    }
    
    initrd_fd = sys_open("/tmp/initrd", O_RDONLY, 0);
    if (initrd_fd < 0) {
        print_fail("open initrd", initrd_fd);
        sys_close(kernel_fd);
        return initrd_fd;
    }
    
    print_info("Calling kexec_file_load...");
    ret = sys_kexec_file_load(kernel_fd, initrd_fd, strlen(cmdline) + 1, cmdline, 0);
    
    sys_close(kernel_fd);
    sys_close(initrd_fd);
    
    if (ret < 0) {
        print_fail("kexec_file_load", ret);
        return ret;
    }
    print_ok("kexec_file_load succeeded");
    
    print_info("Executing kexec reboot...");
    ret = sys_reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                     LINUX_REBOOT_CMD_KEXEC, NULL);
    
    /* Should not reach here */
    print_fail("reboot failed", ret);
    return ret;
}

int main(int argc, char** argv) {
    int ret;
    void *uki_data;
    size_t uki_size;
    struct uki_sections uki;
    void *combined_initrd;
    size_t combined_size;
    size_t cpio_size;
    char cmdline[256];
    char *grub_kernel, *grub_initrd, *grub_cmdline;

    if (argc != 2) {
        sys_write(2, "Usage: bootstrap /boot/EFI/Linux/arch-linux.efi\n", 49);
        return -1;
    }

    ret = strlen(argv[1]);
    if (argv[1][ret - 4] == '.' && 
        argv[1][ret - 3] == 'c' && 
        argv[1][ret - 2] == 'f' &&
        argv[1][ret - 1] == 'g') {
        uki_data = read_file(argv[1], &uki_size);
        if (!uki_data) {
            print_fail("Failed to read UKI", 0);
            goto fail;
        }
        grub_kernel = (char*)memmem(uki_data, uki_size, "/boot/vmlinuz", 13);
        grub_kernel -= 9;
        grub_kernel[0] = '/';
        grub_kernel[1] = 'l';
        grub_kernel[2] = 'n';
        grub_kernel[3] = 'c';
        grub_kernel[4] = '_';
        grub_kernel[5] = 'r';
        grub_kernel[6] = 'o';
        grub_kernel[7] = 'f';
        grub_kernel[8] = 's';
        grub_initrd = (char*)memmem(uki_data, uki_size, "/boot/initrd", 12);
        grub_initrd -= 9;
        grub_initrd[0] = '/';
        grub_initrd[1] = 'l';
        grub_initrd[2] = 'n';
        grub_initrd[3] = 'c';
        grub_initrd[4] = '_';
        grub_initrd[5] = 'r';
        grub_initrd[6] = 'o';
        grub_initrd[7] = 'f';
        grub_initrd[8] = 's';
        grub_cmdline = (char*)memmem(grub_kernel, uki_size - (grub_kernel - (char*)uki_data), " ", 1);
        grub_cmdline[0] = 0;
        grub_cmdline++;
        ret = 0;
        while (grub_initrd[ret] != '\n') ret++;
        grub_initrd[ret] = 0;
        ret = 0;
        while (grub_cmdline[ret] != '\n') ret++;
        grub_cmdline[ret] = 0;
        print("[ INFO ] grub_kernel: ");
        print(grub_kernel);
        print("\n");
        print("[ INFO ] grub_initrd: ");
        print(grub_initrd);
        print("\n");
        print("[ INFO ] grub_cmdline: ");
        print(grub_cmdline);
        print("\n");
        uki.cmdline_data = grub_cmdline;
        uki.cmdline_size = strlen(grub_cmdline);
        uki.linux_data = read_file(grub_kernel, &uki.linux_size);
        uki.initrd_data = read_file(grub_initrd, &uki.initrd_size);
    } else {
        uki_data = read_file(argv[1], &uki_size);
        if (!uki_data) {
            print_fail("Failed to read UKI", 0);
            goto fail;
        }
        ret = parse_uki(uki_data, uki_size, &uki);
        if (ret < 0) {
            print_fail("Failed to parse UKI", ret);
            goto fail;
        }
    }
    memcpy(cmdline, uki.cmdline_data, strlen(uki.cmdline_data));

    /* Build combined initrd: our cpio + original initrd */
    print_info("Building combined initrd...");
    
struct stat st1, st2;
int f;
f = sys_open("/bin/busybox", O_RDONLY, 0);              sys_fstat(f, &st1); sys_close(f);
f = sys_open("/lib/ld-musl-x86_64.so.1", O_RDONLY, 0);  sys_fstat(f, &st2); sys_close(f);
combined_size = st1.st_size + st2.st_size + 65536 + uki.initrd_size;
//    combined_size = 8192 + uki.initrd_size;  /* 8K for our cpio */
    combined_initrd = sys_mmap(NULL, combined_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (combined_initrd == MAP_FAILED) {
        print_fail("mmap for combined initrd", -1);
        goto fail;
    }
    
    /* Build loop setup cpio */
    cpio_size = build_loop_setup_cpio(combined_initrd);
    print("[ INFO ] CPIO size: ");
    print_num(cpio_size);
    println(" bytes");
    
    /* Append original initrd */
    memcpy((char *)combined_initrd + cpio_size, uki.initrd_data, uki.initrd_size);
    combined_size = cpio_size + uki.initrd_size;
    
    print_ok("Combined initrd built");
    print("[ INFO ] Total initrd size: ");
    print_num(combined_size);
    println(" bytes");
    
    /* Build cmdline - use our wrapper init, root is loop0p2 */
    strcat(cmdline, " rdinit=/lnc/init drm.panic_screen=kmsg"); // console=ttyS0 
    
    print("[ INFO ] Using cmdline: ");
    println(cmdline);

    /* Perform kexec */
    ret = do_kexec(uki.linux_data, uki.linux_size, combined_initrd, combined_size, cmdline);
    
    /* Should not reach here */
    print_fail("kexec failed", ret);
    
fail:
    println("");
    println("Boot failed! Halting...");
    
    return 1;
}
