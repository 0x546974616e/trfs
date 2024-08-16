#include <linux/module.h> // Needed by all modules
#include <linux/printk.h> // Needed for pr_info()

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Titan 0x546974616e");
MODULE_DESCRIPTION("A simple file system for educational purposes");

int init_module(void) {
  // pr_info() is printk() with the KERN_INFO priority.
  // See <linux/prink.h> #define
  // pr_info() use pr_fmt() macro.
  // pr_fmt() can be #undef then #define again with custom format.
  pr_info("Hello world.\n");

  // A non 0 return means init_module failed; module can't be loaded.
  return 0;
}

void cleanup_module(void) {
  // Writes messages to the kernel log buffer.
  pr_info("Goodbye world.\n");
}
