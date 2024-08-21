#include <linux/fs.h>

#include "trfs/file.h"
#include "trfs/printk.h"

struct dentry* trfs_inode_lookup(
  struct inode* const parent_inode,
  struct dentry* const child_dentry,
  unsigned int flags
) {
  // inode->i_op
  // https://www.kernel.org/doc/Documentation/filesystems/vfs.txt
  pr_info("Inode lookup\n");
  return NULL;
}

struct inode_operations const trfs_inode_operations = {
  .lookup = trfs_inode_lookup,
};

static int trfs_directory_iterate(
  struct file* const file,
  struct dir_context* const context
) {
  pr_info("Directory iterate\n");
  // Emit the standard entries "." and "..".
  dir_emit_dots(file, context);
  return 0;
}

struct file_operations const trfs_directory_operations = {
  .owner = THIS_MODULE,
  .iterate = trfs_directory_iterate,
};