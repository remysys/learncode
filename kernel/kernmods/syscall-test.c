#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/kprobes.h>

static unsigned long lookup_name(const char *name) {
	struct kprobe kp = {
		.symbol_name = name
	};
	unsigned long retval;

	if (register_kprobe(&kp) < 0) return 0;
	retval = (unsigned long) kp.addr;
	unregister_kprobe(&kp);
	return retval;
}

struct ftrace_hook {
	const char *name;
	void *function;
	void *original;

	unsigned long address;
	struct ftrace_ops ops;
};

static int fh_resolve_hook_address(struct ftrace_hook *hook) {
	hook->address = lookup_name(hook->name);

	if (!hook->address) {
		pr_debug("unresolved symbol: %s\n", hook->name);
		return -ENOENT;
	}

	*((unsigned long*) hook->original) = hook->address;
	return 0;
}

static void notrace fh_ftrace_thunk(unsigned long ip, unsigned long parent_ip,
		struct ftrace_ops *ops, struct ftrace_regs *fregs) {
	struct pt_regs *regs = ftrace_get_regs(fregs);
	struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
  
  const char __user *filename = (const char __user *)regs->di;

  char fname[256] = {0};
  if (strncpy_from_user(fname, filename, sizeof(fname) - 1) > 0) {
    pr_info("openat: %s\n",fname);
  } else {
    pr_info("openat [filename unreadable]\n");
  }
  
	if (!within_module(parent_ip, THIS_MODULE))
		regs->ip = (unsigned long)hook->function;
}

/**
 * fh_install_hooks() - register and enable a single hook
 * hook: a hook to install
 *
 * returns: zero on success, negative error code otherwise.
 */
int fh_install_hook(struct ftrace_hook *hook) {
	int err;

	err = fh_resolve_hook_address(hook);
	if (err)
		return err;

	
	hook->ops.func = fh_ftrace_thunk;
	hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS
	                | FTRACE_OPS_FL_RECURSION
	                | FTRACE_OPS_FL_IPMODIFY;

	err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
	if (err) {
		pr_debug("ftrace_set_filter_ip() failed: %d\n", err);
		return err;
	}

	err = register_ftrace_function(&hook->ops);
	if (err) {
		pr_debug("register_ftrace_function() failed: %d\n", err);
		ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
		return err;
	}

	return 0;
}

/*
 * fh_remove_hooks() - disable and unregister a single hook
 * hook: a hook to remove
 */
void fh_remove_hook(struct ftrace_hook *hook) {
	int err;

	err = unregister_ftrace_function(&hook->ops);
	if (err) {
		pr_debug("unregister_ftrace_function() failed: %d\n", err);
	}

	err = ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
	if (err) {
		pr_debug("ftrace_set_filter_ip() failed: %d\n", err);
	}
}

/*
 * fh_install_hooks() - register and enable multiple hooks
 * hooks: array of hooks to install
 * count: number of hooks to install
 *
 * if some hooks fail to install then all hooks will be removed.
 *
 * returns: zero on success, negative error code otherwise.
 */
int fh_install_hooks(struct ftrace_hook *hooks, size_t count)
{
	int err;
	size_t i;

	for (i = 0; i < count; i++) {
		err = fh_install_hook(&hooks[i]);
		if (err)
			goto error;
	}

	return 0;

error:
	while (i != 0) {
		fh_remove_hook(&hooks[--i]);
	}

	return err;
}

/*
 * fh_remove_hooks() - disable and unregister multiple hooks
 * hooks: array of hooks to remove
 * count: number of hooks to remove
 */
void fh_remove_hooks(struct ftrace_hook *hooks, size_t count) {
	size_t i;

	for (i = 0; i < count; i++)
		fh_remove_hook(&hooks[i]);
}


static asmlinkage long (*real_sys_clone)(struct pt_regs *regs);

static asmlinkage long fh_sys_clone(struct pt_regs *regs) {
	long ret;

	pr_info("clone() before\n");

	ret = real_sys_clone(regs);

	pr_info("clone() after: %ld\n", ret);

	return ret;
}


static char *duplicate_filename(const char __user *filename) {
	char *kernel_filename;

	kernel_filename = kmalloc(4096, GFP_KERNEL);
	if (!kernel_filename)
		return NULL;

	if (strncpy_from_user(kernel_filename, filename, 4096) < 0) {
		kfree(kernel_filename);
		return NULL;
	}

	return kernel_filename;
}


static asmlinkage long (*real_sys_execve)(struct pt_regs *regs);

static asmlinkage long fh_sys_execve(struct pt_regs *regs) {
	long ret;
	char *kernel_filename;

	kernel_filename = duplicate_filename((void*) regs->di);

	pr_info("execve() before: %s\n", kernel_filename);

	kfree(kernel_filename);

	ret = real_sys_execve(regs);

	pr_info("execve() after: %ld\n", ret);

	return ret;
}

#define SYSCALL_NAME(name) ("__x64_" name)

#define HOOK(_name, _function, _original)	\
{					\
  .name = SYSCALL_NAME(_name),	\
  .function = (_function),	\
  .original = (_original),	\
}

static struct ftrace_hook demo_hooks[] = {
	HOOK("sys_clone",  fh_sys_clone,  &real_sys_clone),
	HOOK("sys_execve", fh_sys_execve, &real_sys_execve),
};

static int syscall_hook_init(void) {
	int err;

	err = fh_install_hooks(demo_hooks, ARRAY_SIZE(demo_hooks));
	if (err)
		return err;

	pr_info("module loaded\n");

	return 0;
}


static void syscall_hook_exit(void) {
	fh_remove_hooks(demo_hooks, ARRAY_SIZE(demo_hooks));
	pr_info("module unloaded\n");
}

module_init(syscall_hook_init);
module_exit(syscall_hook_exit);

MODULE_DESCRIPTION("Example module hooking clone() and execve() via ftrace");
MODULE_LICENSE("GPL");