/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp27.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP27 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP27CON			(S5PC11X_MP27_BASE + 0x00)
#define S5PC11X_MP27DAT			(S5PC11X_MP27_BASE + 0x04)
#define S5PC11X_MP27PUD			(S5PC11X_MP27_BASE + 0x08)
#define S5PC11X_MP27DRV			(S5PC11X_MP27_BASE + 0x0c)
#define S5PC11X_MP27CONPDN		(S5PC11X_MP27_BASE + 0x10)
#define S5PC11X_MP27PUDPDN		(S5PC11X_MP27_BASE + 0x14)

#define S5PC11X_MP27_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP27_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP27_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP27_0_LD1_DQM_0	(0x2 << 0)
#define S5PC11X_MP27_1_LD1_DQM_1	(0x2 << 4)
#define S5PC11X_MP27_2_LD1_DQM_2	(0x2 << 8)
#define S5PC11X_MP27_3_LD1_DQM_3	(0x2 << 12)
#define S5PC11X_MP27_4_LD1_CKE_0	(0x2 << 16)
#define S5PC11X_MP27_5_LD1_CKE_1	(0x2 << 20)
#define S5PC11X_MP27_6_LD1_SCLK		(0x2 << 24)
#define S5PC11X_MP27_7_LD1_nSCLK	(0x2 << 28)

