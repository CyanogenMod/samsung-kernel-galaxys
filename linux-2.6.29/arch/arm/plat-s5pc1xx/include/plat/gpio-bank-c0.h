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

#define S5PC1XX_GPC0CON			(S5PC1XX_GPC0_BASE + 0x00)
#define S5PC1XX_GPC0DAT			(S5PC1XX_GPC0_BASE + 0x04)
#define S5PC1XX_GPC0PUD			(S5PC1XX_GPC0_BASE + 0x08)
#define S5PC1XX_GPC0DRV			(S5PC1XX_GPC0_BASE + 0x0c)
#define S5PC1XX_GPC0CONPDN		(S5PC1XX_GPC0_BASE + 0x10)
#define S5PC1XX_GPC0PUDPDN		(S5PC1XX_GPC0_BASE + 0x14)

#define S5PC1XX_GPC0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPC0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPC0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPC0_0_I2S_1_SCLK	(0x2 << 0)
#define S5PC1XX_GPC0_0_PCM_1_SCLK	(0x3 << 0)
#define S5PC1XX_GPC0_0_AC97_BITCLK	(0x4 << 0)
#define S5PC1XX_GPC0_0_GPIO_INT3_0	(0xf << 0)

#define S5PC1XX_GPC0_1_I2S_1_CDCLK	(0x2 << 4)
#define S5PC1XX_GPC0_1_PCM_1_EXTCLK	(0x3 << 4)
#define S5PC1XX_GPC0_1_AC97_RESETn	(0x4 << 4)
#define S5PC1XX_GPC0_1_GPIO_INT3_1	(0xf << 4)

#define S5PC1XX_GPC0_2_I2S_1_LRCK	(0x2 << 8)
#define S5PC1XX_GPC0_2_PCM_1_FSYNC	(0x3 << 8)
#define S5PC1XX_GPC0_2_AC97_SYNC	(0x4 << 8)
#define S5PC1XX_GPC0_2_GPIO_INT3_2	(0xf << 8)

#define S5PC1XX_GPC0_3_I2S_1_SDI	(0x2 << 12)
#define S5PC1XX_GPC0_3_PCM_1_SIN	(0x3 << 12)
#define S5PC1XX_GPC0_3_AC97_SDI		(0x4 << 12)
#define S5PC1XX_GPC0_3_GPIO_INT3_3	(0xf << 12)

#define S5PC1XX_GPC0_4_I2S_1_SDO	(0x2 << 16)
#define S5PC1XX_GPC0_4_PCM_1_SOUT	(0x3 << 16)
#define S5PC1XX_GPC0_4_AC97_SDO		(0x4 << 16)
#define S5PC1XX_GPC0_4_GPIO_INT3_4	(0xf << 16)

