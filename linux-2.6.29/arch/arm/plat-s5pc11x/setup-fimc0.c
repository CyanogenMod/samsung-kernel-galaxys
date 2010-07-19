/* linux/arch/arm/plat-s5pc11x/setup-fimc0.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * Base S5PC11X FIMC controller 0 gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-e0.h>
#include <plat/gpio-bank-e1.h>
#include <plat/gpio-bank-j0.h>
#include <plat/gpio-bank-j1.h>
#include <asm/io.h>
#include <mach/map.h>

struct platform_device; /* don't need the contents */

void s3c_fimc0_cfg_gpio(struct platform_device *dev)
{
	int i;

	s3c_gpio_cfgpin(S5PC11X_GPE0(0), S5PC11X_GPE0_0_CAM_A_PCLK);
	s3c_gpio_cfgpin(S5PC11X_GPE0(1), S5PC11X_GPE0_1_CAM_A_VSYNC);
	s3c_gpio_cfgpin(S5PC11X_GPE0(2), S5PC11X_GPE0_2_CAM_A_HREF);
	s3c_gpio_cfgpin(S5PC11X_GPE0(3), S5PC11X_GPE0_3_CAM_A_DATA_0);
	s3c_gpio_cfgpin(S5PC11X_GPE0(4), S5PC11X_GPE0_4_CAM_A_DATA_1);
	s3c_gpio_cfgpin(S5PC11X_GPE0(5), S5PC11X_GPE0_5_CAM_A_DATA_2);
	s3c_gpio_cfgpin(S5PC11X_GPE0(6), S5PC11X_GPE0_6_CAM_A_DATA_3);
	s3c_gpio_cfgpin(S5PC11X_GPE0(7), S5PC11X_GPE0_7_CAM_A_DATA_4);
	s3c_gpio_cfgpin(S5PC11X_GPE1(0), S5PC11X_GPE1_0_CAM_A_DATA_5);
	s3c_gpio_cfgpin(S5PC11X_GPE1(1), S5PC11X_GPE1_1_CAM_A_DATA_6);
	s3c_gpio_cfgpin(S5PC11X_GPE1(2), S5PC11X_GPE1_2_CAM_A_DATA_7);

#ifdef CONFIG_MACH_SMDKC110
	/* In Jupiter, MCLK (GPE1_3) is enabled in the sensor power on function */
	s3c_gpio_cfgpin(S5PC11X_GPE1(3), S5PC11X_GPE1_3_CAM_A_CLKOUT);
#endif
	for (i = 0; i < 8; i++)
		s3c_gpio_setpull(S5PC11X_GPE0(i), S3C_GPIO_PULL_NONE);

	for (i = 0; i < 4; i++)
		s3c_gpio_setpull(S5PC11X_GPE1(i), S3C_GPIO_PULL_NONE);

#ifdef CONFIG_MACH_SMDKC110
	s3c_gpio_cfgpin(S5PC11X_GPE1(4), S5PC11X_GPE1_4_CAM_A_FIELD);
	s3c_gpio_setpull(S5PC11X_GPE1(4), S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(S5PC11X_GPJ0(0), S5PC11X_GPJ0_0_CAM_B_DATA_0);
	s3c_gpio_cfgpin(S5PC11X_GPJ0(1), S5PC11X_GPJ0_1_CAM_B_DATA_1);
	s3c_gpio_cfgpin(S5PC11X_GPJ0(2), S5PC11X_GPJ0_2_CAM_B_DATA_2);
	s3c_gpio_cfgpin(S5PC11X_GPJ0(3), S5PC11X_GPJ0_3_CAM_B_DATA_3);
	s3c_gpio_cfgpin(S5PC11X_GPJ0(4), S5PC11X_GPJ0_4_CAM_B_DATA_4);
	s3c_gpio_cfgpin(S5PC11X_GPJ0(5), S5PC11X_GPJ0_5_CAM_B_DATA_5);
	s3c_gpio_cfgpin(S5PC11X_GPJ0(6), S5PC11X_GPJ0_6_CAM_B_DATA_6);
	s3c_gpio_cfgpin(S5PC11X_GPJ0(7), S5PC11X_GPJ0_7_CAM_B_DATA_7);
	s3c_gpio_cfgpin(S5PC11X_GPJ1(0), S5PC11X_GPJ1_0_CAM_B_PCLK);
	s3c_gpio_cfgpin(S5PC11X_GPJ1(1), S5PC11X_GPJ1_1_CAM_B_VSYNC);
	s3c_gpio_cfgpin(S5PC11X_GPJ1(2), S5PC11X_GPJ1_2_CAM_B_HREF);
	s3c_gpio_cfgpin(S5PC11X_GPJ1(3), S5PC11X_GPJ1_3_CAM_B_FIELD);
	s3c_gpio_cfgpin(S5PC11X_GPJ1(4), S5PC11X_GPJ1_4_CAM_B_CLKOUT);

	for (i = 0; i < 8; i++)
		s3c_gpio_setpull(S5PC11X_GPJ0(i), S3C_GPIO_PULL_NONE);

	for (i = 0; i < 5; i++)
		s3c_gpio_setpull(S5PC11X_GPJ1(i), S3C_GPIO_PULL_NONE);

	/* drive strength to max */
	writel(0xc0, S5PC11X_VA_GPIO + 0x10c);
	writel(0x300, S5PC11X_VA_GPIO + 0x26c);
#endif
}

