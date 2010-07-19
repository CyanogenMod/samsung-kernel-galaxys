/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-j0.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank J0 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_GPJ0CON			(S5PC11X_GPJ0_BASE + 0x00)
#define S5PC11X_GPJ0DAT			(S5PC11X_GPJ0_BASE + 0x04)
#define S5PC11X_GPJ0PUD			(S5PC11X_GPJ0_BASE + 0x08)
#define S5PC11X_GPJ0DRV			(S5PC11X_GPJ0_BASE + 0x0c)
#define S5PC11X_GPJ0CONPDN		(S5PC11X_GPJ0_BASE + 0x10)
#define S5PC11X_GPJ0PUDPDN		(S5PC11X_GPJ0_BASE + 0x14)

#define S5PC11X_GPJ0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_GPJ0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_GPJ0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_GPJ0_0_MSM_ADDR_0	(0x2 << 0)
#define S5PC11X_GPJ0_0_CAM_B_DATA_0	(0x3 << 0)
#define S5PC11X_GPJ0_0_CF_ADDR_0	(0x4 << 0)
#define S5PC11X_GPJ0_0_MIPI_BYTE_CLK	(0x5 << 0)
#define S5PC11X_GPJ0_0_GPIO_INT18_0	(0xf << 0)

#define S5PC11X_GPJ0_1_MSM_ADDR_1	(0x2 << 4)
#define S5PC11X_GPJ0_1_CAM_B_DATA_1	(0x3 << 4)
#define S5PC11X_GPJ0_1_CF_ADDR_1	(0x4 << 4)
#define S5PC11X_GPJ0_1_MIPI_ESC_CLK	(0x5 << 4)
#define S5PC11X_GPJ0_1_GPIO_INT18_1	(0xf << 4)

#define S5PC11X_GPJ0_2_MSM_ADDR_2	(0x2 << 8)
#define S5PC11X_GPJ0_2_CAM_B_DATA_2	(0x3 << 8)
#define S5PC11X_GPJ0_2_CF_ADDR_2	(0x4 << 8)
#define S5PC11X_GPJ0_2_TS_CLK		(0x5 << 8)
#define S5PC11X_GPJ0_2_GPIO_INT18_2	(0xf << 8)

#define S5PC11X_GPJ0_3_MSM_ADDR_3	(0x2 << 12)
#define S5PC11X_GPJ0_3_CAM_B_DATA_3	(0x3 << 12)
#define S5PC11X_GPJ0_3_CF_IORDY		(0x4 << 12)
#define S5PC11X_GPJ0_3_TS_SYNC		(0x5 << 12)
#define S5PC11X_GPJ0_3_GPIO_INT18_3	(0xf << 12)

#define S5PC11X_GPJ0_4_MSM_ADDR_4	(0x2 << 16)
#define S5PC11X_GPJ0_4_CAM_B_DATA_4	(0x3 << 16)
#define S5PC11X_GPJ0_4_CF_INTRQ		(0x4 << 16)
#define S5PC11X_GPJ0_4_TS_VAL		(0x5 << 16)
#define S5PC11X_GPJ0_4_GPIO_INT18_4	(0xf << 16)

#define S5PC11X_GPJ0_5_MSM_ADDR_5	(0x2 << 20)
#define S5PC11X_GPJ0_5_CAM_B_DATA_5	(0x3 << 20)
#define S5PC11X_GPJ0_5_CF_DMARQ		(0x4 << 20)
#define S5PC11X_GPJ0_5_TS_DATA		(0x5 << 20)
#define S5PC11X_GPJ0_5_GPIO_INT18_5	(0xf << 20)

#define S5PC11X_GPJ0_6_MSM_ADDR_6	(0x2 << 24)
#define S5PC11X_GPJ0_6_CAM_B_DATA_6	(0x3 << 24)
#define S5PC11X_GPJ0_6_CF_DRESETN	(0x4 << 24)
#define S5PC11X_GPJ0_6_TS_ERROR		(0x5 << 24)
#define S5PC11X_GPJ0_6_GPIO_INT18_6	(0xf << 24)

#define S5PC11X_GPJ0_7_MSM_ADDR_7	(0x2 << 28)
#define S5PC11X_GPJ0_7_CAM_B_DATA_7	(0x3 << 28)
#define S5PC11X_GPJ0_7_CF_DMACKN	(0x4 << 28)
#define S5PC11X_GPJ0_7_MHL_D0		(0x5 << 28)
#define S5PC11X_GPJ0_7_GPIO_INT18_7	(0xf << 28)

