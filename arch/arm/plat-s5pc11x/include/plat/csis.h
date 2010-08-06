/* linux/arch/arm/plat-s5pc11x/include/plat/csis.h
 *
 * Platform header file for MIPI-CSI2 driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _CSIS_H
#define _CSIS_H

struct platform_device;

struct s3c_platform_csis {
	const char	srclk_name[16];
	const char	clk_name[16];
	unsigned long	clk_rate;

	void		(*cfg_gpio)(void);
	void		(*cfg_phy_global)(int on);
};

extern void s3c_csis_set_platdata(struct s3c_platform_csis *csis);
extern void s3c_csis_cfg_gpio(void);
extern void s3c_csis_cfg_phy_global(int on);

#endif /* _CSIS_H */

