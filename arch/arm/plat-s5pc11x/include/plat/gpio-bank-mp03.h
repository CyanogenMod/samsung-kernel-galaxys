/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp03.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP03 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP03CON			(S5PC11X_MP03_BASE + 0x00)
#define S5PC11X_MP03DAT			(S5PC11X_MP03_BASE + 0x04)
#define S5PC11X_MP03PUD			(S5PC11X_MP03_BASE + 0x08)
#define S5PC11X_MP03DRV			(S5PC11X_MP03_BASE + 0x0c)
#define S5PC11X_MP03CONPDN		(S5PC11X_MP03_BASE + 0x10)
#define S5PC11X_MP03PUDPDN		(S5PC11X_MP03_BASE + 0x14)

#define S5PC11X_MP03_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP03_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP03_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP03_0_NF_CLE		(0x2 << 0)
#define S5PC11X_MP03_0_ONANDXL_ADDRVALID	(0x5 << 0)

#define S5PC11X_MP03_1_NF_ALE		(0x2 << 4)
#define S5PC11X_MP03_1_ONANDXL_SMCLK	(0x5 << 4)

#define S5PC11X_MP03_2_NF_FWEn		(0x2 << 8)
#define S5PC11X_MP03_2_ONANDXL_RPn	(0x5 << 8)

#define S5PC11X_MP03_3_NF_FREn		(0x2 << 12)

#define S5PC11X_MP03_4_NF_RnB_0		(0x2 << 16)
#define S5PC11X_MP03_4_ONANDXL_INT_0	(0x5 << 16)

#define S5PC11X_MP03_5_NF_RnB_1		(0x2 << 20)
#define S5PC11X_MP03_5_ONANDXL_INT_1	(0x5 << 20)

#define S5PC11X_MP03_6_NF_RnB_2		(0x2 << 24)

#define S5PC11X_MP03_6_NF_RnB_3		(0x2 << 28)

