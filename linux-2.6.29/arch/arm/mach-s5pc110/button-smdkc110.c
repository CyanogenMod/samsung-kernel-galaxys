#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <asm/gpio.h>

static irqreturn_t
s3c_button_interrupt(int irq, void *dev_id)
{
	printk("Button Interrupt occure\n");
	
	return IRQ_HANDLED;
}

static struct irqaction s3c_button_irq = {
	.name		= "s3c button Tick",
	.flags		= IRQF_SHARED ,
	.handler	= s3c_button_interrupt,
};

static unsigned int s3c_button_gpio_init(void)
{
	#if 0
	u32 err;

	err = gpio_request(S5PC11X_GPH0(4),"GPH0");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5PC11X_GPH0(4),S5PC11X_GPH0_4_EXT_INT30_4);
		s3c_gpio_setpull(S5PC11X_GPH0(4), S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(S5PC11X_GPH3(7),"GPH3");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5PC11X_GPH3(7),S5PC11X_GPH3_7_EXT_INT33_7);
		s3c_gpio_setpull(S5PC11X_GPH3(7), S3C_GPIO_PULL_NONE);
	}

	return err;
#else
	return true ;
#endif
}

static void __init s3c_button_init(void)
{
	u32 tmp;
	
	printk("SMDKC110 Button init function \n");

	if (s3c_button_gpio_init()) {
		printk(KERN_ERR "%s failed\n", __FUNCTION__);
		return;
	}	
	
	set_irq_type(IRQ_EINT4, IRQF_TRIGGER_FALLING);
	setup_irq(IRQ_EINT4, &s3c_button_irq);

	set_irq_type(IRQ_EINT(31), IRQF_TRIGGER_FALLING);
	setup_irq(IRQ_EINT(31), &s3c_button_irq);	
}

device_initcall(s3c_button_init);
