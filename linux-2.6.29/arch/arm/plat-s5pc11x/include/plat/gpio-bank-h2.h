/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-h2.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank H2 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_GPH2CON			(S5PC11X_GPH2_BASE + 0x00)
#define S5PC11X_GPH2DAT			(S5PC11X_GPH2_BASE + 0x04)
#define S5PC11X_GPH2PUD			(S5PC11X_GPH2_BASE + 0x08)
#define S5PC11X_GPH2DRV			(S5PC11X_GPH2_BASE + 0x0c)
#define S5PC11X_GPH2CONPDN		(S5PC11X_GPH2_BASE + 0x10)
#define S5PC11X_GPH2PUDPDN		(S5PC11X_GPH2_BASE + 0x14)

#define S5PC11X_GPH2_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_GPH2_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_GPH2_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_GPH2_0_KP_COL_0		(0x3 << 0)
#define S5PC11X_GPH2_0_EXT_INT32_0	(0xf << 0)

#define S5PC11X_GPH2_1_KP_COL_1		(0x3 << 4)
#define S5PC11X_GPH2_1_EXT_INT32_1	(0xf << 4)

#define S5PC11X_GPH2_2_KP_COL_2		(0x3 << 8)
#define S5PC11X_GPH2_2_EXT_INT32_2	(0xf << 8)

#define S5PC11X_GPH2_3_KP_COL_3		(0x3 << 12)
#define S5PC11X_GPH2_3_EXT_INT32_3	(0xf << 12)

#define S5PC11X_GPH2_4_KP_COL_4		(0x3 << 16)
#define S5PC11X_GPH2_4_EXT_INT32_4	(0xf << 16)

#define S5PC11X_GPH2_5_KP_COL_5		(0x3 << 20)
#define S5PC11X_GPH2_5_EXT_INT32_5	(0xf << 20)

#define S5PC11X_GPH2_6_KP_COL_6		(0x3 << 24)
#define S5PC11X_GPH2_6_EXT_INT32_6	(0xf << 24)

#define S5PC11X_GPH2_7_KP_COL_7		(0x3 << 28)
#define S5PC11X_GPH2_7_EXT_INT32_7	(0xf << 28)


