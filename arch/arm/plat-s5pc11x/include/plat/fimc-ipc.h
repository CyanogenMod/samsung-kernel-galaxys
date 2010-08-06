/* linux/arch/arm/plat-s5pc11x/include/plat/ipc.h
 *
 * Platform header file for IPC driver
 *
 * Youngmok Song, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _IPC_H
#define _IPC_H

struct platform_device;

struct s3c_platform_ipc {
	const char			srclk_name[16];		/* source of interface clock name */
	const char			clk_name[16];		/* interface clock name */
	u32				clk_rate;		/* clockrate for interface clock */	

	void				(*cfg_gpio)(struct platform_device *dev);
};

extern void s3c_ipc_set_platdata(struct s3c_platform_ipc *ipc);
extern void s3c_ipc_cfg_gpio(struct platform_device *dev);

#endif /* _IPC_H */
