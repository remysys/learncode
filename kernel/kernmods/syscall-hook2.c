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

/* uid we want to spy on - will be filled from the command line. */
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

  const char __user *filename = (const char __user *)regs->si;

  char fname[256] = {0};
  if (strncpy_from_user(fname, filename, sizeof(fname) - 1) > 0) {
    pr_info("openat by uid %d: %s\n", uid, fname);
  } else {
    pr_info("openat by uid %d: [filename unreadable]\n", uid);
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