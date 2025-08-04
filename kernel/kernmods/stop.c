/*
 * stop.c - illustration of multi filed modules
 */

#include <linux/kernel.h> /* we are doing kernel work */
#include <linux/module.h> /* specifically, a module  */

void cleanup_module(void) {
  pr_info("short is the life of a kernel module\n");
}

MODULE_LICENSE("GPL");