/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp24.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP24 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP24CON			(S5PC11X_MP24_BASE + 0x00)
#define S5PC11X_MP24DAT			(S5PC11X_MP24_BASE + 0x04)
#define S5PC11X_MP24PUD			(S5PC11X_MP24_BASE + 0x08)
#define S5PC11X_MP24DRV			(S5PC11X_MP24_BASE + 0x0c)
#define S5PC11X_MP24CONPDN		(S5PC11X_MP24_BASE + 0x10)
#define S5PC11X_MP24PUDPDN		(S5PC11X_MP24_BASE + 0x14)

#define S5PC11X_MP24_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP24_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP24_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP24_0_LD1_DATA_16	(0x2 << 0)
#define S5PC11X_MP24_1_LD1_DATA_17	(0x2 << 4)
#define S5PC11X_MP24_2_LD1_DATA_18	(0x2 << 8)
#define S5PC11X_MP24_3_LD1_DATA_19	(0x2 << 12)
#define S5PC11X_MP24_4_LD1_DATA_20	(0x2 << 16)
#define S5PC11X_MP24_5_LD1_DATA_21	(0x2 << 20)
#define S5PC11X_MP24_6_LD1_DATA_22	(0x2 << 24)
#define S5PC11X_MP24_7_LD1_DATA_23	(0x2 << 28)

