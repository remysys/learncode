/*
 * tty.c - send output to the tty we're running on, regardless if
 * it is through X11, telnet, etc. we do this by printing the string to the
 * tty associated with the current task.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>  /* for current */
#include <linux/tty.h>    /* for the tty declarations */

static void tty_print_string(char *str) {
  /* the tty for the current task */
  struct tty_struct *current_tty  = get_current_tty();

  /* 
   * if current_tty  is NULL, the current task has no tty you can print to (i.e.,
   * if it is a daemon). if so, there is nothing we can do.
   */
  if (current_tty ) {
    const struct tty_operations *ttyops = current_tty ->driver->ops;
    /* 
     * current_tty ->driver is a struct which holds the tty's functions,
     * one of which (write) is used to write strings to the tty.
     * it can be used to take a string either from the user's or
     * kernel's memory segment.
     *
     * the function's 1st parameter is the tty to write to, because the
     * same function would normally be used for all tty's of a certain
     * type.
     * the 2nd parameter is a pointer to a string.
     * the 3rd parameter is the length of the string.
     *
     * as you will see below, sometimes it's necessary to use
     * preprocessor stuff to create code that works for different
     * kernel versions. the (naive) approach we've taken here does not
     * scale well. the right way to deal with this is described in
     * section 2 of
     * linux/Documentation/SubmittingPatches
     */
    (ttyops->write)(current_tty ,       /* the tty itself */
                    str,                /* string */
                    strlen(str));       /* length */

    /*
     * ttys were originally hardware devices, which (usually) strictly
     * followed the ASCII standard. in ASCII, to move to a new line you
     * need two characters, a carriage return and a line feed. on Unix,
     * the ASCII line feed is used for both purposes - so we can not
     * just use \n, because it would not have a carriage return and the
     * next line will start at the column right after the line feed.
     *
     * this is why text files are different between Unix and MS Windows.
     * in CP/M and derivatives, like MS-DOS and MS Windows, the ASCII
     * standard was strictly adhered to, and therefore a newline requires
     * both a LF and a CR.
     */
    (ttyops->write)(current_tty , "\015\012", 2);
  }
}

static int __init tty_print_init(void) {
  tty_print_string("the module has been inserted. hello world!");
  return 0;
}

static void __exit tty_print_exit(void) {
  tty_print_string("the module has been removed. farewell world!");
}

module_init(tty_print_init);
module_exit(tty_print_exit);

MODULE_LICENSE("GPL");