
# Setup

## 1. Install QEMU

### Architecture

Multiple ways to retrieve the current CPU architecture:

- `arch`
- `lscpu`
- `uname -m`
- `cat /proc/cpuinfo`
- `dpkg --print-architecture`
- `lsb_release`
- `hostnamectl`
- ...

### QEMU

On Debian:

```sh
apt-get install qemu-system-<ARCHITECTURE>
apt-get install qemu-system # To get all
```

## 2. Create disk image

```sh
qemu-img create debian.img 4G
qemu-img create -f qcow2 debian.qcow 4G
```

According to the [debian wiki][qemu_debian_wiki] more than 2G is needed when
installing a desktop environment (see [memory and disk space requirements][]
and [disk space needed for tasks][]).

[memory and disk space requirements]: https://www.debian.org/releases/bookworm/amd64/ch02s05.en.html
[disk space needed for tasks]: https://www.debian.org/releases/bookworm/amd64/apds02.en.html

## 3. Download boot image

Download image at https://www.debian.org/CD/netinst/.

## 4. Install Debian

```sh
qemu-system-x86_64 \
  -hda debian.qcow \
  -cdrom debian-12.6.0-amd64-netinst.iso \
  -boot d -m 512 -enable-kvm
```

When the debian boot screen appears, boot into "expert install" mode.

### NOTE

According to the [QEMU debian wiki][qemu_debian_wiki]:

> By default, QEMU invokes the `-nic` and `-user` options to add a single
> network adapter to the guest and provide NATed external Internet access.
> The host and guest will not see each other.

## 5. Boot the system

```sh
qemu-system-x86_64 -hda debian.qcow -m 512 -enable-kvm
```

### Note

QEMU `-boot` option:
- `c`: first hard disk
- `d`: first CD-ROM

## 6. OS Setup

### Networking

- `ip link show`
- `ip addr list`

1. `vi /etc/network/interfaces`

  ```
  auto ens3
  iface ens3 inet dhcp
  ```

2. `systemctl restart networking`

### APT sources list

Edit `/etc/apt/sources.list`:

```
deb http://deb.debian.org/debian bookworm main
deb-src http://deb.debian.org/debian bookworm main

deb http://deb.debian.org/debian-security/ bookworm-security main
deb-src http://deb.debian.org/debian-security/ bookworm-security main

deb http://deb.debian.org/debian bookworm-updates main
deb-src http://deb.debian.org/debian bookworm-updates main
```

- `apt-get update`
- `apt-get install vim tree`
- ``apt-cache search linux-headers-`uname -r` ``
- ``apt-get install linux-headers-`uname -r` ``

## 6. Shared directory

### Boot with virtfs

```sh
qemu-system-x86_64 \
  -hda debian.qcow -m size=512M \
  -enable-kvm -display default,show-cursor=on \
  -virtfs local,path=./shared,readonly=on,mount_tag=shared0,id=shared0,security_model=mapped
```

Only the `module/` directory is shared in order to prevent the guest OS from
accessing to the `.git/` and `image/` directories.

See [9p QEMU wiki][9p_qemu_wiki], superuser [question 1][virtfs_source_1]
and [question 2][virtfs_source_2].

[9p_qemu_wiki]: https://wiki.qemu.org/Documentation/9psetup
[virtfs_source_1]: https://superuser.com/questions/628169/how-to-share-a-directory-with-the-host-without-networking-in-qemu
[virtfs_source_2]: https://superuser.com/questions/1581788/qemu-kvm-creating-file-sharing-between-linux-host-and-macos-guest

### Guest `fstab`

```fstab
shared0 /media/shared0 9p ro,trans=virtio,version=9p2000.L 0 0
```

First create `/media/shared0` then `mount -a`.

## See

- <https://wiki.debian.org/QEMU>
- [Debootstrap](https://wiki.debian.org/Debootstrap)?

[qemu_debian_wiki]: https://wiki.debian.org/QEMU
