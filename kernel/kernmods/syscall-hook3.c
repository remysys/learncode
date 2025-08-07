#include <asm/ptrace.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ftrace.h>
#include <linux/sched.h>
#include <linux/uaccess.h>


static unsigned long target_ip = 0;
module_param(target_ip, ulong, 0644);

/* uid we want to spy on - will be filled from the command line. */
static uid_t uid = -1;
module_param(uid, int, 0644);


static void notrace trace_sys_openat(unsigned long ip, unsigned long parent_ip,
                                     struct ftrace_ops *ops,
                                     struct ftrace_regs *fregs) {

  struct pt_regs * regs = ftrace_get_regs(fregs);

  if (__kuid_val(current_uid()) != uid) {
    return;
  }

  const char __user *filename = (const char __user *)regs->si;

  char fname[256] = {0};
  if (strncpy_from_user(fname, filename, sizeof(fname) - 1) > 0) {
    pr_info("openat by uid %d: %s\n", uid, fname);
  } else {
    pr_info("openat by uid %d: [filename unreadable]\n", uid);
  }

}

// define ftrace_ops
static struct ftrace_ops ftrace_ops_sys_openat = {
  .func = trace_sys_openat,
  .flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION | FTRACE_OPS_FL_IPMODIFY,
};


// module initialization
static int __init syscall_hook_init(void) {
  int ret;

  ret = ftrace_set_filter_ip(&ftrace_ops_sys_openat, target_ip, 0, 0);
  if (ret) {
    pr_alert("ftrace_set_filter_ip failed: %d\n", ret);
    return ret;
  }

  ret = register_ftrace_function(&ftrace_ops_sys_openat);
  if (ret) {
    pr_alert("register_ftrace_function failed: %d\n", ret);
    ftrace_set_filter_ip(&ftrace_ops_sys_openat, target_ip, 1, 0);
    return ret;
  }

  pr_info("ftrace hook registered at __x64_sys_openat\n");
  return 0;
}

// module exit function
static void __exit syscall_hook_exit(void) {
  // detach the ftrace hook
  unregister_ftrace_function(&ftrace_ops_sys_openat);
  pr_info("ftrace hook unregistered successfully\n");
}

module_init(syscall_hook_init);
module_exit(syscall_hook_exit);

MODULE_LICENSE("GPL");
