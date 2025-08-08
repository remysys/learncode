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

static asmlinkage long (*original_call)(const struct pt_regs *);

static asmlinkage long hooked_sys_openat(const struct pt_regs *regs) {
    
  if (__kuid_val(current_uid()) != uid)
    goto orig_call;

  char fname[256] = {0};
  const char __user *filename = (const char __user *)regs->si;

  if (strncpy_from_user(fname, filename, sizeof(fname) - 1) > 0) {
    pr_info("open %s by uid %d\n", fname, uid);
  }

orig_call:
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
  original_call = (void *)sys_call_table_stolen[__NR_openat];
  sys_call_table_stolen[__NR_openat] = (unsigned long *)hooked_sys_openat;

  enable_write_protection();

  pr_info("spying on uid:%d\n", uid);
  return 0;
}

static void __exit syscall_hook_exit(void) {

  if (!sys_call_table_stolen)
      return;

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


 /*
 * sys_call_table is no longer used for actual syscall dispatch since Linux v6.9.
 * commit 1e3ad78334a69b36e107232e337f9d693dcc9df2 introduced a security
 * mitigation against speculative execution on x86 by removing the syscall table
 * from the dispatch path. this change has been backported to:
 *   - v6.8.5+
 *   - v6.6.26+
 *   - v6.1.85+
 *   - v5.15.154+
 *
 * as a result, sys_call_table is now only used for tracing (CONFIG_FTRACE_SYSCALLS=y).
 * the actual syscall dispatch is now implemented as a large inlined switch-case:
 *
 *   #define __SYSCALL(nr, sym) case nr: return __x64_##sym(regs);
 *
 *   long x64_sys_call(const struct pt_regs *regs, unsigned int nr)
 *   {
 *       switch (nr) {
 *       #include <asm/syscalls_64.h>
 *       default: return __x64_sys_ni_syscall(regs);
 *       }
 *   };
 *
 * this means that replacing sys_call_table entries no longer affects the
 * real syscall execution path on modern kernels.
 *
 * reference:
 *   https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=1e3ad78334a69b36e107232e337f9d693dcc9df2
 */