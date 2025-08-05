/*
 * chardev2.c - Create an input/output character device
 */

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h> /* specifically, a module */
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/version.h>

#include <asm/errno.h>

#define DEVICE_NAME "char_dev"
#define BUF_LEN 80

enum {
  CDEV_NOT_USED,
  CDEV_EXCLUSIVE_OPEN,
};

#define MAJOR_NUM 100

/* set the message of the device driver */
#define IOCTL_SET_MSG _IOW(MAJOR_NUM, 0, char *)

/* _IOW means that we are creating an ioctl command number for passing
 * information from a user process to the kernel module.
 *
 * the first arguments, MAJOR_NUM, is the major device number we are using.
 *
 * the second argument is the number of the command (there could be several
 * with different meanings).
 *
 * the third argument is the type we want to get from the process to the
 * kernel.
 */

/* get the message of the device driver */
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)
/* this IOCTL is used for output, to get the message of the device driver.
 * however, we still need the buffer to place the message in to be input,
 * as it is allocated by the process.
 */

/* get the n'th byte of the message */
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)
/* the IOCTL is used for both input and output. it receives from the user
 * a number, n, and returns message[n].
 */

/* the name of the device file */
#define DEVICE_FILE_NAME "char_dev"
#define DEVICE_PATH "/dev/char_dev"

/* is the device open right now? used to prevent concurrent access into
 * the same device
 */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

/* the message the device will give when asked */
static char message[BUF_LEN + 1];

static struct class *cls;

/* this is called whenever a process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file) {
  pr_info("device_open(%p)\n", file);

  try_module_get(THIS_MODULE);
  return 0;
}

static int device_release(struct inode *inode, struct file *file) {
  pr_info("device_release(%p,%p)\n", inode, file);

  module_put(THIS_MODULE);
  return 0;
}

/* this function is called whenever a process which has already opened the
 * device file attempts to read from it.
 */
static ssize_t device_read(struct file *file,     /* see include/linux/fs.h */
                           char __user *buffer,   /* buffer to be filled    */
                           size_t length,         /* length of the buffer   */
                           loff_t *offset) {
  /* number of bytes actually written to the buffer */
  int bytes_read = 0;
  /* how far did the process reading the message get? useful if the message
   * is larger than the size of the buffer we get to fill in device_read.
   */
  const char *message_ptr = message;

  if (!*(message_ptr + *offset)) { /* we are at the end of message */
    *offset = 0;                   /* reset the offset             */
    return 0;                      /* signify end of file          */
  }

  message_ptr += *offset;

    /* actually put the data into the buffer */
  while (length && *message_ptr) {
    /* because the buffer is in the user data segment, not the kernel
     * data segment, assignment would not work. instead, we have to
     * use put_user which copies data from the kernel data segment to
     * the user data segment.
     */
    put_user(*(message_ptr++), buffer++);
    length--;
    bytes_read++;
  }

  pr_info("read %d bytes, %ld left\n", bytes_read, length);

  *offset += bytes_read;

  /* read functions are supposed to return the number of bytes actually
   * inserted into the buffer.
   */
  return bytes_read;
}

/* called when somebody tries to write into our device file. */
static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
  int i;

  pr_info("device_write(%p,%p,%ld)", file, buffer, length);

  for (i = 0; i < length && i < BUF_LEN; i++)
    get_user(message[i], buffer + i);

  /* again, return the number of input characters used. */
  return i;
}

/* this function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * if the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 */
static long
device_ioctl(struct file *file,           /* ditto */
             unsigned int ioctl_num,      /* number and param for ioctl */
             unsigned long ioctl_param) {
  int i;
  long ret = 0;

  /* we don't want to talk to two processes at the same time. */
  if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
    return -EBUSY;

  /* switch according to the ioctl called */
  switch (ioctl_num) {
  case IOCTL_SET_MSG: {
    /* receive a pointer to a message (in user space) and set that to
      * be the device's message. Get the parameter given to ioctl by
      * the process.
      */
    char __user *tmp = (char __user *)ioctl_param;
    char ch;

    /* find the length of the message */
    get_user(ch, tmp);
    for (i = 0; ch && i < BUF_LEN; i++, tmp++)
      get_user(ch, tmp);

    device_write(file, (char __user *)ioctl_param, i, NULL);
    break;
  }
  case IOCTL_GET_MSG: {
    loff_t offset = 0;

    /* give the current message to the calling process - the parameter
     * we got is a pointer, fill it.
     */
    i = device_read(file, (char __user *)ioctl_param, 99, &offset);

    /* put a zero at the end of the buffer, so it will be properly
     * terminated.
     */
    put_user('\0', (char __user *)ioctl_param + i);
    break;
  }
  case IOCTL_GET_NTH_BYTE:
    /* this ioctl is both input (ioctl_param) and output (the return
     * value of this function).
     */
    ret = (long)message[ioctl_param];
    break;
  }

  /* we're now ready for our next caller */
  atomic_set(&already_open, CDEV_NOT_USED);

  return ret;
}

/* module declarations */

/* this structure will hold the functions to be called when a process does
 * something to the device we created. since a pointer to this structure
 * is kept in the devices table, it can't be local to init_module. NULL is
 * for unimplemented functions.
 */
static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .unlocked_ioctl = device_ioctl,
  .open = device_open,
  .release = device_release, /* a.k.a. close */
};

/* initialize the module - register the character device */
static int __init chardev2_init(void) {
  /* register the character device (at least try) */
  int ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);

  /* negative values signify an error */
  if (ret_val < 0) {
    pr_alert("%s failed with %d\n", "sorry, registering the character device ", ret_val);
    return ret_val;
  }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
  cls = class_create(DEVICE_FILE_NAME);
#else
  cls = class_create(THIS_MODULE, DEVICE_FILE_NAME);
#endif
  device_create(cls, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_FILE_NAME);

  pr_info("device created on /dev/%s\n", DEVICE_FILE_NAME);

  return 0;
}

/* cleanup - unregister the appropriate file from /proc */
static void __exit chardev2_exit(void) {
  device_destroy(cls, MKDEV(MAJOR_NUM, 0));
  class_destroy(cls);
  /* unregister the device */
  unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

module_init(chardev2_init);
module_exit(chardev2_exit);

MODULE_LICENSE("GPL");