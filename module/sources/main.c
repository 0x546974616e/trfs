#include <linux/module.h>

#include "misc/procfs.h"
#include "misc/sysfs.h"
#include "trfs/impl.h"
#include "trfs/printk.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Titan 0x546974616e");
MODULE_DESCRIPTION("A simple file system for educational purposes");

// The __init macro causes init functions to be freed once invoked for built-in
// drivers (in-tree build + built-in module), but not loadable modules (in-tree
// loadable module or out-of-tree build).
//
// For in-tree build module the kernel build system arranges all init functions
// in the same block of memory and when the kernel boots it frees that that one
// block all at once (at this moment a message like "Freeing unused kernel
// memory: 236k freed" is logged).
//
// Loadable modules, at the other end, cannot shared their code space between
// each other, therefore freeing indivual .init section is probably more trouble
// than it's worth because these sections are probably smaller than the page
// size.
//
// The __exit macro causes the omission of the function when the module is built
// into the kernel and like __init has no effect for loadable modules. Built-in
// drivers do not need a cleanup function because they cannot be unloaded.
//
// However, in our case, init functions call exit functions whenever an error
// occurs. Because these functions are stored in different sections (namely
// .init.text and .exit.text) they have a different lifetimes (functions may be
// released before use). Therefore, __init and __exit macros are not used (see
// modpost).
//
// https://sysprog21.github.io/lkmpg/#the-init-and-exit-macros
// https://stackoverflow.com/questions/11680641/init-and-exit-macros-usage-for-built-in-and-loadable-modules
// https://medium.com/@adityapatnaik27/linux-kernel-module-in-tree-vs-out-of-tree-build-77596fc35891
// https://stackoverflow.com/questions/8563978/what-is-kernel-section-mismatch


static int trfs_init(void) {
  pr_info("TRFS(main) init\n");

  int error;
  if ((error = trfs_impl_init())) goto trfs_cleanup;
  if ((error = trfs_procfs_init())) goto procfs_cleanup;
  if ((error = trfs_sysfs_init())) goto sysfs_cleanup;
  return 0; // Success

  // Expect cleanup functions to be robust.
  sysfs_cleanup: trfs_sysfs_exit();
  procfs_cleanup: trfs_procfs_exit();
  trfs_cleanup: trfs_impl_exit();
  return error;
}

static void __exit trfs_exit(void) {
  pr_info("TRFS(main) exit\n");

  trfs_impl_exit();
  trfs_procfs_exit();
  trfs_sysfs_exit();
}

module_init(trfs_init);
module_exit(trfs_exit);
