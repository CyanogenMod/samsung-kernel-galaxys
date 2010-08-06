/* linux/arch/arm/plat-s5pc11x/dev-ipc.c
 *
 * Copyright 2009 Samsung Electronics
 *	Youngmok Song <ym.song@samsung.com>
 *	http://samsungsemi.com/
 *
 * S5PC1XX series device definition for IPC
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

#include <plat/fimc-ipc.h>
#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s3c_ipc_resource[] = {
	[0] = {
		.start = S5PC11X_PA_IPC,
		.end   = S5PC11X_PA_IPC + S5PC11X_SZ_IPC - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device s3c_device_ipc = {
	.name		  = "s3c-ipc",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s3c_ipc_resource),
	.resource	  = s3c_ipc_resource,
};

static struct s3c_platform_ipc default_ipc_data __initdata = {
	.clk_name = "ipc",
};

void __init s3c_ipc_set_platdata(struct s3c_platform_ipc *pd)
{
	struct s3c_platform_ipc *npd;

	if (!pd)
		pd = &default_ipc_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_ipc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	npd->cfg_gpio = s3c_ipc_cfg_gpio;

	s3c_device_ipc.dev.platform_data = npd;
}


