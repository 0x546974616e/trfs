#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/log2.h>

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

// What about:
// super_block.s_dirt:
//   Set when superblock is changed, and cleared whenever it is written back to disk.
// super_block.s_dirty:
//   A list of all dirty inodes. Recall that if inode is dirty (inode->i_state & I_DIRTY)
//   then it is on superblock-specific dirty list linked via inode->i_list.
int trfs_save_super_block(
  struct super_block* const super_block
) {
  // After sb_bread, the buffer_head and the data block contents are pinned in
  // memory. The page cache won't remove them. Nevertheless, could buffer_head
  // exists but block data not in memory (cache miss)?

  // Disk blocks are represented by buffer_heads. Actual data is in buffer_head->b_data.
  struct buffer_head* const buffer_head = sb_bread(
    super_block, TRFS_SUPER_BLOCK_AT_BLOCK
  );

  if (!buffer_head) {
    TRFS_ERROR("Could not read superblock at block [%u].\n", TRFS_SUPER_BLOCK_AT_BLOCK);
    return -EIO;
  }

  // *((struct oneblockfs_super *)bh->b_data) = *super_block->s_fs_info;

  // Mark the buffer_head dirty so that kernel will eventually sync it to disk.
  mark_buffer_dirty(buffer_head);

  // Actually flush dirty buffer to disk.
  sync_dirty_buffer(buffer_head);

  // Decrement a buffer_head's reference count.
  // Kernel will decide to either keep buffer_head around, or free the
  // buffer_head and flush its data (only if marked as dirty before).
  brelse(buffer_head);

  return 0;
}

int trfs_set_block_size(
  struct super_block* const super_block,
  int size
) {
  // Size must be a power of two, and between 512 and PAGE_SIZE.
  if (size > PAGE_SIZE || size < 512 || !is_power_of_2(size)) {
    return -EINVAL;
  }

  // Size cannot be smaller than the size supported by the device.
  if (size < bdev_logical_block_size(super_block->s_bdev)) {
    return -EINVAL;
  }

  if (!sb_set_blocksize(super_block, size)) {
    return -EINVAL;
  }

  return 0;
}

///
/// Finds the superblock and configures the device block size accordingly.
///
/// @pre super_block != NULL
///
static int trfs_find_super_block(
  struct super_block* const super_block
) {
  int retcode = TRFS_SUCCESS;
  struct buffer_head* buffer_head = NULL;

  // Should already be PAGE_SIZE by default?
  if (!sb_set_blocksize(super_block, PAGE_SIZE)) {
    TRFS_ERROR("Unable to set device block size to page size (%lu).\n", PAGE_SIZE);
    retcode = -EINVAL;
    goto cleanup;
  }

  buffer_head = sb_bread(super_block, 0);
  if (!buffer_head) {
    TRFS_ERROR("Could not read block 0.\n");
    retcode = -EIO;
    goto cleanup;
  }

  // As the block size is smaller than the page size and a power of 2, the
  // superblock is either on the first page (512, 1024, 2048...) or at the
  // beginning of the second page.
  struct trfs_super_block_info* disk_info;
  uint64_t block_size = bdev_logical_block_size(super_block->s_bdev);
  while (block_size < PAGE_SIZE) { // 512 1024 2048 ... < PAGE_SIZE
    disk_info = (struct trfs_super_block_info*) (buffer_head->b_data + block_size);
    if (0 == strncmp(disk_info->magic_number, TRFS_MAGIC_NUMBER, TRFS_MAGIC_NUMBER_LENGTH)) {
      break;
    }

    // Block size is a power of 2.
    block_size <<= 1;
  }

  if (block_size >= PAGE_SIZE) {
    block_size = 0u; // Reset block size.
    brelse(buffer_head); // bforget
    buffer_head = sb_bread(super_block, 1);
    if (!buffer_head) {
      // May happen when device is to small.
      TRFS_ERROR("Could not read block 1.\n");
      retcode = -EIO;
      goto cleanup;
    }
  }

  disk_info = (struct trfs_super_block_info*) (buffer_head->b_data + block_size);
  if (0 == strncmp(disk_info->magic_number, TRFS_MAGIC_NUMBER, TRFS_MAGIC_NUMBER_LENGTH)) {
    // kzalloc() allocates memory and set it with zeros.
    // GFP stands for "Get Free Page", see documentation below.
    // https://www.kernel.org/doc/html/next/core-api/memory-allocation.html
    struct trfs_super_block_info* alloc_info = kzalloc(sizeof(struct trfs_super_block_info), GFP_KERNEL);
    if (alloc_info == NULL) {
      TRFS_ERROR("Could not allocate superblock info.\n");
      retcode = -ENOMEM;
      goto cleanup;
    }

    // Retrieve superblock info.
    memcpy(alloc_info->magic_number, disk_info->magic_number, TRFS_MAGIC_NUMBER_LENGTH);
    // Integers have been encoded to big-endian for readability.
    alloc_info->block_size = be32_to_cpu(disk_info->block_size);
    alloc_info->blocks = be32_to_cpu(disk_info->blocks);
    super_block->s_fs_info = alloc_info;

    TRFS_INFO("Block size: %u\n", alloc_info->block_size);
    TRFS_INFO("Number of blocks: %u\n", alloc_info->blocks);

    // "No-op" if the block size is the same.
    if (!sb_set_blocksize(super_block, alloc_info->block_size)) {
      TRFS_ERROR("Unable to set device block size to page size (%u).\n", alloc_info->block_size);
      retcode = -EINVAL;
      goto cleanup;
    }
  }
  else {
    retcode = -1;
    goto cleanup;
  }

cleanup:
  if (buffer_head != NULL) {
    brelse(buffer_head);
  }

  if (retcode) {
    // Explicitly set as NULL for trfs_kill_super_block().
    super_block->s_fs_info = NULL;
  }

  return retcode;
}

int trfs_fill_super_block(
  struct super_block* const super_block,
  void* const data, // Key-value ASCII options?
  int const silent
) {
  int error;

  // ╔═╗┬ ┬┌─┐┌─┐┬─┐
  // ╚═╗│ │├─┘├┤ ├┬┘
  // ╚═╝└─┘┴  └─┘┴└─

  if ((error = trfs_find_super_block(super_block))) {
    TRFS_ERROR("Unable to find the superblock on disk.\n");
    return error;
  }

  // sb->s_magic = AUFS_MAGIC_NUMBER;
  // sb->s_op = &aufs_super_ops;

  struct inode* const root_inode = new_inode(super_block);
  if (!root_inode) {
    TRFS_ERROR("Could not create the root inode.");
    return -ENOMEM;
  }

  // TODO: What is the size of data block? Do mount_bdev set it correctly?

  // ╦┌┬┐┌┬┐┌─┐┌─┐
  // ║ │││││├─┤├─┘
  // ╩╶┴┘┴ ┴┴ ┴┴

  // https://www.kernel.org/doc/Documentation/filesystems/idmappings.rst

  // From inode_init_owner(): [source/fs/inode.c]
  // Init uid,gid,mode for new inode according to posix standards.
  // If the inode has been created through an idmapped mount the idmap of
  // the vfsmount must be passed through @idmap. This function will then take
  // care to map the inode according to @idmap before checking permissions
  // and initializing i_uid and i_gid. On non-idmapped mounts or if permission
  // checking is to be performed on the raw inode simply pass @nop_mnt_idmap.

  // From [source/include/linux/mnt_idmapping.h]:
  // struct mnt_idmap nop_mnt_idmap; // Identity mapping.
  // struct user_namespace init_user_ns; // ??

  // From "struct super_block::s_user_ns": [source/include/linux/fs.h]
  // Owning user namespace and default context in which to interpret filesystem
  // uids, gids, quotas, device nodes, xattrs and security labels.

  // S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO
  inode_init_owner(super_block->s_user_ns, root_inode, NULL, S_IFDIR | 0755);

  // ╦┌┐┌┌─┐┌┬┐┌─┐
  // ║││││ │ ││├┤
  // ╩┘└┘└─┘╶┴┘└─┘

  root_inode->i_ino = 1;
  // inode->i_ino = get_next_ino();
  // root_inode->i_uid = root_inode->i_gid = 0;
  root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

  root_inode->i_op = &trfs_inode_operations;
  root_inode->i_fop = &trfs_directory_operations;

  // ╔╦╗┌─┐┌┐┌┌┬┐┬─┐┬ ┬
  //  ║║├┤ │││ │ ├┬┘└┬┘
  // ═╩╝└─┘┘└┘ ┴ ┴└─ ┴

  // TODO: Do we have to initialize something about the dcache?
  // (dcache = dentry cache = directory entry cache)
  // d_add(dentry, inode)?

  // From d_alloc_root(): [source/fs/dcache.c] (linux < 3.3.0)
  // Allocate a root ("/") dentry for the inode given. The inode is
  // instantiated and returned. %NULL is returned if there is insufficient
  // memory or the inode passed is %NULL.

  super_block->s_root = d_make_root(root_inode);
  if (!super_block->s_root) {

    // d_make_root() already calls iput(root_inode), what happens when you call
    // iput() twice on an already released inode, does the kernel prevent this?

    TRFS_ERROR("Could not create the root (\"/\") dentry.");
    return -ENOMEM;
  }

  return 0;
}

void trfs_kill_super_block(
  struct super_block* const super_block
) {
  if (super_block->s_fs_info != NULL) {
    TRFS_INFO("Superblock info are released.\n");
    kfree(super_block->s_fs_info);
  }

  // kill_block_super() is an helper function provided by the VFS which
  // unmounts a file system on a block device. This function frees some
  // internal resources.
  kill_block_super(super_block);

  // So far this function is only here for logging.
  TRFS_INFO("Superblock is destroyed.\n");
  TRFS_INFO("Unmount succesful.\n");
}
