/*
 * sleep.c - create a /proc file, and if several processes try to open it
 * at the same time, put all but one to sleep.
 */

#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/kernel.h>   /* for sprintf() */
#include <linux/module.h>   /* specifically, a module */
#include <linux/printk.h>
#include <linux/proc_fs.h>  /* necessary because we use proc fs */
#include <linux/types.h>
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/version.h>
#include <linux/wait.h>     /* for putting processes to sleep and waking them up */

#include <asm/current.h>
#include <asm/errno.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

/* here we keep the last message received, to prove that we can process our input. */
#define MESSAGE_LENGTH 80
static char message[MESSAGE_LENGTH];

static struct proc_dir_entry *p_entry;
#define PROC_ENTRY_FILENAME "sleep"

/* 
 * since we use the file operations struct, we can't use the special proc
 * output provisions - we have to use a standard read function, which is this
 * function.
 */
static ssize_t procfs_read(struct file *file,     /* see include/linux/fs.h   */
                             char __user *buf,    /* the buffer to put data to (in the user segment)    */
                             size_t len,          /* the length of the buffer */
                             loff_t *offset) {
  static int finished = 0;
  int i;
  char output_msg[MESSAGE_LENGTH + 30];

  /* 
   * return 0 to signify end of file - that we have nothing more to say
   * at this point.
   */
  if (finished) {
    finished = 0;
    return 0;
  }

  sprintf(output_msg, "last input:%s\n", message);
  for (i = 0; i < len && output_msg[i]; i++)
    put_user(output_msg[i], buf + i);

  finished = 1;
  return i; /* return the number of bytes "read" */
}

/* this function receives input from the user when the user writes to the /proc file. */

static ssize_t procfs_write(struct file *file,        /* the file itself          */
                            const char __user *buf,   /* the buffer with input    */
                            size_t length,            /* the buffer's length      */
                            loff_t *offset) {         /* offset to file - ignore  */ 
  int i;

  /* put the input into message, where procfs_read will later be able
   * to use it.
   */
  for (i = 0; i < MESSAGE_LENGTH - 1 && i < length; i++)
    get_user(message[i], buf + i);
  /* we want a standard, zero terminated string */
  message[i] = '\0';

  /* we need to return the number of input characters used */
  return i;
}

/* 1 if the file is currently open by somebody */
static atomic_t already_open = ATOMIC_INIT(0);

/* queue of processes who want our file */
static DECLARE_WAIT_QUEUE_HEAD(waitq);

/* called when the /proc file is opened */
static int procfs_open(struct inode *inode, struct file *file) {
  /* try to get without blocking  */
  if (!atomic_cmpxchg(&already_open, 0, 1)) {
    /* success without blocking, allow the access */
    return 0;
  }

  /* 
   * if the file's flags include O_NONBLOCK, it means the process does not
   * want to wait for the file. in this case, because the file is already open,
   * we should fail with -EAGAIN, meaning "you will have to try again",
   * instead of blocking a process which would rather stay awake.
   */
  if (file->f_flags & O_NONBLOCK)
    return -EAGAIN;

  while (atomic_cmpxchg(&already_open, 0, 1)) {
    int i, is_sig = 0;

    /* 
     * this function puts the current process, including any system
     * calls, such as us, to sleep.  execution will be resumed right
     * after the function call, either because somebody called
     * wake_up(&waitq) (only procfs_close does that, when the file
     * is closed) or when a signal, such as Ctrl-C, is sent
     * to the process
     */
    wait_event_interruptible(waitq, !atomic_read(&already_open));

    /* 
     * if we woke up because we got a signal we're not blocking,
     * return -EINTR (fail the system call).  this allows processes
     * to be killed or stopped.
     */
    for (i = 0; i < _NSIG_WORDS && !is_sig; i++)
      is_sig = current->pending.signal.sig[i] & ~current->blocked.sig[i];

    if (is_sig) {
      /* return -EINTR if we got a signal */
      return -EINTR;
    }
  }

  return 0; /* allow the access */
}

/* called when the /proc file is closed */
static int procfs_close(struct inode *inode, struct file *file) {
  /* 
   * set already_open to zero, so one of the processes in the waitq will
   * be able to set already_open back to one and to open the file. all
   * the other processes will be called when already_open is back to one,
   * so they'll go back to sleep.
   */
  atomic_set(&already_open, 0);

  /* 
   * wake up all the processes in waitq, so if anybody is waiting for the
   * file, they can have it.
   */
  wake_up(&waitq);

  return 0; /* success */
}

/* 
 * file operations for our /proc file. this is where we place pointers to all
 * the functions called when somebody tries to do something to our file. NULL
 * means we don't want to deal with something.
 */
#ifdef HAVE_PROC_OPS
static const struct proc_ops ops = {
  .proc_read = procfs_read,     /* "read" from the file */
  .proc_write = procfs_write,   /* "write" to the file */
  .proc_open = procfs_open,     /* called when the /proc file is opened */
  .proc_release = procfs_close, /* called when it's closed */
  .proc_lseek = noop_llseek,    /* return file->f_pos */
};
#else
static const struct file_operations ops = {
  .read = procfs_read,
  .write = procfs_write,
  .open = procfs_open,
  .release = procfs_close,
  .llseek = noop_llseek,
};
#endif

/* initialize the module - register the /proc file */
static int __init sleep_init(void) {
  p_entry =
    proc_create(PROC_ENTRY_FILENAME, 0644, NULL, &ops);
  
  if (p_entry == NULL) {
    pr_debug("error: could not initialize /proc/%s\n", PROC_ENTRY_FILENAME);
    return -ENOMEM;
  }

  proc_set_size(p_entry, 80);
  proc_set_user(p_entry, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID);

  pr_info("/proc/%s created\n", PROC_ENTRY_FILENAME);

  return 0;
}

/* 
 * cleanup - unregister our file from /proc.  this could get dangerous if
 * there are still processes waiting in waitq, because they are inside our
 * open function, which will get unloaded. I'll explain how to avoid removal
 * of a kernel module in such a case in chapter 10.
 */
static void __exit sleep_exit(void) {
  remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
  pr_debug("/proc/%s removed\n", PROC_ENTRY_FILENAME);
}

module_init(sleep_init);
module_exit(sleep_exit);

MODULE_LICENSE("GPL");