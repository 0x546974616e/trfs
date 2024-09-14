#ifndef TRFS_SUPER_H
#define TRFS_SUPER_H

/// Where to find the super block (0 is boot block?).
#define TRFS_SUPER_BLOCK_AT_BLOCK 1u

#define TRFS_MAGIC_NUMBER "TRFS/1.0"
#define TRFS_MAGIC_NUMBER_LENGTH 8u

struct trfs_super_block_info {
  /// The filesystem’s magic number.
  uint8_t magic_number[TRFS_MAGIC_NUMBER_LENGTH];

  /// The filesystem's block size (!= disk's block size).
  uint32_t block_size;

  /// The number of blocks.
  uint32_t blocks;
};

#ifdef __KERNEL__

  int trfs_fill_super_block(
    struct super_block* const super_block,
    void* const data, // Key-value ASCII options?
    int const silent
  );

  void trfs_kill_super_block(
    struct super_block* const super_block
  );

#endif // __KERNEL__

#endif // TRFS_SUPER_H
