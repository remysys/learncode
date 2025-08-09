/*
 * intr.c - example of registering a real IRQ handler for keyboard IRQ
 *
 * this module registers an interrupt handler for IRQ 1 (keyboard IRQ on most PCs).
 * when a key is pressed or released, the interrupt handler is invoked by the kernel.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>

#define KEYBOARD_IRQ 1  // typical IRQ number for keyboard on x86 PCs
static char *dev_name = "keyboard_irq_example";


static irqreturn_t keyboard_irq_handler(int irq, void *dev_id) {
  static int count = 0;
  count++;
  pr_info("keyboard_irq_handler: IRQ %d triggered %d times\n", irq, count);
  return IRQ_HANDLED;
}

static int __init keyboard_irq_init(void) {
  int ret;

  pr_info("keyboard_irq_example: initializing\n");

  ret = request_irq(KEYBOARD_IRQ, keyboard_irq_handler,
                    IRQF_SHARED, dev_name, dev_name);
  if (ret) {
    pr_err("keyboard_irq_example: request_irq failed with %d\n", ret);
    return ret;
  }

  pr_info("keyboard_irq_example: IRQ handler registered for IRQ %d\n", KEYBOARD_IRQ);
  return 0;
}

static void __exit keyboard_irq_exit(void) {
  free_irq(KEYBOARD_IRQ, dev_name);
  pr_info("keyboard_irq_example: IRQ handler unregistered\n");
}

module_init(keyboard_irq_init);
module_exit(keyboard_irq_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("example module registering keyboard IRQ handler");
