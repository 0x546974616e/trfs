#ifndef TRFS_INODE_H
#define TRFS_INODE_H

#define TRFS_INODE_MAX_BLOCKS 4u;

#define trfs_packed __attribute__((__packed__))

///
/// Inode on disk.
///
/// Every integer is big-endian encoded.
///
struct trfs_disk_inode {
  uint32_t number; // Inode index.
  uint32_t flags; // File or Directory.

  uint32_t mode; // RWXRWXRWX
  uint32_t owner; // UID
  uint32_t group; // GID

  uint32_t size;
  uint32_t blocks;
  uint32_t block[TRFS_INODE_MAX_BLOCKS];
}

struct trfs_inode_info {
  uint32_t block[TRFS_INODE_MAX_BLOCKS];
}

#ifdef __KERNEL__

#endif

#endif // TRFS_INODE_H
