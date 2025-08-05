
/*
 * gcc test_chardev2.c -o test_chardev2  
 * test_chardev2.c - the process to use ioctl's to control the kernel module
 *
 *  until now we could have used cat for input and output.  but now
 *  we need to do ioctl's, which require writing our own process. 
 */



#include <stdio.h>      /* standard I/O */
#include <fcntl.h>      /* open         */
#include <unistd.h>     /* close        */
#include <stdlib.h>     /* exit         */
#include <sys/ioctl.h>  /* ioctl        */

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


/* functions for the ioctl calls */

int ioctl_set_msg(int file_desc, char *message) {
  int ret_val;

  ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);

  if (ret_val < 0) {
    printf("ioctl_set_msg failed:%d\n", ret_val);
  }

  return ret_val;
}

int ioctl_get_msg(int file_desc) {
  int ret_val;
  char message[100] = { 0 };

  /* warning - this is dangerous because we don't tell 
   * the kernel how far it's allowed to write, so it 
   * might overflow the buffer. in a real production 
   * program, we would have used two ioctls - one to tell
   * the kernel the buffer length and another to give 
   * it the buffer to fill
   */
  ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

  if (ret_val < 0) {
    printf("ioctl_get_msg failed:%d\n", ret_val);
  }
  printf("get_msg message:%s", message);

  return ret_val;
}

int ioctl_get_nth_byte(int file_desc) {
  int i, c;
  printf("get_nth_byte message:");

  i = 0;
  do {
    c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);

    if (c < 0) {
      printf("\nioctl_get_nth_byte failed at the %d'th byte:\n", i);
      return c;
    }

    putchar(c);
  } while (c != 0);

  return 0;
}

/* main - call the ioctl functions */
int main(void) {
  int file_desc, ret_val;
  char *msg = "message passed by ioctl\n";

  file_desc = open(DEVICE_PATH, O_RDWR);
  if (file_desc < 0) {
    printf("can't open device file: %s, error:%d\n", DEVICE_PATH, file_desc);
    exit(EXIT_FAILURE);
  }

  ret_val = ioctl_set_msg(file_desc, msg);
  if (ret_val)
      goto error;
  ret_val = ioctl_get_nth_byte(file_desc);
  if (ret_val)
      goto error;
  ret_val = ioctl_get_msg(file_desc);
  if (ret_val)
      goto error;

  close(file_desc);
  return 0;
error:
  close(file_desc);
  exit(EXIT_FAILURE);
}