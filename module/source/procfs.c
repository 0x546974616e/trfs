#include <linux/proc_fs.h>
#include <linux/version.h>

#include "printk.h"
#include "procfs.h"

// https://sysprog21.github.io/lkmpg/#the-proc-file-system
// https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html

#define TRFS_PROCFS_BUFFER_SIZE 128u
#define TRFS_PROCFS_INITIAL "Hello TRFS!"
static char trfs_procfs_buffer[TRFS_PROCFS_BUFFER_SIZE] = TRFS_PROCFS_INITIAL;
static size_t trfs_procfs_buffer_size = sizeof(TRFS_PROCFS_INITIAL) - 1;

static ssize_t trfs_procfs_read(
  struct file* const file,
  char __user* const user_buffer,
  size_t const user_buffer_length,
  loff_t* const file_offset
) {
  // loff_t is a signed integer (man loff_t).
  if (*file_offset < 0) {
    return -EINVAL;
  }

  // An unsigned subtraction is upcomming.
  if (*file_offset >= trfs_procfs_buffer_size) {
    return 0;
  }

  size_t actual_length = min(
    // The (size_t*) cast should be safe here (isn't it?).
    trfs_procfs_buffer_size - *(size_t*)file_offset,
    user_buffer_length // Should not be zero?
  );

  if (actual_length <= 0) {
    return -EINVAL; // Or 0 here?
  }

  if (copy_to_user(
    user_buffer,
    trfs_procfs_buffer + *file_offset,
    actual_length
  )) {
    // copy_to_user() returns number of bytes that could not be copied.
    // What happens when not returning an error? It is in a way a success.
    return -EFAULT;
  }

  *file_offset += actual_length;
  return actual_length;
}

static ssize_t trfs_procfs_write(
  struct file* const file,
  char __user const* const user_buffer,
  size_t user_buffer_length,
  loff_t* const file_offset
) {
  // loff_t is a signed integer (man loff_t).
  if (*file_offset < 0) {
    return -EINVAL;
  }

  // Prevent buffer overflow and subtraction overflow.
  if (*file_offset >= TRFS_PROCFS_BUFFER_SIZE) {
    // There is an infinite loop when returning 0.
    // I would like to RTFM but where to fint it?
    return -EINVAL;
  }

  size_t actual_length = min(
    // The (size_t*) cast should be safe here (right?).
    TRFS_PROCFS_BUFFER_SIZE - *(size_t*)file_offset,
    user_buffer_length // Should be non-zero?
  );

  if (actual_length <= 0) {
    return -EINVAL;
  }

  if (copy_from_user(
    trfs_procfs_buffer + *file_offset,
    user_buffer,
    actual_length
  )) {
    // We can technically not return an error if some bytes have actually
    // been copied and adjust the file_offset consequently.
    return -EFAULT;
  }

  *file_offset += actual_length;
  trfs_procfs_buffer_size = *file_offset; // :thinking:
  return actual_length;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
  // Prior to Linux 5.6 a file_operations is needed.
  static struct proc_ops trfs_procfs_fops = {
    .proc_read = trfs_procfs_read,
    .proc_write = trfs_procfs_write,
  };
#else
  static struct file_operations trfs_procfs_fops = {
    .owner = THIS_MODULE,
    .read = trfs_procfs_read,
    .write = trfs_procfs_write,
  }
#endif

static struct proc_dir_entry* trfs_procfs_entry;
static char const* const TRFS_PROCFS_NAME = "trfs";

int __init trfs_procfs_init(void) {
  pr_info("TRFS(procfs) init\n");

  trfs_procfs_entry = proc_create(
    TRFS_PROCFS_NAME, 0666, NULL, // See proc_mkdir().
    &trfs_procfs_fops
  );

  if (trfs_procfs_entry == NULL) {
    pr_err("Could not initialize /proc/%s\n", TRFS_PROCFS_NAME);
    return -ENOMEM;
  }

  pr_info("/proc/%s created\n", TRFS_PROCFS_NAME);
  return 0;
}

void __exit trfs_procfs_exit(void) {
  pr_info("TRFS(procfs) exit\n");

  if (trfs_procfs_entry != NULL) {
    proc_remove(trfs_procfs_entry);
    pr_info("/proc/%s removed\n", TRFS_PROCFS_NAME);
  }
}
