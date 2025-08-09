/*
 * tasklet.c
 */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/printk.h>

static void tasklet_fn(struct tasklet_struct *t) {
  pr_info("example tasklet starts\n");
  mdelay(5000);
  pr_info("example tasklet ends\n");
}

static DECLARE_TASKLET(demo_task, tasklet_fn);

static int __init tasklet_demo_init(void) {
  pr_info("tasklet example init\n");
  tasklet_schedule(&demo_task);
  pr_info("example tasklet init continues...\n");
  return 0;
}

static void __exit tasklet_demo_exit(void) {
  pr_info("tasklet example exit\n");
  tasklet_kill(&demo_task);
}

module_init(tasklet_demo_init);
module_exit(tasklet_demo_exit);

MODULE_DESCRIPTION("tasklet example");
MODULE_LICENSE("GPL");