/*
 * hello-3.c - illustrating the __init, __initdata and __exit macros.
 */
#include <linux/init.h>   /* needed for the macros */
#include <linux/module.h> /* needed by all modules */
#include <linux/printk.h> /* needed for pr_info() */

static int hello3_data __initdata = 3;

static int __init hello_3_init(void) {
  pr_info("hello, world %d\n", hello3_data);
  return 0;
}

static void __exit hello_3_exit(void) {
  pr_info("goodbye, world 3\n");
}

module_init(hello_3_init);
module_exit(hello_3_exit);

MODULE_LICENSE("GPL");