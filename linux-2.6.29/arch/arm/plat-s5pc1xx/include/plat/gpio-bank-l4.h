/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-l4.h
 *
 * Copyright 2009 Samsung Electronics Co.
 * 	Byungho Min <bhmin@samsung.com>
 *
 * GPIO Bank L4 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC1XX_GPL4CON                 (S5PC1XX_GPL4_BASE + 0x00)
#define S5PC1XX_GPL4DAT                 (S5PC1XX_GPL4_BASE + 0x04)
#define S5PC1XX_GPL4PUD                 (S5PC1XX_GPL4_BASE + 0x08)
#define S5PC1XX_GPL4DRV                 (S5PC1XX_GPL4_BASE + 0x0c)
#define S5PC1XX_GPL4CONPDN              (S5PC1XX_GPL4_BASE + 0x10)
#define S5PC1XX_GPL4PUDPDN              (S5PC1XX_GPL4_BASE + 0x14)

#define S5PC1XX_GPL4_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPL4_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPL4_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPL4_0_EBI_DATA11	(0x2 << 0)
#define S5PC1XX_GPL4_1_EBI_DATA12	(0x2 << 4)
#define S5PC1XX_GPL4_2_EBI_DATA13	(0x2 << 8)
#define S5PC1XX_GPL4_3_EBI_DATA14	(0x2 << 12)
#define S5PC1XX_GPL4_4_EBI_DATA15	(0x2 << 16)
