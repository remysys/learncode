/*
 * livepatch-uptime.c - Kernel Live Patching Sample Module
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/livepatch.h>

/*
 * this (dumb) live patch overrides the function that prints the uptime
 *
 * example:
 *
 * $ uptime -p
 * up 3 hours, 46 minutes
 *
 * $ insmod livepatch-uptime.ko
 * $ uptime -p
 * up 1 decade, 4 hours, 13 minutes
 *
 * $ echo 0 > /sys/kernel/livepatch/livepatch_uptime/enabled
 * $ uptime -p
 * up 3 hours, 48 minutes
 */


#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/time_namespace.h>

static int livepatch_uptime_proc_show(struct seq_file *m, void *v)
{
  struct timespec64 uptime;

  ktime_get_boottime_ts64(&uptime);
  timens_add_boottime(&uptime);

  uptime.tv_sec += 315360000; // decade in seconds
  seq_printf(m, "%lu.%02lu %lu.%02lu\n",
                  (unsigned long) uptime.tv_sec,
                  (uptime.tv_nsec / (NSEC_PER_SEC / 100)), 0lu, 0lu);
      return 0;
}

static struct klp_func funcs[] = {
  {
    .old_name = "uptime_proc_show",
    .new_func = livepatch_uptime_proc_show,
  }, { }
};

static struct klp_object objs[] = {
  {
    /* name being NULL means vmlinux */
    .funcs = funcs,
  }, { }
};

static struct klp_patch patch = {
  .mod = THIS_MODULE,
  .objs = objs,
};

static int livepatch_init(void)
{
  return klp_enable_patch(&patch);
}

static void livepatch_exit(void)
{
}

module_init(livepatch_init);
module_exit(livepatch_exit);
MODULE_LICENSE("GPL");
MODULE_INFO(livepatch, "Y");
MODULE_DESCRIPTION("livepatch sample module");