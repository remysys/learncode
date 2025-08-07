/*
 * syscall-hook2.c
 *
 * system call "stealing" sample.
 *
 * disables page protection at a processor level by changing the 16th bit
 * in the cr0 register (could be Intel specific).
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>  /* which will have params */
#include <linux/unistd.h>       /* the list of system calls */
#include <linux/cred.h>         /* for current_uid() */
#include <linux/uidgid.h>       /* for __kuid_val() */
#include <linux/version.h>

/* for the current (process) structure, we need this to know who the current user is. */
#include <linux/sched.h>
#include <linux/uaccess.h>

#include <linux/kprobes.h>

/* 
 * uid we want to spy on - will be filled from the command line. 
 * you can check your user uid by running `id -u` in the shell.
 * example usage:
 * insmod syscall-hook2.ko uid=0
 */

static uid_t uid = -1;
module_param(uid, int, 0644);


/* syscall_sym is the symbol name of the syscall to spy on. the default is
 * "__x64_sys_openat", which can be changed by the module parameter. you can 
 * look up the symbol name of a syscall in /proc/kallsyms.
 */
static char *syscall_sym = "__x64_sys_openat";
module_param(syscall_sym, charp, 0644);

static int sys_call_kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs) {
  if (__kuid_val(current_uid()) != uid) {
    return 0;
  }

  /*
   * __x64_sys_openat is the actual syscall handler for openat, defined as:
   *   __SYSCALL_DEFINEx(4, openat, int dfd, const char __user *filename, ...)
   * this macro expands to a function with a single argument:
   *   long __x64_sys_openat(struct pt_regs *regs);
   * where the real syscall arguments (dfd, filename, etc.) are stored inside the pt_regs structure.

   * according to the x86-64 calling convention:
   * - the pointer to the pt_regs struct is passed in the RDI register when __x64_sys_openat is called.
   * - thus, regs->di holds the address of the pt_regs struct containing the real syscall arguments.

   * we cast regs->di to (struct pt_regs *) to access these real arguments.
   * for the openat syscall, the second argument 'filename' is located in the RSI register,
   * so we read syscall_regs->si to get the filename pointer.
   */

  char fname[256] = {0};
  struct pt_regs *syscall_regs = (struct pt_regs *)regs->di;
  const char __user *filename = (const char __user *)syscall_regs->si;

  if (strncpy_from_user(fname, filename, sizeof(fname) - 1) > 0) {
    pr_info("open %s by uid %d\n", fname, uid);
  }

  return 0;
}

static struct kprobe kp = {
  .symbol_name = "__x64_sys_openat",
  .pre_handler = sys_call_kprobe_pre_handler,
};


static int __init syscall_hook_init(void) {
  int err;
  /* use symbol name from the module parameter */
  kp.symbol_name = syscall_sym;
  err = register_kprobe(&kp);
  if (err) {
    pr_err("register_kprobe() on %s failed: %d\n", syscall_sym, err);
    pr_err("please check the symbol name from 'syscall_sym' parameter.\n");
    return err;
  }

  pr_info("spying on uid:%d\n", uid);
  return 0;
}

static void __exit syscall_hook_exit(void) {
  unregister_kprobe(&kp);
  msleep(2000);
}

module_init(syscall_hook_init);
module_exit(syscall_hook_exit);

MODULE_LICENSE("GPL");