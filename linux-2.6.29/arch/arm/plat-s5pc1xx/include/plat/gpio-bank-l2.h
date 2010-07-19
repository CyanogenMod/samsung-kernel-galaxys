/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-l2.h
 *
 * Copyright 2009 Samsung Electronics Co.
 * 	Byungho Min <bhmin@samsung.com>
 *
 * GPIO Bank L2 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC1XX_GPL2CON                 (S5PC1XX_GPL2_BASE + 0x00)
#define S5PC1XX_GPL2DAT                 (S5PC1XX_GPL2_BASE + 0x04)
#define S5PC1XX_GPL2PUD                 (S5PC1XX_GPL2_BASE + 0x08)
#define S5PC1XX_GPL2DRV                 (S5PC1XX_GPL2_BASE + 0x0c)
#define S5PC1XX_GPL2CONPDN              (S5PC1XX_GPL2_BASE + 0x10)
#define S5PC1XX_GPL2PUDPDN              (S5PC1XX_GPL2_BASE + 0x14)

#define S5PC1XX_GPL2_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPL2_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPL2_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPL2_0_EBI_ADDR16	(0x2 << 0)
#define S5PC1XX_GPL2_1_EBI_ADDR17	(0x2 << 4)
#define S5PC1XX_GPL2_2_EBI_ADDR18	(0x2 << 8)
#define S5PC1XX_GPL2_3_EBI_ADDR19	(0x2 << 12)
#define S5PC1XX_GPL2_4_EBI_ADDR20	(0x2 << 16)
#define S5PC1XX_GPL2_5_EBI_DATA0	(0x2 << 20)
#define S5PC1XX_GPL2_6_EBI_DATA1	(0x2 << 24)
#define S5PC1XX_GPL2_7_EBI_DATA2	(0x2 << 28)
