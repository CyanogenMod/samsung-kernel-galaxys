/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-l3.h
 *
 * Copyright 2009 Samsung Electronics Co.
 * 	Byungho Min <bhmin@samsung.com>
 *
 * GPIO Bank L3 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC1XX_GPL3CON                 (S5PC1XX_GPL3_BASE + 0x00)
#define S5PC1XX_GPL3DAT                 (S5PC1XX_GPL3_BASE + 0x04)
#define S5PC1XX_GPL3PUD                 (S5PC1XX_GPL3_BASE + 0x08)
#define S5PC1XX_GPL3DRV                 (S5PC1XX_GPL3_BASE + 0x0c)
#define S5PC1XX_GPL3CONPDN              (S5PC1XX_GPL3_BASE + 0x10)
#define S5PC1XX_GPL3PUDPDN              (S5PC1XX_GPL3_BASE + 0x14)

#define S5PC1XX_GPL3_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPL3_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPL3_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPL3_0_EBI_DATA3	(0x2 << 0)
#define S5PC1XX_GPL3_1_EBI_DATA4	(0x2 << 4)
#define S5PC1XX_GPL3_2_EBI_DATA5	(0x2 << 8)
#define S5PC1XX_GPL3_3_EBI_DATA6	(0x2 << 12)
#define S5PC1XX_GPL3_4_EBI_DATA7	(0x2 << 16)
#define S5PC1XX_GPL3_5_EBI_DATA8	(0x2 << 20)
#define S5PC1XX_GPL3_6_EBI_DATA9	(0x2 << 24)
#define S5PC1XX_GPL3_7_EBI_DATA10	(0x2 << 28)
