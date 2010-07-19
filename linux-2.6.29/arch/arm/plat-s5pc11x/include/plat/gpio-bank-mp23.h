/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp23.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP23 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP23CON			(S5PC11X_MP23_BASE + 0x00)
#define S5PC11X_MP23DAT			(S5PC11X_MP23_BASE + 0x04)
#define S5PC11X_MP23PUD			(S5PC11X_MP23_BASE + 0x08)
#define S5PC11X_MP23DRV			(S5PC11X_MP23_BASE + 0x0c)
#define S5PC11X_MP23CONPDN		(S5PC11X_MP23_BASE + 0x10)
#define S5PC11X_MP23PUDPDN		(S5PC11X_MP23_BASE + 0x14)

#define S5PC11X_MP23_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP23_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP23_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP23_0_LD1_DATA_8	(0x2 << 0)
#define S5PC11X_MP23_1_LD1_DATA_9	(0x2 << 4)
#define S5PC11X_MP23_2_LD1_DATA_10	(0x2 << 8)
#define S5PC11X_MP23_3_LD1_DATA_11	(0x2 << 12)
#define S5PC11X_MP23_4_LD1_DATA_12	(0x2 << 16)
#define S5PC11X_MP23_5_LD1_DATA_13	(0x2 << 20)
#define S5PC11X_MP23_6_LD1_DATA_14	(0x2 << 24)
#define S5PC11X_MP23_7_LD1_DATA_15	(0x2 << 28)

