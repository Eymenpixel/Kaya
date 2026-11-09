# Nexus

Nexus is the planned CLI and GUI virtual-machine manager for Nexora. The starter CLI can inspect image names and launch an ISO or disk image with QEMU when the matching host tools are installed. Its GUI offers the same basic inspect/start flow.

ISO, IMG, raw, and ext4 files are not all virtual-machine formats: ext4 is a filesystem, and Nexus treats an ext4 image as a raw disk input. Support for VDI, VMDK, VHD/VHDX, and other formats depends on QEMU, VirtualBox, and future backend adapters. This package does not promise that every format or VM tool is supported yet.

Examples:

```sh
nexus backends
nexus inspect ./disk.img
nexus start ./installer.iso --ram 2048
nexus gui
```
