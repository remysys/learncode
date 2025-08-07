/*
 * syscall-hook.c
 *
 * system call "stealing" sample.
 *
 * disables page protection at a processor level by changing the 16th bit
 * in the cr0 register (could be Intel specific).
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>  /* which will have params   */
#include <linux/unistd.h>       /* the list of system calls */
#include <linux/cred.h>         /* for current_uid()        */
#include <linux/uidgid.h>       /* for __kuid_val()         */
#include <linux/version.h>

/* for the current (process) structure, we need this to know who the current user is. */

#include <linux/sched.h>
#include <linux/uaccess.h>

#define HAVE_PARAM 1
#include <linux/kallsyms.h>   /* for sprint_symbol */
/* the address of the sys_call_table, which can be obtained with looking up
 * "/boot/System.map" or "/proc/kallsyms". when the kernel version is v5.7+,
 * without CONFIG_KPROBES, you can input the parameter or the module will look
 * up all the memory.
 */
static unsigned long sym = 0;
module_param(sym, ulong, 0644);

/* uid we want to spy on - will be filled from the command line. */
static uid_t uid = -1;
module_param(uid, int, 0644);

static unsigned long **sys_call_table_stolen;

/* a pointer to the original system call. the reason we keep this, rather
 * than call the original function (sys_openat), is because somebody else
 * might have replaced the system call before us. note that this is not
 * 100% safe, because if another module replaced sys_openat before us,
 * then when we are inserted, we will call the function in that module -
 * and it might be removed before we are.
 *
 * another reason for this is that we can not get sys_openat.
 * it is a static variable, so it is not exported.
 */

static asmlinkage long (*original_call)(const struct pt_regs *);

/* the function we will replace sys_openat (the function called when you
 * call the open system call) with. to find the exact prototype, with
 * the number and type of arguments, we find the original function first
 * (it is at fs/open.c).
 *
 * in theory, this means that we are tied to the current version of the
 * kernel. in practice, the system calls almost never change (it would
 * wreck havoc and require programs to be recompiled, since the system
 * calls are the interface between the kernel and the processes).
 */

static asmlinkage long hooked_sys_openat(const struct pt_regs *regs) {
  int i = 0;
  char ch;

  pr_alert("hooked_sys_openat called by uid:%d\n", __kuid_val(current_uid()));

  
  if (__kuid_val(current_uid()) != uid)
    goto orig_call;

  /* report the file, if relevant */
  pr_info("opened file by %d: ", uid);
  
  do {
    get_user(ch, (char __user *)regs->si + i);

    i++;
    pr_info("%c", ch);
  } while (ch != 0);
  pr_info("\n");
  
orig_call:
  /* call the original sys_openat - otherwise, we lose the ability to open files. */
  return original_call(regs);
}

static unsigned long **acquire_sys_call_table(void) {
  const char sct_name[15] = "sys_call_table";
  char symbol[40] = { 0 };

  if (sym == 0) {
    pr_alert("for linux v5.7+, kprobes is the preferable way to get symbol.\n");
    pr_info("if kprobes is absent, you have to specify the address of sys_call_table symbol\n");
    pr_info("by /boot/System.map or /proc/kallsyms, which contains all the symbol addresses, into sym parameter.\n");
    return NULL;
  }
  sprint_symbol(symbol, sym);
  pr_info("looking for %s at %lx\n", symbol, sym);

  if (!strncmp(sct_name, symbol, sizeof(sct_name) - 1))
    return (unsigned long **)sym;

  return NULL;
}

static inline void __write_cr0(unsigned long cr0) {
  asm volatile("mov %0,%%cr0" : "+r"(cr0) : : "memory");
}

static void enable_write_protection(void) {
  unsigned long cr0 = read_cr0();
  set_bit(16, &cr0);
  __write_cr0(cr0);
}

static void disable_write_protection(void) {
  unsigned long cr0 = read_cr0();
  clear_bit(16, &cr0);
  __write_cr0(cr0);
}

static int __init syscall_hook_init(void) {

  if (!(sys_call_table_stolen = acquire_sys_call_table()))
    return -1;

  disable_write_protection();

  /* keep track of the original open function */
  original_call = (void *)sys_call_table_stolen[__NR_openat];

  /* use our openat function instead */
  sys_call_table_stolen[__NR_openat] = (unsigned long *)hooked_sys_openat;

  enable_write_protection();

  pr_info("spying on uid:%d\n", uid);
  return 0;
}

static void __exit syscall_hook_exit(void) {

  if (!sys_call_table_stolen)
      return;

  /* return the system call back to normal */
  if (sys_call_table_stolen[__NR_openat] != (unsigned long *)hooked_sys_openat) {
    pr_alert("somebody else also played with the open system call\n");
    pr_alert("the system may be left in an unstable state.\n");
  }

  disable_write_protection();
  sys_call_table_stolen[__NR_openat] = (unsigned long *)original_call;
  enable_write_protection();

  msleep(2000);
}

module_init(syscall_hook_init);
module_exit(syscall_hook_exit);
MODULE_LICENSE("GPL");