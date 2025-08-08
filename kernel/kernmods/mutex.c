/*
 * mutex.c
 */
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>

static DEFINE_MUTEX(demo_lock);

static int __init mutex_demo_init(void) {
  int ret;

  pr_info("mutex_demo init\n");

  ret = mutex_trylock(&demo_lock);
  if (ret != 0) {
    pr_info("mutex is locked\n");

    if (mutex_is_locked(&demo_lock) == 0)
      pr_info("the mutex failed to lock!\n");

    mutex_unlock(&demo_lock);
    pr_info("mutex is unlocked\n");
  } else
    pr_info("failed to lock\n");

  return 0;
}

static void __exit mutex_demo_exit(void) {
  pr_info("mutex_demo exit\n");
}

module_init(mutex_demo_init);
module_exit(mutex_demo_exit);

MODULE_DESCRIPTION("mutex example");
MODULE_LICENSE("GPL");