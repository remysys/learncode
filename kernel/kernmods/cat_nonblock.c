/*
 *  cat_nonblock.c - open a file and display its contents, but exit rather than
 *  wait for input.
 *  gcc cat_nonblock.c -o cat_nonblock
 */
#include <errno.h>    /* for errno */
#include <fcntl.h>    /* for open */
#include <stdio.h>    /* standard I/O */
#include <stdlib.h>   /* for exit */
#include <unistd.h>   /* for read */

#define MAX_BYTES 1024 * 4

int main(int argc, char *argv[]) {
  int fd;                 /* the file descriptor for the file to read */
  ssize_t bytes;           /* the number of bytes read */
  char buffer[MAX_BYTES]; /* the buffer for the bytes */

  /* usage */
  if (argc != 2) {
    printf("usage: %s <filename>\n", argv[0]);
    puts("reads the content of a file, but doesn't wait for input");
    exit(EXIT_FAILURE);
  }

  /* open the file for reading in non blocking mode */
  fd = open(argv[1], O_RDONLY | O_NONBLOCK);

  /* if open failed */
  if (fd == -1) {
    puts(errno == EAGAIN ? "open would block" : "open failed");
    exit(EXIT_FAILURE);
  }

  /* read the file and output its contents */
  do {
    /* read characters from the file */
    bytes = read(fd, buffer, MAX_BYTES);

    /* if there's an error, report it and die */
    if (bytes == -1) {
      if (errno == EAGAIN)
        puts("normally I'd block, but you told me not to");
      else
        puts("another read error");
      exit(EXIT_FAILURE);
    }

    /* print the characters */
    if (bytes > 0) {
      for (int i = 0; i < bytes; i++)
        putchar(buffer[i]);
    }

    /* while there are no errors and the file isn't over */
  } while (bytes > 0);

  close(fd);
  return 0;
}