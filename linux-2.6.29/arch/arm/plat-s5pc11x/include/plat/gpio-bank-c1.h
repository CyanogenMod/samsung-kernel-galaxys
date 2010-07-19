/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-c.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank C register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_GPC1CON			(S5PC11X_GPC1_BASE + 0x00)
#define S5PC11X_GPC1DAT			(S5PC11X_GPC1_BASE + 0x04)
#define S5PC11X_GPC1PUD			(S5PC11X_GPC1_BASE + 0x08)
#define S5PC11X_GPC1DRV			(S5PC11X_GPC1_BASE + 0x0c)
#define S5PC11X_GPC1CONPDN		(S5PC11X_GPC1_BASE + 0x10)
#define S5PC11X_GPC1PUDPDN		(S5PC11X_GPC1_BASE + 0x14)

#define S5PC11X_GPC1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_GPC1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_GPC1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_GPC1_0_PCM_0_SCLK	(0x2 << 0)
#define S5PC11X_GPC1_0_SPDIF_0_OUT	(0x3 << 0)
#define S5PC11X_GPC1_0_I2S_2_SCLK	(0x4 << 0)
#define S5PC11X_GPC1_0_GPIO_INT5_0	(0xf << 0)

#define S5PC11X_GPC1_1_PCM_0_EXTCLK	(0x2 << 4)
#define S5PC11X_GPC1_1_SPDIF_EXTCLK	(0x3 << 4)
#define S5PC11X_GPC1_1_I2S_2_CDCLK	(0x4 << 4)
#define S5PC11X_GPC1_1_GPIO_INT5_1	(0xf << 4)

#define S5PC11X_GPC1_2_PCM_0_FSYNC	(0x2 << 8)
#define S5PC11X_GPC1_2_LCD_FRM		(0x3 << 8)
#define S5PC11X_GPC1_2_I2S_2_LRCLK	(0x4 << 8)
#define S5PC11X_GPC1_2_GPIO_INT5_2	(0xf << 8)

#define S5PC11X_GPC1_3_PCM_0_SIN	(0x2 << 12)
#define S5PC11X_GPC1_3_I2S_2_SDI	(0x4 << 12)
#define S5PC11X_GPC1_3_GPIO_INT5_3	(0xf << 12)

#define S5PC11X_GPC1_4_PCM_0_SOUT	(0x2 << 16)
#define S5PC11X_GPC1_4_I2S_2_SDO	(0x4 << 16)
#define S5PC11X_GPC1_4_GPIO_INT5_4	(0xf << 16)

