/* linux/arch/arm/plat-s5pc1xx/setup-fimc0.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * Base S5PC1XX FIMC controller 0 gpio configuration
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
#include <plat/gpio-bank-h2.h>
#include <plat/gpio-bank-h3.h>

struct platform_device; /* don't need the contents */

void s3c_fimc0_cfg_gpio(struct platform_device *dev)
{
	int i;
	s3c_gpio_cfgpin(S5PC1XX_GPE0(0), S5PC1XX_GPE0_0_CAM_A_PCLK);
	s3c_gpio_cfgpin(S5PC1XX_GPE0(1), S5PC1XX_GPE0_1_CAM_A_VSYNC);
	s3c_gpio_cfgpin(S5PC1XX_GPE0(2), S5PC1XX_GPE0_2_CAM_A_HREF);
	s3c_gpio_cfgpin(S5PC1XX_GPE0(3), S5PC1XX_GPE0_3_CAM_A_DATA_0);
	s3c_gpio_cfgpin(S5PC1XX_GPE0(4), S5PC1XX_GPE0_4_CAM_A_DATA_1);
	s3c_gpio_cfgpin(S5PC1XX_GPE0(5), S5PC1XX_GPE0_5_CAM_A_DATA_2);
	s3c_gpio_cfgpin(S5PC1XX_GPE0(6), S5PC1XX_GPE0_6_CAM_A_DATA_3);
	s3c_gpio_cfgpin(S5PC1XX_GPE0(7), S5PC1XX_GPE0_7_CAM_A_DATA_4);
	s3c_gpio_cfgpin(S5PC1XX_GPE1(0), S5PC1XX_GPE1_0_CAM_A_DATA_5);
	s3c_gpio_cfgpin(S5PC1XX_GPE1(1), S5PC1XX_GPE1_1_CAM_A_DATA_6);
	s3c_gpio_cfgpin(S5PC1XX_GPE1(2), S5PC1XX_GPE1_2_CAM_A_DATA_7);
	s3c_gpio_cfgpin(S5PC1XX_GPE1(3), S5PC1XX_GPE1_3_CAM_A_CLKOUT);
	s3c_gpio_cfgpin(S5PC1XX_GPE1(4), S5PC1XX_GPE1_4_CAM_A_RESET);
	s3c_gpio_cfgpin(S5PC1XX_GPE1(5), S5PC1XX_GPE1_5_CAM_A_FIELD);

	for (i = 0; i < 8; i++)
		s3c_gpio_setpull(S5PC1XX_GPE0(i), S3C_GPIO_PULL_UP);

	for (i = 0; i < 6; i++)
		s3c_gpio_setpull(S5PC1XX_GPE1(i), S3C_GPIO_PULL_UP);

	s3c_gpio_cfgpin(S5PC1XX_GPH2(0), S5PC1XX_GPH2_0_CAM_B_DATA_0);
	s3c_gpio_cfgpin(S5PC1XX_GPH2(1), S5PC1XX_GPH2_1_CAM_B_DATA_1);
	s3c_gpio_cfgpin(S5PC1XX_GPH2(2), S5PC1XX_GPH2_2_CAM_B_DATA_2);
	s3c_gpio_cfgpin(S5PC1XX_GPH2(3), S5PC1XX_GPH2_3_CAM_B_DATA_3);
	s3c_gpio_cfgpin(S5PC1XX_GPH2(4), S5PC1XX_GPH2_4_CAM_B_DATA_4);
	s3c_gpio_cfgpin(S5PC1XX_GPH2(5), S5PC1XX_GPH2_5_CAM_B_DATA_5);
	s3c_gpio_cfgpin(S5PC1XX_GPH2(6), S5PC1XX_GPH2_6_CAM_B_DATA_6);
	s3c_gpio_cfgpin(S5PC1XX_GPH2(7), S5PC1XX_GPH2_7_CAM_B_DATA_7);
	s3c_gpio_cfgpin(S5PC1XX_GPH3(0), S5PC1XX_GPH3_0_CAM_B_PCLK);
	s3c_gpio_cfgpin(S5PC1XX_GPH3(1), S5PC1XX_GPH3_1_CAM_B_VSYNC);
	s3c_gpio_cfgpin(S5PC1XX_GPH3(2), S5PC1XX_GPH3_2_CAM_B_HREF);
	s3c_gpio_cfgpin(S5PC1XX_GPH3(3), S5PC1XX_GPH3_3_CAM_B_FIELD);
	s3c_gpio_cfgpin(S5PC1XX_GPE1(3), S5PC1XX_GPE1_3_CAM_A_CLKOUT);

	for (i = 0; i < 8; i++)
		s3c_gpio_setpull(S5PC1XX_GPH2(i), S3C_GPIO_PULL_UP);

	for (i = 0; i < 4; i++)
		s3c_gpio_setpull(S5PC1XX_GPH3(i), S3C_GPIO_PULL_UP);
}

