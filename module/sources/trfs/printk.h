#ifndef TRFS_PRINTK_H
#define TRFS_PRINTK_H

#include <linux/printk.h>

#ifdef pr_fmt
#undef pr_fmt
#endif

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

#define TRFS_SUCCESS 0 // TODO: Move

// __FILE_NAME__ is not standard.
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
// https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html

// NOTE: pr_fmt() can be defined before the #include <linux/printk.h>:
// https://github.com/torvalds/linux/blob/e5fa841af679cb830da6c609c740a37bdc0b8b35/include/linux/printk.h#L354

#define pr_fmt(fmt) KBUILD_MODNAME ":" __FILE_NAME__ ":" STRINGIFY(__LINE__) ": " fmt

#define TRFS_PRINTK(level, fmt, ...) \
  printk(                            \
    level                            \
    KBUILD_MODNAME ":"               \
    __FILE_NAME__ ":"                \
    "%s:" /* __func__ */             \
    STRINGIFY(__LINE__) ": " fmt     \
    , __func__                       \
    , ##__VA_ARGS__                  \
  )

// Print an emergency-level message.
#define TRFS_EMERG(fmt, ...) \
  TRFS_PRINTK(KERN_EMERG, fmt, ##__VA_ARGS__)

// Print an alert-level message.
#define TRFS_ALERT(fmt, ...) \
  TRFS_PRINTK(KERN_ALERT, fmt, ##__VA_ARGS__)

// Print a critical-level message.
#define TRFS_CRIT(fmt, ...) \
  TRFS_PRINTK(KERN_CRIT, fmt, ##__VA_ARGS__)

// Print an error-level message.
#define TRFS_ERROR(fmt, ...) \
  TRFS_PRINTK(KERN_ERR, fmt, ##__VA_ARGS__)

// Print a warning-level message.
#define TRFS_WARN(fmt, ...) \
  TRFS_PRINTK(KERN_WARNING, fmt, ##__VA_ARGS__)

// Print a notice-level message.
#define TRFS_NOTICE(fmt, ...) \
  TRFS_PRINTK(KERN_NOTICE, fmt, ##__VA_ARGS__)

// Print an info-level message.
#define TRFS_INFO(fmt, ...) \
  TRFS_PRINTK(KERN_INFO, fmt, ##__VA_ARGS__)

// Continues a previous log message (without '\n') in the same line.
#define TRFS_CONT(fmt, ...) \
  printk(KERN_CONT fmt, ##__VA_ARGS__)

#endif // TRFS_PRINTK_H
