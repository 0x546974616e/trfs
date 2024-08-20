
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

#include <linux/fs.h>

#include "impl.h"
#include "printk.h"

#define TRFS_IMPL_NAME "trfs"

static struct dentry* trfs_impl_inode_lookup(
  struct inode* parent_inode,
  struct dentry* child_dentry,
  unsigned int flags
) {
  // inode->i_op
  // https://www.kernel.org/doc/Documentation/filesystems/vfs.txt
  pr_info("Inode lookup\n");
  return NULL;
}

static const struct inode_operations trfs_impl_inode_operations = {
  .lookup = trfs_impl_inode_lookup,
};

static int trfs_impl_directory_iterate(
  struct file* file,
  struct dir_context* context
) {
  pr_info("Directory iterate\n");
  // Emit the standard entries "." and "..".
  dir_emit_dots(file, context);
  return 0;
}

static const struct file_operations trfs_impl_directory_operations = {
  .owner = THIS_MODULE,
  .iterate = trfs_impl_directory_iterate,
};

static int trfs_impl_fill_super_block(
  struct super_block* const super_block,
  void* const data, // Key-value ASCII options?
  int const silent
) {
  struct inode* const root_inode = new_inode(super_block);
  if (!root_inode) {
    return -ENOMEM;
  }

  // S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO
  // https://www.kernel.org/doc/Documentation/filesystems/idmappings.rst
  // What about nop_mnt_idmap (identity mapping).
  inode_init_owner(super_block->s_user_ns, root_inode, NULL, S_IFDIR | 0755);

  root_inode->i_ino = 1;
  // root_inode->i_uid = root_inode->i_gid = 0;
  root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

  root_inode->i_op = &trfs_impl_inode_operations;
  root_inode->i_fop = &trfs_impl_directory_operations;

  // dcache = dentry cache = directory entry cache
  super_block->s_root = d_make_root(root_inode);
  if (!super_block->s_root) {
    iput(root_inode);
    return -ENOMEM;
  }

  return 0;
}

static struct dentry* trfs_impl_mount(
  struct file_system_type* const file_system_type,
  int const flags,
  char const* const device_name,
  void* const data // ASCII key-value options?
) {
  // See mount_bdev() and mount_nodev().
  struct dentry* const root_entry = mount_bdev(
    file_system_type, flags, device_name, data, trfs_impl_fill_super_block
  );

  // root_entry is either a valid pointer or an error code.
  if (unlikely(IS_ERR(root_entry))) {
    pr_err("Error while mounting %s on %s\n", TRFS_IMPL_NAME, device_name);
    return root_entry;
  }

  pr_info("%s is succesfully mounted on %s\n", TRFS_IMPL_NAME, device_name);
  return root_entry;
}

static void trfs_impl_kill_super_block(
  struct super_block* const super_block
) {
  // kill_block_super() is an helper function provided by the VFS which
  // unmounts a file system on a block device. This function frees some
  // internal resources.
  kill_block_super(super_block);

  // So far this function is only here for logging.
  pr_info("Superblock is destroyed\n");
  pr_info("Unmount succesful\n");
}

struct file_system_type trfs_impl_type = {
  .owner = THIS_MODULE,
  .name = TRFS_IMPL_NAME,
  .mount = trfs_impl_mount,
  .kill_sb = trfs_impl_kill_super_block,
  .fs_flags = FS_REQUIRES_DEV,
};

int __init trfs_impl_init(void) {
  pr_info("TRFS(impl) exit\n");

  int error = register_filesystem(&trfs_impl_type);
  if (unlikely(error)) {
    pr_err("Failed to register %s\n", TRFS_IMPL_NAME);
    return error;
  }

  pr_info("Sucessfully registered %s\n", TRFS_IMPL_NAME);
  return 0;
}

void __exit trfs_impl_exit(void) {
  pr_info("TRFS(impl) exit\n");

  int error = unregister_filesystem(&trfs_impl_type);
  if (unlikely(error)) {
    pr_err("Failed to unregister %s (error: [%d])\n", TRFS_IMPL_NAME, error);
  }

  pr_info("Sucessfully unregistered %s\n", TRFS_IMPL_NAME);
}
