/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp04.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP04 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP04CON			(S5PC11X_MP04_BASE + 0x00)
#define S5PC11X_MP04DAT			(S5PC11X_MP04_BASE + 0x04)
#define S5PC11X_MP04PUD			(S5PC11X_MP04_BASE + 0x08)
#define S5PC11X_MP04DRV			(S5PC11X_MP04_BASE + 0x0c)
#define S5PC11X_MP04CONPDN		(S5PC11X_MP04_BASE + 0x10)
#define S5PC11X_MP04PUDPDN		(S5PC11X_MP04_BASE + 0x14)

#define S5PC11X_MP04_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP04_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP04_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP04_0_EBI_ADDR_0	(0x2 << 0)
#define S5PC11X_MP04_1_EBI_ADDR_1	(0x2 << 4)
#define S5PC11X_MP04_2_EBI_ADDR_2	(0x2 << 8)
#define S5PC11X_MP04_3_EBI_ADDR_3	(0x2 << 12)
#define S5PC11X_MP04_4_EBI_ADDR_4	(0x2 << 16)
#define S5PC11X_MP04_5_EBI_ADDR_5	(0x2 << 20)
#define S5PC11X_MP04_6_EBI_ADDR_6	(0x2 << 24)
#define S5PC11X_MP04_7_EBI_ADDR_7	(0x2 << 28)

