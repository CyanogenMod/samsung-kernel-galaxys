/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp25.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP25 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP25CON			(S5PC11X_MP25_BASE + 0x00)
#define S5PC11X_MP25DAT			(S5PC11X_MP25_BASE + 0x04)
#define S5PC11X_MP25PUD			(S5PC11X_MP25_BASE + 0x08)
#define S5PC11X_MP25DRV			(S5PC11X_MP25_BASE + 0x0c)
#define S5PC11X_MP25CONPDN		(S5PC11X_MP25_BASE + 0x10)
#define S5PC11X_MP25PUDPDN		(S5PC11X_MP25_BASE + 0x14)

#define S5PC11X_MP25_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP25_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP25_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP25_0_LD1_DATA_24	(0x2 << 0)
#define S5PC11X_MP25_1_LD1_DATA_25	(0x2 << 4)
#define S5PC11X_MP25_2_LD1_DATA_26	(0x2 << 8)
#define S5PC11X_MP25_3_LD1_DATA_27	(0x2 << 12)
#define S5PC11X_MP25_4_LD1_DATA_28	(0x2 << 16)
#define S5PC11X_MP25_5_LD1_DATA_29	(0x2 << 20)
#define S5PC11X_MP25_6_LD1_DATA_30	(0x2 << 24)
#define S5PC11X_MP25_7_LD1_DATA_31	(0x2 << 28)

