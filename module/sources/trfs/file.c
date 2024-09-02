#include <linux/fs.h>

#include "trfs/file.h"
#include "trfs/printk.h"

// The File Object:
// A file object represents a file opened by a process. This is also known as an
// "open file description" in POSIX parlance.
// https://www.kernel.org/doc/Documentation/filesystems/vfs.txt

static struct dentry*
trfs_inode_lookup(
  struct inode* const parent_inode,
  struct dentry* const child_dentry,
  unsigned int flags
) {
  // inode->i_op
  // https://www.kernel.org/doc/Documentation/filesystems/vfs.txt
  TRFS_INFO("Inode lookup\n");
  return NULL;
}

struct inode_operations const trfs_inode_operations = {
  .lookup = trfs_inode_lookup,
};

/**
 * Called by the VFS when an inode should be opened. When the VFS opens a file,
 * it creates a new "struct file" and then calls the open() method for the newly
 * allocated file structure.
 *
 * The open() method is a good place to initialize the "private_data" member in
 * the file structure if you want to point to a device structure.
 *
 * @param inode
 * @param file
 * @return
 */
static int
trfs_file_open(
  struct inode* const inode,
  struct file* const file
) {
  // Log only for test purposes.
  TRFS_INFO("File open");
  return 0;
}

/**
 * Called by the close(2) system call to flush a file.
 *
 * @param file
 * @param id
 * @return
 */
static int
trfs_file_flush(
  struct file* const file,
  fl_owner_t const id
) {
  // Log only for test purposes.
  TRFS_INFO("File flush");
  return 0;
}

/**
 * Called when the last reference to an open file is closed.
 *
 * @param inode
 * @param file
 * @return
 */
static int
trfs_file_release(
  struct inode* const inode,
  struct file* const file
) {
  // Log only for test purposes.
  TRFS_INFO("File release");
  return 0;
}

static int
trfs_directory_iterate(
  struct file* const file,
  struct dir_context* const context
) {
  TRFS_INFO("Directory iterate\n");
  // Emit the standard entries "." and "..".
  dir_emit_dots(file, context);
  return 0;
}

struct file_operations const trfs_directory_operations = {
  .owner = THIS_MODULE,
  .iterate = trfs_directory_iterate,

  .open = trfs_file_open,
  .flush = trfs_file_flush,
  .release = trfs_file_release,
};