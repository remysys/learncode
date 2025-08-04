/*
 * start.c - illustration of multi filed modules
 */

#include <linux/kernel.h> /* we are doing kernel work */
#include <linux/module.h> /* specifically, a module */

int init_module(void) {
  pr_info("hello, world - this is the kernel speaking\n");
  return 0;
}

MODULE_LICENSE("GPL");