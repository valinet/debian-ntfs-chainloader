# debian-ntfs-chainloader

The purpose of this is to boot a the [Debian Cloud Image](https://cdimage.debian.org/cdimage/cloud/) on a physical machine using Ventoy.

This makes it super easy to install Debian, by basically starting from a barebones install that you customize to your needs. Keeping it on the NTFS partition means you can run it alongside Windows easily, without messing with your partitions layout.

Only things to customize are in the `init` script/file: the path where you will place the VHD to boot on the NTFS partition, and the ID of the NTFS partition on which to look for the VHD.

To proceed, first install dependencies:

```
make deps
```

Then:

```
make
```

That will boot a VM using qemu and test the generated files for you, by booting Debian in the same configuration you will do on the physical system.

In the end, what you will need is the EFI application which you will start from Ventoy, which will be placed in `uki/debian.efi`, and the coresponding VHD from `cache\debian.vhd`. Default login user and pass will be `root:root`.
