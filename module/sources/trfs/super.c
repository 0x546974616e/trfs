#include <linux/fs.h>

#include "trfs/file.h"
#include "trfs/printk.h"
#include "trfs/super.h"

// The Superblock Object:
// A superblock object represents a mounted filesystem.
// https://www.kernel.org/doc/Documentation/filesystems/vfs.txt

// static const struct super_operations trfs_super_operations = {
//   .statfs = simple_statfs, // Provided by the kernel
//   .drop_inode = generic_delete_inode, // Provided by the kernel
// };

int trfs_fill_super_block(
  struct super_block* const super_block,
  void* const data, // Key-value ASCII options?
  int const silent
) {
  struct inode* const root_inode = new_inode(super_block);
  if (!root_inode) {
    return -ENOMEM;
  }

  // TODO: What is the size of data block? Do mount_bdev set it correctly?

  // S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO
  // https://www.kernel.org/doc/Documentation/filesystems/idmappings.rst
  // What about nop_mnt_idmap (identity mapping)?
  inode_init_owner(super_block->s_user_ns, root_inode, NULL, S_IFDIR | 0755);

  root_inode->i_ino = 1;
  // root_inode->i_uid = root_inode->i_gid = 0;
  root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

  root_inode->i_op = &trfs_inode_operations;
  root_inode->i_fop = &trfs_directory_operations;

  // dcache = dentry cache = directory entry cache
  super_block->s_root = d_make_root(root_inode);
  if (!super_block->s_root) {
    iput(root_inode);
    return -ENOMEM;
  }

  return 0;
}

void trfs_kill_super_block(
  struct super_block* const super_block
) {
  // kill_block_super() is an helper function provided by the VFS which
  // unmounts a file system on a block device. This function frees some
  // internal resources.
  kill_block_super(super_block);

  // So far this function is only here for logging.
  TRFS_INFO("Superblock is destroyed\n");
  TRFS_INFO("Unmount succesful\n");
}
