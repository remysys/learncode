#include <asm/ptrace.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ftrace.h>
#include <linux/sched.h>
#include <linux/uaccess.h>


/* 
 * target_ip is the address of the function to hook (e.g. __x64_sys_openat).
 * you can get this address by running the following command in the shell:
 *
 *   grep '__x64_sys_openat' /proc/kallsyms
 *
 * example output:
 *   ffffffffb7d5e590 T __x64_sys_openat
 *
 * use the hex address (e.g. 0xffffffffb7d5e590) as the target_ip module parameter
 * when loading the module:
 *
 * insmod syscall-hook3.ko uid=0 target_ip=0xffffffffb7d5e590
 *
 */

static unsigned long target_ip = 0;
module_param(target_ip, ulong, 0644);

/* 
 * uid we want to spy on - will be filled from the command line. 
 * you can check your user uid by running `id -u` in the shell.
 */

static uid_t uid = -1;
module_param(uid, int, 0644);


static void notrace trace_sys_openat(unsigned long ip, unsigned long parent_ip,
                                     struct ftrace_ops *ops,
                                     struct ftrace_regs *fregs) {

  char fname[256] = {0};
  struct pt_regs * regs = ftrace_get_regs(fregs);

  if (__kuid_val(current_uid()) != uid) {
    return;
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

  struct pt_regs *syscall_regs = (struct pt_regs *)regs->di;
  const char __user *filename = (const char __user *)syscall_regs->si;

  if (strncpy_from_user(fname, filename, sizeof(fname) - 1) > 0) {
    pr_info("open %s by uid %d\n", fname, uid);
  }
}

// define ftrace_ops
static struct ftrace_ops fops = {
  .func = trace_sys_openat,
  .flags = FTRACE_OPS_FL_SAVE_REGS,
};


// module initialization
static int __init syscall_hook_init(void) {
  int ret;

  ret = ftrace_set_filter_ip(&fops, target_ip, 0, 0);
  if (ret) {
    pr_alert("ftrace_set_filter_ip failed: %d\n", ret);
    return ret;
  }

  ret = register_ftrace_function(&fops);
  if (ret) {
    pr_alert("register_ftrace_function failed: %d\n", ret);
    ftrace_set_filter_ip(&fops, target_ip, 1, 0);
    return ret;
  }

  pr_info("ftrace hook registered at __x64_sys_openat\n");
  return 0;
}

// module exit function
static void __exit syscall_hook_exit(void) {
  // detach the ftrace hook
  unregister_ftrace_function(&fops);
  pr_info("ftrace hook unregistered successfully\n");
}

module_init(syscall_hook_init);
module_exit(syscall_hook_exit);

MODULE_LICENSE("GPL");
