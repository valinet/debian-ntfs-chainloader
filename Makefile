SHELL := /bin/bash

all: qemu

.PHONY: qemu
qemu: cache/disk.raw esp/EFI/BOOT/BOOTx64.EFI
	@setsid -w -c qemu-system-x86_64 -m 1024 \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_VARS_4M.fd \
		-drive format=raw,file=fat:rw:esp \
		-drive if=none,id=hd0,format=raw,file=$< \
		-device ich9-ahci,id=ahci \
		-device ide-hd,drive=hd0,bus=ahci.0 \
		-smbios type=11,value=io.systemd.stub.kernel-cmdline-extra=console=ttyS0 \
		-nographic

cache/disk.raw: cache/debian.vhd init init.cpio
	set -eu; \
	getvar() { grep -m1 "^$$1=" init | cut -d= -f2- | tr -d "\"'"; }; \
	hex2bin() { printf '%b' "$$(sed 's/../\\x&/g' <<< "$$1")"; }; \
	rev8() { sed 's/\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)/\8\7\6\5\4\3\2\1/' <<< "$$1"; }; \
	TARGET_NTFSID=$$(getvar TARGET_NTFSID); \
	TARGET_PATH=$$(getvar TARGET_PATH); \
	[[ $$TARGET_NTFSID =~ ^[0-9a-fA-F]{16}$$ ]] || { echo "bad TARGET_NTFSID" >&2; exit 1; }; \
	rm -f $@; \
	fallocate -l 3.1G $@; \
	sgdisk -o -n 1:0:0 -t 1:0700 $@; \
	mkdir -p mnt; \
	LOOPDEV=$$(losetup --find --show --partscan --direct-io=on $@); \
	trap 'umount -R mnt 2>/dev/null || true; \
	      losetup -d "$$LOOPDEV" 2>/dev/null || true; \
	      rmdir mnt 2>/dev/null || true' EXIT; \
	udevadm settle; \
	mkfs.ntfs -Q -L data "$${LOOPDEV}p1"; \
	dd if="$${LOOPDEV}p1" bs=1 skip=72 count=8 2>/dev/null | hexdump -e '8/1 "%02x"' | sed 's/\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)/\8\7\6\5\4\3\2\1/'; echo; \
	hex2bin "$$(rev8 "$$TARGET_NTFSID")" | dd of="$${LOOPDEV}p1" bs=1 seek=72 conv=notrunc status=none; \
	dd if="$${LOOPDEV}p1" bs=1 skip=72 count=8 2>/dev/null | hexdump -e '8/1 "%02x"' | sed 's/\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)/\8\7\6\5\4\3\2\1/'; echo; \
	blockdev --flushbufs "$${LOOPDEV}p1"; \
	mount -t ntfs3 "$${LOOPDEV}p1" mnt; \
	DEST=mnt/$${TARGET_PATH#/}; \
	mkdir -p "$${DEST%/*}"; \
	cp $< "$$DEST"; \
	sync

esp/EFI/BOOT/BOOTx64.EFI: uki/debian.efi
	mkdir -p esp/EFI/BOOT
	cp uki/debian.efi $@

uki/debian.efi: uki init.cpio cache/debian.vhd
	mkdir -p mnt; \
	LOOPDEV=$$(losetup --find --show --partscan --direct-io=on cache/debian.vhd); \
	trap 'umount -R mnt 2>/dev/null || true; \
	      losetup -d "$$LOOPDEV" 2>/dev/null || true; \
	      rmdir mnt 2>/dev/null || true' EXIT; \
	udevadm settle; \
	mount -t ext4 -oro "$${LOOPDEV}p1" mnt; \
	mount -t vfat -oro "$${LOOPDEV}p15" mnt/boot/efi; \
	ukify build --linux=`ls -1t mnt/boot/vmlinuz* | head -1` --initrd=`ls -1t mnt/boot/initrd* | head -1` --initrd=init.cpio --output $@

uki:
	mkdir -p uki

init.cpio: fs/init fs/lnc/setdio fs/lnc/busybox fs/lnc/musl.so fs/lnc/bootstrap fs/lnc/dmimage
	mkdir -p fs/dev
	mkdir -p fs/proc
	mkdir -p fs/sys
	cd fs && find | cpio -H newc -o > ../$@

fs/lnc/bootstrap: bootstrap/bootstrap.c
	make -C bootstrap

fs/lnc/dmimage: dmimage/dmimage.c
	make -C dmimage

fs/init: init
	mkdir -p fs
	cp init $@

fs/lnc/setdio: setdio/setdio.S
	make -C setdio

fs/lnc/musl.so: cache/musl.tar.gz
	mkdir -p tmp fs/lnc; \
	trap 'rm -rf tmp' EXIT; \
	tar xf cache/musl.tar.gz -C tmp; \
	cp tmp/lib/ld-musl-x86_64.so.1 $@

fs/lnc/busybox: cache/busybox.tar.gz
	mkdir -p tmp fs/lnc; \
	trap 'rm -rf tmp' EXIT; \
	tar xf cache/busybox.tar.gz -C tmp; \
	cp tmp/bin/busybox $@; \
	python3 bswap $@ "/lib/ld-musl-x86_64.so.1" "/lnc/musl.so"; \
	python3 bswap $@ "libc.musl-x86_64.so.1" "/lnc/musl.so"

cache/debian.vhd: | cache
	wget http://host/cdimage/cloud/trixie/20260722-2547/debian-13-nocloud-amd64-20260722-2547.raw -O $@
	python3 bswap $@ "root:!unprovisioned:" "root:ab6TRGT20sY26:0"
	python3 scripts/usr/local/sbin/raw2vhd $@
	mkdir -p mnt; \
	LOOPDEV=$$(losetup --find --show --partscan --direct-io=on $@); \
	trap 'umount -R mnt 2>/dev/null || true; \
	      losetup -d "$$LOOPDEV" 2>/dev/null || true; \
	      rmdir mnt 2>/dev/null || true' EXIT; \
	udevadm settle; \
	mount -t ext4 "$${LOOPDEV}p1" mnt; \
	mount -t vfat "$${LOOPDEV}p15" mnt/boot/efi; \
	printf '%s\n' "`cat customize`" | arch-chroot mnt

cache/busybox.tar.gz: | cache
	wget https://dl-cdn.alpinelinux.org/alpine/latest-stable/main/x86_64/busybox-1.37.0-r31.apk -O $@

cache/musl.tar.gz: | cache
	wget https://dl-cdn.alpinelinux.org/alpine/latest-stable/main/x86_64/musl-1.2.6-r2.apk -O $@

cache:
	mkdir -p $@

.PHONY: deps
deps:
	apt install --no-install-recommends build-essential nasm qemu-system-x86 ovmf ukify arch-install-scripts wget

.PHONY: clean
clean:
	umount -R mnt || true
	make -C bootstrap clean
	make -C dmimage clean
	rm -rf fs init.cpio esp uki mnt cache

FORCE: ;
