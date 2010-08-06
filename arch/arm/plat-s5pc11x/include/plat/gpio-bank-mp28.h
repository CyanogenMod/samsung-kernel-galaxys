/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp28.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP28 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP28CON			(S5PC11X_MP28_BASE + 0x00)
#define S5PC11X_MP28DAT			(S5PC11X_MP28_BASE + 0x04)
#define S5PC11X_MP28PUD			(S5PC11X_MP28_BASE + 0x08)
#define S5PC11X_MP28DRV			(S5PC11X_MP28_BASE + 0x0c)
#define S5PC11X_MP28CONPDN		(S5PC11X_MP28_BASE + 0x10)
#define S5PC11X_MP28PUDPDN		(S5PC11X_MP28_BASE + 0x14)

#define S5PC11X_MP28_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP28_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP28_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP28_0_LD1_CSn_0	(0x2 << 0)
#define S5PC11X_MP28_1_LD1_CSn_1	(0x2 << 4)
#define S5PC11X_MP28_2_LD1_RASn		(0x2 << 8)
#define S5PC11X_MP28_3_LD1_CASn		(0x2 << 12)
#define S5PC11X_MP28_4_LD1_WEn		(0x2 << 16)
#define S5PC11X_MP28_5_LD1_IOGATE_IN	(0x2 << 20)
#define S5PC11X_MP28_6_LD1_IOGATE_OUT	(0x2 << 24)

