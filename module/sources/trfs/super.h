#ifndef TRFS_SUPER_H
#define TRFS_SUPER_H

int trfs_fill_super_block(
  struct super_block* const super_block,
  void* const data, // Key-value ASCII options?
  int const silent
);

void trfs_kill_super_block(
  struct super_block* const super_block
);

#endif // TRFS_SUPER_H
