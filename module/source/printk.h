#ifndef TRFS_PRINTK_H
#define TRFS_PRINTK_H

#include <linux/printk.h>

#ifdef pr_fmt
#undef pr_fmt
#endif

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

// __FILE_NAME__ is not standard.
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
// https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html

// NOTE: pr_fmt() can be defined before the #include <linux/printk.h>:
// https://github.com/torvalds/linux/blob/e5fa841af679cb830da6c609c740a37bdc0b8b35/include/linux/printk.h#L354

#define pr_fmt(fmt) KBUILD_MODNAME ":" __FILE_NAME__ ":" STRINGIFY(__LINE__) ": " fmt

#endif // TRFS_PRINTK_H
