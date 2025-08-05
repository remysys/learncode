/*
 * ioctl.c
 */
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>

struct ioctl_arg {
  unsigned int val;
};

/* documentation/userspace-api/ioctl/ioctl-number.rst */
#define IOC_MAGIC '\x66'

#define IOCTL_VALSET _IOW(IOC_MAGIC, 0, struct ioctl_arg)
#define IOCTL_VALGET _IOR(IOC_MAGIC, 1, struct ioctl_arg)
#define IOCTL_VALGET_NUM _IOR(IOC_MAGIC, 2, int)
#define IOCTL_VALSET_NUM _IOW(IOC_MAGIC, 3, int)

#define IOCTL_VAL_MAXNR 3
#define DRIVER_NAME "ioctltest"

static struct class *device_class;
static unsigned int device_major = 0;
static unsigned int num_of_dev = 1;
static struct cdev chardev;
static int ioctl_num = 0;

struct ioctl_device_data {
  unsigned char val;
  rwlock_t lock;
};

static long device_ioctl(struct file *filp, unsigned int cmd,
                             unsigned long arg) {
  struct ioctl_device_data *ioctl_data = filp->private_data;
  int retval = 0;
  unsigned char val;
  struct ioctl_arg data;
  memset(&data, 0, sizeof(data));

  switch (cmd) {
  case IOCTL_VALSET:
    if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
      retval = -EFAULT;
      goto done;
    }

    pr_alert("ioctl set val:%x .\n", data.val);
    write_lock(&ioctl_data->lock);
    ioctl_data->val = data.val;
    write_unlock(&ioctl_data->lock);
    break;

  case IOCTL_VALGET:
    read_lock(&ioctl_data->lock);
    val = ioctl_data->val;
    read_unlock(&ioctl_data->lock);
    data.val = val;

    if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
      retval = -EFAULT;
      goto done;
    }

    break;

  case IOCTL_VALGET_NUM:
    retval = __put_user(ioctl_num, (int __user *)arg);
    break;

  case IOCTL_VALSET_NUM:
    ioctl_num = arg;
    break;

  default:
    retval = -ENOTTY;
  }

done:
  return retval;
}

static ssize_t device_read(struct file *filp, char __user *buf,
                               size_t count, loff_t *f_pos) {
  struct ioctl_device_data *ioctl_data = filp->private_data;
  unsigned char val;
  int retval;
  int i = 0;

  if (*f_pos > 0) {
    return 0;  // EOF
  }
  read_lock(&ioctl_data->lock);
  val = ioctl_data->val;
  read_unlock(&ioctl_data->lock);

  for (; i < count; i++) {
    if (copy_to_user(&buf[i], &val, 1)) {
      retval = -EFAULT;
      goto out;
    }
  }

  *f_pos += count;
  retval = count;
out:
  return retval;
}

static int device_close(struct inode *inode, struct file *filp) {
  pr_alert("%s call.\n", __func__);

  if (filp->private_data) {
    kfree(filp->private_data);
    filp->private_data = NULL;
  }

  return 0;
}

static int device_open(struct inode *inode, struct file *filp) {
  struct ioctl_device_data *ioctl_data;

  pr_alert("%s call.\n", __func__);
  ioctl_data = kmalloc(sizeof(struct ioctl_device_data), GFP_KERNEL);

  if (ioctl_data == NULL)
    return -ENOMEM;

  rwlock_init(&ioctl_data->lock);
  ioctl_data->val = 0xFF;
  filp->private_data = ioctl_data;

  return 0;
}

static struct file_operations fops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
  .owner = THIS_MODULE,
#endif
  .open = device_open,
  .release = device_close,
  .read = device_read,
  .unlocked_ioctl = device_ioctl,
};

static int __init ioctl_init(void) {
  dev_t dev;
  int alloc_ret = -1;
  int cdev_ret = -1;
  alloc_ret = alloc_chrdev_region(&dev, 0, num_of_dev, DRIVER_NAME);

  if (alloc_ret)
    goto error;

  device_major = MAJOR(dev);
  cdev_init(&chardev, &fops);
  cdev_ret = cdev_add(&chardev, dev, num_of_dev);

  if (cdev_ret)
    goto error;

  #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    device_class = class_create(DEVICE_NAME);
  #else
    device_class = class_create(THIS_MODULE, DRIVER_NAME);
  #endif

  if (device_class == NULL) {
    pr_alert("failed to create class for %s\n", DRIVER_NAME);
    goto error;
  }


  device_create(device_class, NULL, dev, NULL, DRIVER_NAME);
  

  pr_alert("%s driver(major: %d) installed.\n", DRIVER_NAME, device_major);
  return 0;
error:
  if (cdev_ret == 0)
    cdev_del(&chardev);
  if (alloc_ret == 0)
    unregister_chrdev_region(dev, num_of_dev);
  return -1;
}

static void __exit ioctl_exit(void) {
  dev_t dev = MKDEV(device_major, 0);

  device_destroy(device_class, dev);
  class_destroy(device_class);

  cdev_del(&chardev);
  unregister_chrdev_region(dev, num_of_dev);
  pr_alert("%s driver removed.\n", DRIVER_NAME);
}

module_init(ioctl_init);
module_exit(ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("this is test_ioctl module");