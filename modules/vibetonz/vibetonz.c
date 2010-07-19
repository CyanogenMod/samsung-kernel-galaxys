/****************************************************************************
**
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2006-2015 ALL RIGHTS RESERVED
**
*****************************************************************************/
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <linux/delay.h>
#include <linux/timed_output.h>
#include <linux/pwm.h>

/*********** for debug **********************************************************/
#if 0 
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/
#define VIBRATOR_PERIOD	93500
#define VIBRATOR_DUTY	93400

#if !(defined CONFIG_JUPITER_VER_B4) 
#define VIBETONZ_EN		GPIO_VIBTONE_EN1
#else
#define VIBETONZ_EN		GPIO_HAPTIC_EN
#endif

#if !(defined CONFIG_JUPITER_VER_B4) 
#define VIBETONZ_PWM	1
#else
#define VIBETONZ_PWM	2
#endif

static struct hrtimer timer;

static int max_timeout = 5000;
static int vibrator_value = 0;

struct pwm_device	*vib_pwm;

static int set_vibetonz(int timeout)
{
	if(!timeout) {
		pwm_disable(vib_pwm);
		printk("[VIBETONZ] DISABLE\n");
		gpio_set_value(VIBETONZ_EN, GPIO_LEVEL_LOW);
		gpio_direction_input(VIBETONZ_EN);
		s3c_gpio_setpull(VIBETONZ_EN,S3C_GPIO_PULL_DOWN);
	}
	else {
		pwm_config(vib_pwm, VIBRATOR_DUTY, VIBRATOR_PERIOD);
		pwm_enable(vib_pwm);
		
		printk("[VIBETONZ] ENABLE\n");
		gpio_direction_output(VIBETONZ_EN, GPIO_LEVEL_LOW);
		mdelay(1);
		gpio_set_value(VIBETONZ_EN, GPIO_LEVEL_HIGH);
	}

	vibrator_value = timeout;
	
	return 0;
}

static enum hrtimer_restart vibetonz_timer_func(struct hrtimer *timer)
{
	//gprintk("[VIBETONZ] %s : \n",__func__);
	set_vibetonz(0);
	return HRTIMER_NORESTART;
}

static int get_time_for_vibetonz(struct timed_output_dev *dev)
{
	int remaining;

	if (hrtimer_active(&timer)) {
		ktime_t r = hrtimer_get_remaining(&timer);
		remaining = r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		remaining = 0;

	if (vibrator_value ==-1)
		remaining = -1;

	return remaining;

}

static void enable_vibetonz_from_user(struct timed_output_dev *dev,int value)
{
	printk("[VIBETONZ] %s : time = %d msec \n",__func__,value);
	hrtimer_cancel(&timer);
	
	set_vibetonz(value);
	vibrator_value = value;

	if (value > 0) 
	{
		if (value > max_timeout)
			value = max_timeout;

		hrtimer_start(&timer,
						ktime_set(value / 1000, (value % 1000) * 1000000),
						HRTIMER_MODE_REL);
		vibrator_value = 0;
	}
}

static struct timed_output_dev timed_output_vt = {
	.name     = "vibrator",
	.get_time = get_time_for_vibetonz,
	.enable   = enable_vibetonz_from_user,
};

static void vibetonz_start(void)
{
	int ret = 0;

	//printk("[VIBETONZ] %s : \n",__func__);

	/* hrtimer settings */
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = vibetonz_timer_func;

	gpio_direction_output(VIBETONZ_EN,0);
	mdelay(10);
	s3c_gpio_setpull(VIBETONZ_EN, S3C_GPIO_PULL_NONE);
    gpio_set_value(VIBETONZ_EN, GPIO_LEVEL_HIGH);

    vib_pwm = pwm_request(VIBETONZ_PWM, "vibtonz");
	
	/* timed_output_device settings */
	ret = timed_output_dev_register(&timed_output_vt);
	if(ret)
		printk(KERN_ERR "[VIBETONZ] timed_output_dev_register is fail \n");	
}


static void vibetonz_end(void)
{
	printk("[VIBETONZ] %s \n",__func__);
}

static void __init vibetonz_init(void)
{
	vibetonz_start();
}

static void __exit vibetonz_exit(void)
{
	vibetonz_end();
}

#ifndef CONFIG_JUPITER_VER_B4
module_init(vibetonz_init);
#endif
module_exit(vibetonz_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("vibetonz control interface");
