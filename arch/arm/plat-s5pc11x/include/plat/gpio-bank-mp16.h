/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp16.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank MP16 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_MP16CON			(S5PC11X_MP16_BASE + 0x00)
#define S5PC11X_MP16DAT			(S5PC11X_MP16_BASE + 0x04)
#define S5PC11X_MP16PUD			(S5PC11X_MP16_BASE + 0x08)
#define S5PC11X_MP16DRV			(S5PC11X_MP16_BASE + 0x0c)
#define S5PC11X_MP16CONPDN		(S5PC11X_MP16_BASE + 0x10)
#define S5PC11X_MP16PUDPDN		(S5PC11X_MP16_BASE + 0x14)

#define S5PC11X_MP16_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_MP16_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_MP16_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_MP16_0_LD0_DQS_0	(0x2 << 0)
#define S5PC11X_MP16_1_LD0_DQS_1	(0x2 << 4)
#define S5PC11X_MP16_2_LD0_DQS_2	(0x2 << 8)
#define S5PC11X_MP16_3_LD0_DQS_3	(0x2 << 12)
#define S5PC11X_MP16_4_LD0_DQSn_0	(0x2 << 16)
#define S5PC11X_MP16_5_LD0_DQSn_1	(0x2 << 20)
#define S5PC11X_MP16_6_LD0_DQSn_2	(0x2 << 24)
#define S5PC11X_MP16_7_LD0_DQSn_3	(0x2 << 28)

