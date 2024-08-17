#include <linux/module.h> // Needed by all modules
#include <linux/printk.h> // Needed for pr_info()

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Titan 0x546974616e");
MODULE_DESCRIPTION("A simple file system for educational purposes");

// The __init macro causes init functions to be freed once invoked for built-in
// drivers (in-tree build + built-in module), but not loadable modules (in-tree
// loadable module or out-of-tree build). For in-tree build module the kernel
// build system arranges all init functions in the same block of memory and when
// the kernel boots it frees that that one block all at once (at this moment a
// message like "Freeing unused kernel memory: 236k freed" is printed). Loadable
// modules, at the other end, cannot shared their code space between each other,
// therefore freeing indivual .init section is probably more trouble than it's
// worth because these sections are probably smaller than the page size.
//
// The __exit macro causes the omission of the function when the module is built
// into the kernel and like __init has no effect for loadable modules. Built-in
// drivers do not need a cleanup function becausee they cannot be unloaded.
//
// https://sysprog21.github.io/lkmpg/#the-init-and-exit-macros
// https://stackoverflow.com/questions/11680641/init-and-exit-macros-usage-for-built-in-and-loadable-modules
// https://medium.com/@adityapatnaik27/linux-kernel-module-in-tree-vs-out-of-tree-build-77596fc35891

int __init trfs_init(void) {
  // pr_info() is printk() with the KERN_INFO priority.
  // See <linux/prink.h> #define
  // pr_info() use pr_fmt() macro.
  // pr_fmt() can be #undef then #define again with custom format.
  pr_info("Hello world.\n");

  // A non 0 return means init_module failed; module can't be loaded.
  return 0;
}

void __exit trfs_exit(void) {
  // Writes messages to the kernel log buffer.
  pr_info("Goodbye world.\n");
}

module_init(trfs_init);
module_exit(trfs_exit);
