/*
 * hello-sysfs.c sysfs example
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>

static struct kobject *kobj;

/* the variable you want to be able to change */
static int foo = 0;

static ssize_t foo_show(struct kobject *kobj,
                               struct kobj_attribute *attr, char *buf) {
  return sprintf(buf, "%d\n", foo);
}

static ssize_t foo_store(struct kobject *kobj,
                                struct kobj_attribute *attr, const char *buf,
                                size_t count) {
  sscanf(buf, "%d", &foo);
  return count;
}

static struct kobj_attribute foo_attribute =
    __ATTR(foo, 0660, foo_show, foo_store);

static int __init hello_sysfs_init(void) {
  int error = 0;

  pr_info("hello_sysfs_init: initialized\n");

  kobj = kobject_create_and_add("hello-sysfs", kernel_kobj);
  if (!kobj)
    return -ENOMEM;

  error = sysfs_create_file(kobj, &foo_attribute.attr);
  if (error) {
    kobject_put(kobj);
    pr_info("failed to create the foo file in /sys/kernel/hello-sysfs\n");
  }

  return error;
}

static void __exit hello_sysfs_exit(void) {
  pr_info("hello_sysfs_init: exit success\n");
  kobject_put(kobj);
}

module_init(hello_sysfs_init);
module_exit(hello_sysfs_exit);

MODULE_LICENSE("GPL");