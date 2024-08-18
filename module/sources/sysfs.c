#include <linux/kobject.h>
#include <linux/minmax.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#include "printk.h"
#include "sysfs.h"

// https://docs.kernel.org/core-api/kobject.html
// https://sysprog21.github.io/lkmpg/#sysfs-interacting-with-your-module
// https://medium.com/@emanuele.santini.88/sysfs-in-linux-kernel-a-complete-guide-part-1-c3629470fc84
// https://medium.com/@emanuele.santini.88/a-complete-guide-to-sysfs-part-2-improving-the-attributes-1dbc1fca9b75

struct trfs_sysfs_data {
 struct kobject trfs_kobject;

  #define TRFS_SYSFS_DADA_SIZE 256u
  char dada[TRFS_SYSFS_DADA_SIZE];
  int fafa;
};

// ╔╦╗┌─┐┌┬┐┌─┐
//  ║║├─┤ ││├─┤
// ═╩╝┴ ┴╶┴┘┴ ┴

static ssize_t trfs_sysfs_dada_show(
  struct kobject* const kobject,
  struct kobj_attribute* const attribute,
  char* const buffer
) {
// container_of(pointer, object type, member)
// Given the address of object.member (pointer), return the address of object.
  struct trfs_sysfs_data* const data = container_of(
    kobject, struct trfs_sysfs_data, trfs_kobject
  );

  // The buffer points to a temporary allocated kernel page.
  // sysfs_emit() is equivalent to scnprintf(), but take care about the
  // temporary buffer size (a page size).
  return sysfs_emit(buffer, "%s\n", data->dada);
}

static ssize_t trfs_sysfs_dada_store(
  struct kobject* const kobject,
  struct kobj_attribute* const attribute,
  char const* const buffer,
  size_t count
) {
  struct trfs_sysfs_data* const data = container_of(
    kobject, struct trfs_sysfs_data, trfs_kobject
  );

  // The min() macro produces a warning even though the two values are of the same type...
  size_t const length = min_t(size_t, count, TRFS_SYSFS_DADA_SIZE - 1u);
  strncpy(data->dada, buffer, length);
  data->dada[length] = 0x0; // :thinking:

  return length;
}

// ╔═╗┌─┐┌─┐┌─┐
// ╠╣ ├─┤├┤ ├─┤
// ╚  ┴ ┴└  ┴ ┴

// ╔═╗┬ ┬┌─┐┌─┐┌─┐
// ╚═╗└┬┘└─┐├┤ └─┐
// ╚═╝ ┴ └─┘└  └─┘

static struct kobj_attribute trfs_sysfs_dada_attribute = __ATTR(
  dada, 0664, trfs_sysfs_dada_show, trfs_sysfs_dada_store
);

static struct attribute* trfs_sysfs_attributes[] = {
  &trfs_sysfs_dada_attribute.attr,
  NULL,
};

// We could use ATTRIBUTE_GROUPS(trfs_sysfs_attrs) but it forces the
// "attributes" to be named "attrs".
static struct attribute_group const trfs_sysfs_group = {
 .attrs = trfs_sysfs_attributes,
};

static struct attribute_group const* trfs_sysfs_groups[] = {
  &trfs_sysfs_group,
  NULL,
};

static const struct kobj_type trfs_sysfs_ktype = {
 .default_groups = trfs_sysfs_groups,
 .sysfs_ops = &kobj_sysfs_ops,
  // .release = trfs_sysfs_release,
};

#define TRFS_SYSFS_NAME "trfs"
static struct trfs_sysfs_data * trfs_sysfs_data = NULL;

int __init trfs_sysfs_init(void) {
  pr_info("TRFS(sysfs) init\n");

  // kzalloc() allocates memory and set it with zeros.
  // GFP stands for "Get Free Page", see documentation below.
  // https://www.kernel.org/doc/html/next/core-api/memory-allocation.html
  trfs_sysfs_data = kzalloc(sizeof(struct trfs_sysfs_data), GFP_KERNEL);

  if (trfs_sysfs_data == NULL) {
    pr_err("Could not allocate TRFS kobject\n");
    return -ENOMEM;
  }

  #define TRFS_SYSFS_DADA_INITIAL "Hello from TRFS!"
  size_t const length = min_t(size_t, sizeof(TRFS_SYSFS_DADA_INITIAL), TRFS_SYSFS_DADA_SIZE) - 1u;
  strncpy(trfs_sysfs_data->dada, TRFS_SYSFS_DADA_INITIAL, length);
  trfs_sysfs_data->dada[length] = 0x0;

  // Create the kobject at /sys/kernel/trfs.
  // https://docs.kernel.org/core-api/kobject.html#initialization-of-kobjects
  int error = kobject_init_and_add(
    &trfs_sysfs_data->trfs_kobject,
    &trfs_sysfs_ktype,
    kernel_kobj, // TODO: /sys/fs/
    "%s", TRFS_SYSFS_NAME
  );

  // We could have used kobject_create_and_add() and sysfs_create_file(), but we
  // are storing our variables ("dada" and "fafa") inside the same structure as
  // the kobject. The longest way has been chosen for educational purposes.
  // https://docs.kernel.org/core-api/kobject.html#creating-simple-kobjects
  // https://github.com/torvalds/linux/blob/master/samples/kobject/kobject-example.c
  if (error) {
    pr_err("Could not initialize /sys/kernel/%s\n", TRFS_SYSFS_NAME);

    // https://www.kernel.org/doc/html/latest/driver-api/basics.html
    // According to the documentation, kobject_put() MUST always be called
    // whether an error occurs or not.
    kobject_put(&trfs_sysfs_data->trfs_kobject);
    kfree(trfs_sysfs_data);
    trfs_sysfs_data = NULL;
    return -ENOMEM;
  }

  pr_info("/sys/kernel/%s created\n", TRFS_SYSFS_NAME);
  return 0;
}

void __exit trfs_sysfs_exit(void) {
  pr_info("TRFS(sysfs) exit\n");

  if (trfs_sysfs_data != NULL) {
    kobject_put(&trfs_sysfs_data->trfs_kobject);
    kfree(trfs_sysfs_data);
    pr_info("/sys/kernel/%s removed\n", TRFS_SYSFS_NAME);
  }
}
