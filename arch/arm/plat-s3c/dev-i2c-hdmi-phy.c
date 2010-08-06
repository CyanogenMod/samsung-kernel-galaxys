/* linux/arch/arm/plat-s3c/dev-i2c1.c
 *
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C series device definition for i2c device 1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <asm/irq.h>
#include <mach/map.h>

#include <plat/regs-iic.h>
#include <plat/iic.h>
#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s3c_i2c_resource[] = {
	[0] = {
		.start = S3C_PA_IIC_HDMI_PHY,
		.end   = S3C_PA_IIC_HDMI_PHY + SZ_4K  - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_IIC_HDMI_PHY,
		.end   = IRQ_IIC_HDMI_PHY,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_i2c3 = {
	.name		  = "s3c2410-i2c",
	.id		  = 3,
	.num_resources	  = ARRAY_SIZE(s3c_i2c_resource),
	.resource	  = s3c_i2c_resource,
};

static struct s3c2410_platform_i2c default_i2c_data3 __initdata = {
	.flags		= 0,
	.bus_num	= 3,
	.slave_addr	= 0x10,
	.bus_freq	= 100*1000,
	.max_freq	= 400*1000,
	.sda_delay	= S3C2410_IICLC_SDA_DELAY5 | S3C2410_IICLC_FILTER_ON,
};

void __init s3c_i2c3_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd)
		pd = &default_i2c_data3;

	npd = kmemdup(pd, sizeof(struct s3c2410_platform_i2c), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else if (!npd->cfg_gpio)
		npd->cfg_gpio = NULL;

	s3c_device_i2c3.dev.platform_data = npd;
}
