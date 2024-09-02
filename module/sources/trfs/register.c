
// https://www.uninformativ.de/blog/postings/2017-09-09/0/POSTING-en.html
// https://kukuruku.co/post/writing-a-file-system-in-linux-kernel/

// https://github.com/krinkinmu/aufs.git
// https://github.com/psankar/simplefs.git
// https://github.com/sysprog21/simplefs.git

// https://codebrowser.dev/linux/linux/fs/minix/inode.c.html#minix_fill_super
// ? https://github.com/osxfuse/filesystems/blob/master/filesystems-c/unixfs/minixfs/minixfs.c
// ? https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/master/minix/fs/mfs/inode.c

// https://linux-kernel-labs.github.io/refs/heads/master/labs/filesystems_part1.html#buffer-cache
// https://www.science.smith.edu/~nhowe/262/oldlabs/module.html
// https://www.kernel.org/doc/Documentation/filesystems/vfs.txt
// https://www.kernel.org/doc/Documentation/filesystems/Locking

// https://lwn.net/Articles/13325/
// ? https://tldp.org/LDP/khg/HyperNews/get/fs/vfstour.html
// ? https://www.win.tue.nl/~aeb/linux/vfs/trail.html

// https://fossd.anu.edu.au/linux/v4.16-rc5/C/ident/dir_emit_dots
// https://fossd.anu.edu.au/linux/v4.16-rc5/source/include/linux/fs.h#L3378
// https://fossd.anu.edu.au/linux/v4.16-rc5/source/include/linux/fs.h#L1663

// https://elixir.bootlin.com/linux/v6.10.5/source/include/linux/fs.h#L2545

// /proc/mounts
// /proc/filesystems

// dd if=/dev/zero of=~/dada
// losetup --find --show dada
// mkdir fafa
// mount -t type /dev/loop0 fafa
// ls fafa
// umont /dev/lopp0
// losetup --detach /dev/loop0

#include <linux/fs.h>

#include "trfs/printk.h"
#include "trfs/register.h"
#include "trfs/super.h"

#define TRFS_NAME "trfs"

static struct dentry* trfs_mount(
  struct file_system_type* const file_system_type,
  int const flags,
  char const* const device_name,
  void* const data // ASCII key-value options?
) {
  // See mount_bdev() and mount_nodev().
  struct dentry* const root_entry = mount_bdev(
    file_system_type, flags, device_name, data, trfs_fill_super_block
  );

  // root_entry is either a valid pointer or an error code.
  if (unlikely(IS_ERR(root_entry))) {
    TRFS_ERROR("Error while mounting %s on %s\n", TRFS_NAME, device_name);
    return root_entry;
  }

  TRFS_INFO("%s is succesfully mounted on %s\n", TRFS_NAME, device_name);
  return root_entry;
}

struct file_system_type trfs_type = {
  .owner = THIS_MODULE, // NULL for built-in module?
  .name = TRFS_NAME,
  .mount = trfs_mount,
  .kill_sb = trfs_kill_super_block,
  .fs_flags = FS_REQUIRES_DEV,
};

int trfs_register(void) {
  int error = register_filesystem(&trfs_type);
  if (unlikely(error)) {
    TRFS_ERROR("Failed to register %s\n", TRFS_NAME);
    return error;
  }

  TRFS_INFO("Sucessfully registered %s\n", TRFS_NAME);
  return 0;
}

void trfs_unregister(void) {
  int error = unregister_filesystem(&trfs_type);
  if (unlikely(error)) {
    TRFS_ERROR("Failed to unregister %s (error: [%d])\n", TRFS_NAME, error);
  }

  TRFS_INFO("Sucessfully unregistered %s\n", TRFS_NAME);
}
