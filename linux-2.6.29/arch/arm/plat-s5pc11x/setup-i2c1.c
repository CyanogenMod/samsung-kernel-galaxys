/* linux/arch/arm/plat-s5pc11x/setup-i2c0.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Base S5PC11X I2C bus 0 gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>

struct platform_device; /* don't need the contents */

#include <mach/gpio.h>
#include <plat/iic.h>
#include <plat/gpio-bank-d1.h>
#include <plat/gpio-cfg.h>

void s3c_i2c1_cfg_gpio(struct platform_device *dev)
{
	s3c_gpio_cfgpin(S5PC11X_GPD1(2), S5PC11X_GPD_1_2_I2C1_SDA);
	s3c_gpio_cfgpin(S5PC11X_GPD1(3), S5PC11X_GPD_1_3_I2C1_SCL);
	s3c_gpio_setpull(S5PC11X_GPD1(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPD1(3), S3C_GPIO_PULL_NONE);
}
