#include <linux/fs.h>

#include <trfs/inode.h>

// The Inode Object:
// An inode object represents an object within the filesystem.
// https://www.kernel.org/doc/Documentation/filesystems/vfs.txt

// https://codebrowser.dev/linux/linux/fs/minix/inode.c.html#minix_aops

int trfs_make_inode(
  struct super_block* super_block,
) {
  return 0;
}

struct inode* trfs_inode_from_disk(
  struct super_block* super_block,
  struct trfs_disk_inode* disk_inode
) {
  return NULL;
}
