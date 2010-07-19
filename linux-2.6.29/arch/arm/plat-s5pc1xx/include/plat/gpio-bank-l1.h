/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-l1.h
 *
 * Copyright 2009 Samsung Electronics Co.
 * 	Byungho Min <bhmin@samsung.com>
 *
 * GPIO Bank L1 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC1XX_GPL1CON                 (S5PC1XX_GPL1_BASE + 0x00)
#define S5PC1XX_GPL1DAT                 (S5PC1XX_GPL1_BASE + 0x04)
#define S5PC1XX_GPL1PUD                 (S5PC1XX_GPL1_BASE + 0x08)
#define S5PC1XX_GPL1DRV                 (S5PC1XX_GPL1_BASE + 0x0c)
#define S5PC1XX_GPL1CONPDN              (S5PC1XX_GPL1_BASE + 0x10)
#define S5PC1XX_GPL1PUDPDN              (S5PC1XX_GPL1_BASE + 0x14)

#define S5PC1XX_GPL1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPL1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPL1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPL1_0_EBI_ADDR8	(0x2 << 0)
#define S5PC1XX_GPL1_1_EBI_ADDR9	(0x2 << 4)
#define S5PC1XX_GPL1_2_EBI_ADDR10	(0x2 << 8)
#define S5PC1XX_GPL1_3_EBI_ADDR11	(0x2 << 12)
#define S5PC1XX_GPL1_4_EBI_ADDR12	(0x2 << 16)
#define S5PC1XX_GPL1_5_EBI_ADDR13	(0x2 << 20)
#define S5PC1XX_GPL1_6_EBI_ADDR14	(0x2 << 24)
#define S5PC1XX_GPL1_7_EBI_ADDR15	(0x2 << 28)
