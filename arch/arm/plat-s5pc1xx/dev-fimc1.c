/* linux/arch/arm/plat-s5pc1xx/dev-fimc1.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * S5PC1XX series device definition for fimc device 1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <mach/map.h>
#include <asm/irq.h>
#include <plat/fimc.h>
#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s3c_fimc_resource[] = {
	[0] = {
		.start = S5PC1XX_PA_FIMC1,
		.end   = S5PC1XX_PA_FIMC1 + SZ_1M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_FIMC1,
		.end   = IRQ_FIMC1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc1 = {
	.name		  = "s3c-fimc",
	.id		  = 1,
	.num_resources	  = ARRAY_SIZE(s3c_fimc_resource),
	.resource	  = s3c_fimc_resource,
};

static struct s3c_platform_fimc default_fimc1_data __initdata = {
	.srclk_name	= "dout_mpll",
	.clk_name	= "sclk_fimc",
	.clk_rate	= 133000000,
	.default_cam	= CAMERA_PAR_A,
};

void __init s3c_fimc1_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_fimc1_cfg_gpio;

	s3c_device_fimc1.dev.platform_data = npd;
}

