/*
 * spinlock.c
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/spinlock.h>

static DEFINE_SPINLOCK(static_lock);
static spinlock_t dynamic_lock;

static void spinlock_demo_static(void) {
  unsigned long flags;

  spin_lock_irqsave(&static_lock, flags);
  pr_info("locked static spinlock\n");

  /*
   * do something or other safely. because this uses 100% CPU time, this
   * code should take no more than a few milliseconds to run.
   */

  spin_unlock_irqrestore(&static_lock, flags);
  pr_info("unlocked static spinlock\n");
}

static void spinlock_demo_dynamic(void) {
  unsigned long flags;

  spin_lock_init(&dynamic_lock);
  spin_lock_irqsave(&dynamic_lock, flags);
  pr_info("locked dynamic spinlock\n");

  /*
   * do something or other safely. because this uses 100% CPU time, this
   * code should take no more than a few milliseconds to run.
   */

  spin_unlock_irqrestore(&dynamic_lock, flags);
  pr_info("unlocked dynamic spinlock\n");
}

static int __init spinlock_demo_init(void) {
  pr_info("spinlock demo started\n");

  spinlock_demo_static();
  spinlock_demo_dynamic();

  return 0;
}

static void __exit spinlock_demo_exit(void) {
  pr_info("spinlock demo exit\n");
}

module_init(spinlock_demo_init);
module_exit(spinlock_demo_exit);

MODULE_DESCRIPTION("spinlock example");
MODULE_LICENSE("GPL");