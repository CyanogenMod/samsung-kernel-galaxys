/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp11.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP11 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP11CON			(S5PC11X_MP11_BASE + 0x00)
#define S5PC11X_MP11DAT			(S5PC11X_MP11_BASE + 0x04)
#define S5PC11X_MP11PUD			(S5PC11X_MP11_BASE + 0x08)
#define S5PC11X_MP11DRV			(S5PC11X_MP11_BASE + 0x0c)
#define S5PC11X_MP11CONPDN		(S5PC11X_MP11_BASE + 0x10)
#define S5PC11X_MP11PUDPDN		(S5PC11X_MP11_BASE + 0x14)

#define S5PC11X_MP11_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP11_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP11_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP11_0_LD0_ADDR_8	(0x2 << 0)
#define S5PC11X_MP11_1_LD0_ADDR_9	(0x2 << 4)
#define S5PC11X_MP11_2_LD0_ADDR_10	(0x2 << 8)
#define S5PC11X_MP11_3_LD0_ADDR_11	(0x2 << 12)
#define S5PC11X_MP11_4_LD0_ADDR_12	(0x2 << 16)
#define S5PC11X_MP11_5_LD0_ADDR_13	(0x2 << 20)
#define S5PC11X_MP11_6_LD0_ADDR_14	(0x2 << 24)
#define S5PC11X_MP11_7_LD0_ADDR_15	(0x2 << 28)

