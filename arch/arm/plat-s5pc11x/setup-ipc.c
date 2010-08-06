/* linux/arch/arm/plat-s5pc11x/setup-ipc.c
 *
 * Copyright 2009 Samsung Electronics
 *	Youngmok Song <ym.song@samsung.com>
 *	http://samsungsemi.com/
 *
 * Base S5PC11X ipc gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <plat/map.h>
#include <plat/regs-clock.h>
#include <asm/io.h>

struct platform_device; /* don't need the contents */

void s3c_ipc_cfg_gpio(struct platform_device *dev)
{
	return;
}



