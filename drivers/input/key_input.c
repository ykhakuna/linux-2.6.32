#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/errno.h> /* for -EBUSY */
#include <linux/ioport.h> /* for request_region */
#include <linux/delay.h> /* for loops_per_jiffy */
#include <asm/io.h>   /* for inb_p, outb_p, inb, outb, etc. */
#include <asm/uaccess.h> /* for get_user, etc. */
#include <linux/wait.h>  /* for wait_queue */
#include <linux/init.h>  /* for __init, module_{init,exit} */
#include <linux/poll.h>  /* for POLLIN, etc. */
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <asm/gpio.h>
#include <mach/regs-gpio.h>
#include <linux/input.h>

struct pin_desc {
    int     irq;
    int     pin;
    int     pin_state;
    char    *name;
    int     key_val;
    int     pin_val;
};

static struct pin_desc buttons_pin[] = {
    {IRQ_EINT8,    S3C2410_GPG(0),   S3C2410_GPG0_EINT8,    "KEY1",   BTN_0},
    {IRQ_EINT11,   S3C2410_GPG(3),   S3C2410_GPG3_EINT11,   "KEY3",   BTN_1},
    {IRQ_EINT13,   S3C2410_GPG(5),   S3C2410_GPG5_EINT13,   "KEY2",   BTN_2},
    {IRQ_EINT14,   S3C2410_GPG(6),   S3C2410_GPG6_EINT14,   "KEY4",   BTN_3},
    {IRQ_EINT15,   S3C2410_GPG(7),   S3C2410_GPG7_EINT15,   "KEY5",   BTN_4},
    {IRQ_EINT19,   S3C2410_GPG(11),  S3C2410_GPG11_EINT19,  "KEY6",   BTN_5},
};

static struct input_dev *buttons_input;

static irqreturn_t buttons_interrupt(int irq,void *dev_id)
{
    struct pin_desc *pin_desc = (struct pin_desc *)dev_id;
    int val;
    val = s3c2410_gpio_getpin(pin_desc->pin);

    input_event(buttons_input,EV_KEY,pin_desc->key_val, !val);
//input_report_key(buttons_input,pin_desc->key_val, !val);
    input_sync(buttons_input);

    return IRQ_HANDLED;
}

static int __init buttons_init(void)
{
    int error;
    int i;

    buttons_input = input_allocate_device();
    if(!buttons_input)
        return  -ENOMEM;

#if 0
    buttons_input->evbit[0] = BIT(EV_KEY);

    buttons_input->keybit[BITS_TO_LONGS(KEY_L)] = BIT(KEY_L);
    buttons_input->keybit[BITS_TO_LONGS(KEY_S)] = BIT(KEY_S);
    buttons_input->keybit[BITS_TO_LONGS(KEY_ENTER)] = BIT(KEY_ENTER);
    buttons_input->keybit[BITS_TO_LONGS(KEY_LEFTSHIFT)] = BIT(KEY_LEFTSHIFT);
#endif

    set_bit(EV_KEY,buttons_input->evbit);
//@evbit: bitmap of types of events supported by the device (EV_KEY, *EV_REL, etc.)

    for(i = 0;i<sizeof(buttons_pin)/sizeof(buttons_pin[0]);i++)
    {
        set_bit(buttons_pin[i].key_val,buttons_input->keybit);
	//@keybit bitmap of keys/buttons this device has
    }

    error = input_register_device(buttons_input);
    if(error){
        input_free_device(buttons_input);
        return -1;
    }
   
    for(i = 0;i<sizeof(buttons_pin)/sizeof(buttons_pin[0]);i++)
    {
       s3c2410_gpio_cfgpin(buttons_pin[i].pin,buttons_pin[i].pin_state);
       request_irq(buttons_pin[i].irq,buttons_interrupt, IRQF_SHARED | IRQF_TRIGGER_RISING
               | IRQF_TRIGGER_FALLING,buttons_pin[i].name,(void *)&buttons_pin[i]);
    }
   
    return 0;

}
static void __exit buttons_exit(void)
{
    int i;
   
    for(i = 0; i< sizeof(buttons_pin)/sizeof(buttons_pin[0]); i++)
    {
        free_irq(buttons_pin[i].irq, (void *)&buttons_pin[i]);
    }

    input_unregister_device(buttons_input);
    input_free_device(buttons_input);
}

module_init(buttons_init);
module_exit(buttons_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("RopenYuan");
