/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-j1.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank J1 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_GPJ1CON			(S5PC11X_GPJ1_BASE + 0x00)
#define S5PC11X_GPJ1DAT			(S5PC11X_GPJ1_BASE + 0x04)
#define S5PC11X_GPJ1PUD			(S5PC11X_GPJ1_BASE + 0x08)
#define S5PC11X_GPJ1DRV			(S5PC11X_GPJ1_BASE + 0x0c)
#define S5PC11X_GPJ1CONPDN		(S5PC11X_GPJ1_BASE + 0x10)
#define S5PC11X_GPJ1PUDPDN		(S5PC11X_GPJ1_BASE + 0x14)

#define S5PC11X_GPJ1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_GPJ1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_GPJ1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_GPJ1_0_MSM_ADDR_8	(0x2 << 0)
#define S5PC11X_GPJ1_0_CAM_B_PCLK	(0x3 << 0)
#define S5PC11X_GPJ1_0_SROM_ADDR_16to22_0	(0x4 << 0)
#define S5PC11X_GPJ1_0_MHL_D1		(0x5 << 0)
#define S5PC11X_GPJ1_0_GPIO_INT19_0	(0xf << 0)

#define S5PC11X_GPJ1_1_MSM_ADDR_9	(0x2 << 4)
#define S5PC11X_GPJ1_1_CAM_B_VSYNC	(0x3 << 4)
#define S5PC11X_GPJ1_1_SROM_ADDR_16to22_1	(0x4 << 4)
#define S5PC11X_GPJ1_1_MHL_D12		(0x5 << 4)
#define S5PC11X_GPJ1_1_GPIO_INT19_1	(0xf << 4)

#define S5PC11X_GPJ1_2_MSM_ADDR_10	(0x2 << 8)
#define S5PC11X_GPJ1_2_CAM_B_HREF	(0x3 << 8)
#define S5PC11X_GPJ1_2_SROM_ADDR_16to22_2	(0x4 << 8)
#define S5PC11X_GPJ1_2_MHL_D3		(0x5 << 8)
#define S5PC11X_GPJ1_2_GPIO_INT19_2	(0xf << 8)

#define S5PC11X_GPJ1_3_MSM_ADDR_11	(0x2 << 12)
#define S5PC11X_GPJ1_3_CAM_B_FIELD	(0x3 << 12)
#define S5PC11X_GPJ1_3_SROM_ADDR_16to22_3	(0x4 << 12)
#define S5PC11X_GPJ1_3_MHL_D4		(0x5 << 12)
#define S5PC11X_GPJ1_3_GPIO_INT19_3	(0xf << 12)

#define S5PC11X_GPJ1_4_MSM_ADDR_12	(0x2 << 16)
#define S5PC11X_GPJ1_4_CAM_B_CLKOUT	(0x3 << 16)
#define S5PC11X_GPJ1_4_SROM_ADDR_16to22_4	(0x4 << 16)
#define S5PC11X_GPJ1_4_MHL_D5		(0x5 << 16)
#define S5PC11X_GPJ1_4_GPIO_INT19_4	(0xf << 16)

#define S5PC11X_GPJ1_5_MSM_ADDR_12	(0x2 << 20)
#define S5PC11X_GPJ1_5_KP_COL_0		(0x3 << 20)
#define S5PC11X_GPJ1_5_SROM_ADDR_16to22_5	(0x4 << 20)
#define S5PC11X_GPJ1_5_MHL_D6		(0x5 << 20)
#define S5PC11X_GPJ1_5_GPIO_INT19_5	(0xf << 20)

