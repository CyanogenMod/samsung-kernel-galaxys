/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-d.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank D register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC1XX_GPD1CON			(S5PC1XX_GPD1_BASE + 0x00)
#define S5PC1XX_GPD1DAT			(S5PC1XX_GPD1_BASE + 0x04)
#define S5PC1XX_GPD1PUD			(S5PC1XX_GPD1_BASE + 0x08)
#define S5PC1XX_GPD1DRV			(S5PC1XX_GPD1_BASE + 0x0c)
#define S5PC1XX_GPD1CONPDN		(S5PC1XX_GPD1_BASE + 0x10)
#define S5PC1XX_GPD1PUDPDN		(S5PC1XX_GPD1_BASE + 0x14)

#define S5PC1XX_GPD1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPD1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPD1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPD_1_0_I2C0_SDA	(0x2 << 0)
#define S5PC1XX_GPD_1_0_GPIO_INT7_0	(0xf << 0)

#define S5PC1XX_GPD_1_1_I2C0_SCL	(0x2 << 4)
#define S5PC1XX_GPD_1_1_GPIO_INT7_1	(0xf << 4)

#define S5PC1XX_GPD_1_2_I2C1_SDA	(0x2 << 8)
#define S5PC1XX_GPD_1_2_GPIO_INT7_2	(0xf << 8)

#define S5PC1XX_GPD_1_3_I2C1_SCL	(0x2 << 12)
#define S5PC1XX_GPD_1_3_GPIO_INT7_3	(0xf << 12)

#define S5PC1XX_GPD_1_4_I2C2_SDA	(0x2 << 16)
#define S5PC1XX_GPD_1_4_IEM_SCLK	(0x3 << 16)
#define S5PC1XX_GPD_1_4_GPIO_INT7_4	(0xf << 16)

#define S5PC1XX_GPD_1_5_I2C2_SCL	(0x2 << 20)
#define S5PC1XX_GPD_1_5_IEM_SPWI	(0x3 << 20)
#define S5PC1XX_GPD_1_5_GPIO_INT7_5	(0xf << 20)

