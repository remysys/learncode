/*
 * rwlock.c
 */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/rwlock.h>

static DEFINE_RWLOCK(demo_rwlock);

static void rwlock_demo_read(void) {
  unsigned long flags;

  read_lock_irqsave(&demo_rwlock, flags);
  pr_info("read locked\n");

  /* read from something */

  read_unlock_irqrestore(&demo_rwlock, flags);
  pr_info("read unlocked\n");
}

static void rwlock_demo_write(void) {
  unsigned long flags;

  write_lock_irqsave(&demo_rwlock, flags);
  pr_info("write locked\n");

  /* write to something */

  write_unlock_irqrestore(&demo_rwlock, flags);
  pr_info("write unlocked\n");
}

static int __init rwlock_demo_init(void) {
  pr_info("rwlock demo started\n");

  rwlock_demo_read();
  rwlock_demo_write();

  return 0;
}

static void __exit rwlock_demo_exit(void) {
  pr_info("rwlock demo exit\n");
}

module_init(rwlock_demo_init);
module_exit(rwlock_demo_exit);

MODULE_DESCRIPTION("read/write locks example");
MODULE_LICENSE("GPL");