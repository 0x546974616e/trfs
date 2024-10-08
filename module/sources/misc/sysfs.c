#include <linux/kobject.h>
#include <linux/minmax.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#include "misc/sysfs.h"
#include "trfs/printk.h"

// https://docs.kernel.org/core-api/kobject.html
// https://www.kernel.org/doc/Documentation/filesystems/sysfs.txt
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
  // container_of(pointer, object type, member).
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
  // Given the sample structure, the two below statements are equivalents:
  // | struct sample { int mem1; char mem2; int mem3; }
  // | 1. struct sample* pointer = &sample1;
  // | 2. struct sample* pointer = container_of(&sample1.mem3, struct sample, mem3);
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

static ssize_t trfs_sysfs_fafa_show(
  struct kobject* const kobject,
  struct kobj_attribute* const attribute,
  char* const buffer
) {
  struct trfs_sysfs_data* const data = container_of(
    kobject, struct trfs_sysfs_data, trfs_kobject
  );

  return sysfs_emit(buffer, "%d\n", data->fafa);
}

static ssize_t trfs_sysfs_fafa_store(
  struct kobject* const kobject,
  struct kobj_attribute* const attribute,
  char const* const buffer,
  size_t count
) {
  struct trfs_sysfs_data* const data = container_of(
    kobject, struct trfs_sysfs_data, trfs_kobject
  );

  // TODO: Ensure that the buffer is null-terminated (man kstrtoint).
  int error = kstrtoint(buffer, 10, &data->fafa);
  return error < 0 ? error : count;
}

// ╔═╗┬ ┬┌─┐┌─┐┌─┐
// ╚═╗└┬┘└─┐├┤ └─┐
// ╚═╝ ┴ └─┘└  └─┘

static struct kobj_attribute trfs_sysfs_dada_attribute = __ATTR(
  dada, 0664, trfs_sysfs_dada_show, trfs_sysfs_dada_store
);

static struct kobj_attribute trfs_sysfs_fafa_attribute = __ATTR(
  fafa, 0664, trfs_sysfs_fafa_show, trfs_sysfs_fafa_store
);

static struct attribute* trfs_sysfs_attributes[] = {
  &trfs_sysfs_dada_attribute.attr,
  &trfs_sysfs_fafa_attribute.attr,
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

// See kset and how it is useful.
static const struct kobj_type trfs_sysfs_ktype = {
 .default_groups = trfs_sysfs_groups,
 .sysfs_ops = &kobj_sysfs_ops,
  // .release = trfs_sysfs_release,
};

#define TRFS_SYSFS_NAME "trfs"
static struct trfs_sysfs_data * trfs_sysfs_data = NULL;

int trfs_sysfs_init(void) {
  // kzalloc() allocates memory and set it with zeros.
  // GFP stands for "Get Free Page", see documentation below.
  // https://www.kernel.org/doc/html/next/core-api/memory-allocation.html
  trfs_sysfs_data = kzalloc(sizeof(struct trfs_sysfs_data), GFP_KERNEL);

  if (trfs_sysfs_data == NULL) {
    TRFS_ERROR("Could not allocate TRFS kobject\n");
    return -ENOMEM;
  }

  #define TRFS_SYSFS_DADA_INITIAL "Hello from TRFS!"
  size_t const length = min_t(size_t, sizeof(TRFS_SYSFS_DADA_INITIAL), TRFS_SYSFS_DADA_SIZE) - 1u;
  strncpy(trfs_sysfs_data->dada, TRFS_SYSFS_DADA_INITIAL, length);
  trfs_sysfs_data->dada[length] = 0x0;
  trfs_sysfs_data->fafa = 220; // Amicable number with 284.

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
    TRFS_ERROR("Could not initialize /sys/kernel/%s\n", TRFS_SYSFS_NAME);

    // https://www.kernel.org/doc/html/latest/driver-api/basics.html
    // According to the documentation, kobject_put() MUST always be called
    // whether an error occurs or not.
    kobject_put(&trfs_sysfs_data->trfs_kobject);
    kfree(trfs_sysfs_data);
    trfs_sysfs_data = NULL;
    return -ENOMEM;
  }

  TRFS_INFO("/sys/kernel/%s created\n", TRFS_SYSFS_NAME);
  return 0;
}

void trfs_sysfs_exit(void) {
  if (trfs_sysfs_data != NULL) {
    kobject_put(&trfs_sysfs_data->trfs_kobject);
    kfree(trfs_sysfs_data);
    TRFS_INFO("/sys/kernel/%s removed\n", TRFS_SYSFS_NAME);
  }
}
