/*
 * hello-1.c - the simplest kernel module.
 */
#include <linux/module.h> /* meeded by all modules */
#include <linux/printk.h> /* needed for pr_info() */

int init_module(void) {
  pr_info("hello world 1.\n");
  /* a nonzero return means init_module failed; module can't be loaded. */
  return 0;
}

void cleanup_module(void) {
  pr_info("goodbye world 1.\n");
}

MODULE_LICENSE("GPL");