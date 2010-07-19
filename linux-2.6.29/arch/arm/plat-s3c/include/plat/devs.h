/* linux/include/asm-arm/plat-s3c24xx/devs.h
 *
 * Copyright (c) 2004 Simtec Electronics
 * Ben Dooks <ben@simtec.co.uk>
 *
 * Header file for s3c2410 standard platform devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/platform_device.h>

struct s3c24xx_uart_resources {
	struct resource		*resources;
	unsigned long		 nr_resources;
};

extern struct s3c24xx_uart_resources s3c2410_uart_resources[];
extern struct s3c24xx_uart_resources s3c64xx_uart_resources[];
extern struct s3c24xx_uart_resources s5p64xx_uart_resources[];
extern struct s3c24xx_uart_resources s5pc1xx_uart_resources[];
extern struct s3c24xx_uart_resources s5pc11x_uart_resources[];

extern struct platform_device *s3c24xx_uart_devs[];
extern struct platform_device *s3c24xx_uart_src[];

extern struct platform_device s3c_device_timer[];

extern struct platform_device s3c_device_fb;
extern struct platform_device s3c_device_usb;
extern struct platform_device s3c_device_lcd;
extern struct platform_device s3c_device_g2d;
extern struct platform_device s3c_device_g3d;
extern struct platform_device s3c_device_vpp;
extern struct platform_device s3c_device_tvenc;
extern struct platform_device s3c_device_tvscaler;
extern struct platform_device s3c_device_rotator;
extern struct platform_device s3c_device_jpeg;
extern struct platform_device s3c_device_wdt;
extern struct platform_device s3c_device_i2c0;
extern struct platform_device s3c_device_i2c1;
extern struct platform_device s3c_device_i2c2;
extern struct platform_device s3c_device_i2c3;
extern struct platform_device s3c_device_iis;
extern struct platform_device s3c_device_rtc;
extern struct platform_device s3c_device_adc;
extern struct platform_device s3c_device_sdi;
extern struct platform_device s3c_device_hsmmc0;
extern struct platform_device s3c_device_hsmmc1;
extern struct platform_device s3c_device_hsmmc2;
extern struct platform_device s3c_device_hsmmc3;

extern struct platform_device s3c_device_tsi;

#ifdef CONFIG_MACH_S5PC110_JUPITER
extern struct platform_device s5p_trs_detect;
#endif
extern struct platform_device s3c_device_spi0;
extern struct platform_device s3c_device_spi1;
extern struct platform_device s3c_device_spi2;

extern struct platform_device s3c_device_nand;
extern struct platform_device s3c_device_onenand;
extern struct platform_device s3c_device_cfcon;

extern struct platform_device s3c_device_usbgadget;
extern struct platform_device s3c_device_usb_otghcd;
extern struct platform_device s3c_device_keypad;
extern struct platform_device s3c_device_ts;
extern struct platform_device s3c_device_g3d;
extern struct platform_device s3c_device_gvg;

#ifndef CONFIG_MACH_SMDKC110
extern struct platform_device s3c_device_smc911x;
#else
extern struct platform_device s3c_device_dm9000;
#endif

extern struct platform_device s3c_device_fimc0;
extern struct platform_device s3c_device_fimc1;

extern struct platform_device s3c_device_mfc;
extern struct platform_device s3c_device_ac97;

extern struct platform_device s3c_device_fimc0;
extern struct platform_device s3c_device_fimc1;
extern struct platform_device s3c_device_fimc2;

extern struct platform_device s3c_device_csis;
extern struct platform_device s3c_device_fb;

extern struct platform_device s3c_device_ipc;

extern struct platform_device s3c_device_rp;

extern struct platform_device s5p_device_tvout;

extern struct platform_device s3c_device_test;
/* s3c2440 specific devices */

#ifdef CONFIG_CPU_S3C2440

extern struct platform_device s3c_device_camif;

#endif
