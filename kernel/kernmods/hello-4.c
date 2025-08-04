/*
 * hello-4.c - demonstrates module documentation.
 */
#include <linux/init.h>   /* needed for the macros */
#include <linux/module.h> /* needed by all modules */
#include <linux/printk.h> /* needed for pr_info()  */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKMPG");
MODULE_DESCRIPTION("a sample driver");

static int __init init_hello_4(void) {
  pr_info("hello, world 4\n");
  return 0;
}

static void __exit cleanup_hello_4(void) {
  pr_info("goodbye, world 4\n");
}

module_init(init_hello_4);
module_exit(cleanup_hello_4);