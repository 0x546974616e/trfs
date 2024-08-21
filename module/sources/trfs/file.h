#ifndef TRFS_FILE_H
#define TRFS_FILE_H

// TODO: Make a function to populate an inode.
extern const struct inode_operations trfs_inode_operations;
extern const struct file_operations trfs_directory_operations;

#endif // TRFS_FILE_H
