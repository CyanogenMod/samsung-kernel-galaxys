/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp17.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP17 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP17CON			(S5PC11X_MP17_BASE + 0x00)
#define S5PC11X_MP17DAT			(S5PC11X_MP17_BASE + 0x04)
#define S5PC11X_MP17PUD			(S5PC11X_MP17_BASE + 0x08)
#define S5PC11X_MP17DRV			(S5PC11X_MP17_BASE + 0x0c)
#define S5PC11X_MP17CONPDN		(S5PC11X_MP17_BASE + 0x10)
#define S5PC11X_MP17PUDPDN		(S5PC11X_MP17_BASE + 0x14)

#define S5PC11X_MP17_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP17_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP17_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP17_0_LD0_DQM_0	(0x2 << 0)
#define S5PC11X_MP17_1_LD0_DQM_1	(0x2 << 4)
#define S5PC11X_MP17_2_LD0_DQM_2	(0x2 << 8)
#define S5PC11X_MP17_3_LD0_DQM_3	(0x2 << 12)
#define S5PC11X_MP17_4_LD0_CKE_0	(0x2 << 16)
#define S5PC11X_MP17_5_LD0_CKE_1	(0x2 << 20)
#define S5PC11X_MP17_6_LD0_SCLK		(0x2 << 24)
#define S5PC11X_MP17_7_LD0_nSCLK	(0x2 << 28)

