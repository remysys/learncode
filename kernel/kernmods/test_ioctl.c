/*
 * gcc test_ioctl.c -o test_ioctl
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// must match kernel driver definitions
#define IOC_MAGIC 0x66

#define IOCTL_VALSET      _IOW(IOC_MAGIC, 0, struct ioctl_arg)
#define IOCTL_VALGET      _IOR(IOC_MAGIC, 1, struct ioctl_arg)
#define IOCTL_VALGET_NUM  _IOR(IOC_MAGIC, 2, int)
#define IOCTL_VALSET_NUM  _IOW(IOC_MAGIC, 3, int)

struct ioctl_arg {
  unsigned int val;
};

int main() {
  int fd;
  struct ioctl_arg data;
  int num;

  // open the device file
  fd = open("/dev/ioctltest", O_RDWR);
  if (fd < 0) {
    perror("failed to open /dev/ioctltest");
    return 1;
  }

  // set data.val to 0xAB
  data.val = 0xAB;
  if (ioctl(fd, IOCTL_VALSET, &data) == -1) {
    perror("ioctl IOCTL_VALSET failed");
  }

  // get val back from kernel
  memset(&data, 0, sizeof(data));
  if (ioctl(fd, IOCTL_VALGET, &data) == -1) {
    perror("ioctl IOCTL_VALGET failed");
  } else {
    printf("IOCTL_VALGET returned: 0x%X\n", data.val);
  }

  // set an integer via IOCTL
  num = 42;
  if (ioctl(fd, IOCTL_VALSET_NUM, num) == -1) {
    perror("ioctl IOCTL_VALSET_NUM failed");
  }

  // get the integer back
  if (ioctl(fd, IOCTL_VALGET_NUM, &num) == -1) {
    perror("ioctl IOCTL_VALGET_NUM failed");
  } else {
    printf("IOCTL_VALGET_NUM returned: %d\n", num);
  }

  // read from the device
  char buf[10];
  ssize_t r = read(fd, buf, sizeof(buf));
  if (r < 0) {
    perror("read");
  } else {
    printf("read %ld bytes: ", r);
    for (int i = 0; i < r; i++) {
      printf("0x%02x ", (unsigned char)buf[i]);
    }
    printf("\n");
  }

  // close device
  close(fd);
  return 0;
}
