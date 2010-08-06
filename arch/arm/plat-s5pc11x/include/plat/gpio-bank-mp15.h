/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp15.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP15 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP15CON			(S5PC11X_MP15_BASE + 0x00)
#define S5PC11X_MP15DAT			(S5PC11X_MP15_BASE + 0x04)
#define S5PC11X_MP15PUD			(S5PC11X_MP15_BASE + 0x08)
#define S5PC11X_MP15DRV			(S5PC11X_MP15_BASE + 0x0c)
#define S5PC11X_MP15CONPDN		(S5PC11X_MP15_BASE + 0x10)
#define S5PC11X_MP15PUDPDN		(S5PC11X_MP15_BASE + 0x14)

#define S5PC11X_MP15_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP15_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP15_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP15_0_LD0_DATA_24	(0x2 << 0)
#define S5PC11X_MP15_1_LD0_DATA_25	(0x2 << 4)
#define S5PC11X_MP15_2_LD0_DATA_26	(0x2 << 8)
#define S5PC11X_MP15_3_LD0_DATA_27	(0x2 << 12)
#define S5PC11X_MP15_4_LD0_DATA_28	(0x2 << 16)
#define S5PC11X_MP15_5_LD0_DATA_29	(0x2 << 20)
#define S5PC11X_MP15_6_LD0_DATA_30	(0x2 << 24)
#define S5PC11X_MP15_7_LD0_DATA_31	(0x2 << 28)

