/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-h3.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank H3 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_GPH3CON			(S5PC11X_GPH3_BASE + 0x00)
#define S5PC11X_GPH3DAT			(S5PC11X_GPH3_BASE + 0x04)
#define S5PC11X_GPH3PUD			(S5PC11X_GPH3_BASE + 0x08)
#define S5PC11X_GPH3DRV			(S5PC11X_GPH3_BASE + 0x0c)
#define S5PC11X_GPH3CONPDN		(S5PC11X_GPH3_BASE + 0x10)
#define S5PC11X_GPH3PUDPDN		(S5PC11X_GPH3_BASE + 0x14)

#define S5PC11X_GPH3_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_GPH3_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_GPH3_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_GPH3_0_KP_ROW_0		(0x3 << 0)
#define S5PC11X_GPH3_0_EXT_INT33_0	(0xf << 0)

#define S5PC11X_GPH3_1_KP_ROW_1		(0x3 << 4)
#define S5PC11X_GPH3_1_EXT_INT33_1	(0xf << 4)

#define S5PC11X_GPH3_2_KP_ROW_2		(0x3 << 8)
#define S5PC11X_GPH3_2_EXT_INT33_2	(0xf << 8)

#define S5PC11X_GPH3_3_KP_ROW_3		(0x3 << 12)
#define S5PC11X_GPH3_3_EXT_INT33_3	(0xf << 12)

#define S5PC11X_GPH3_4_KP_ROW_4		(0x3 << 16)
#define S5PC11X_GPH3_4_EXT_INT33_4	(0xf << 16)

#define S5PC11X_GPH3_5_KP_ROW_5		(0x3 << 20)
#define S5PC11X_GPH3_5_EXT_INT33_5	(0xf << 20)

#define S5PC11X_GPH3_6_KP_ROW_6		(0x3 << 24)
#define S5PC11X_GPH3_6_EXT_INT33_6	(0xf << 24)

#define S5PC11X_GPH3_7_KP_ROW_7		(0x3 << 28)
#define S5PC11X_GPH3_7_EXT_INT33_7	(0xf << 28)

