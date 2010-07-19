/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-l0.h
 *
 * Copyright 2009 Samsung Electronics Co.
 * 	Byungho Min <bhmin@samsung.com>
 *
 * GPIO Bank L0 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC1XX_GPL0CON			(S5PC1XX_GPL0_BASE + 0x00)
#define S5PC1XX_GPL0DAT			(S5PC1XX_GPL0_BASE + 0x04)
#define S5PC1XX_GPL0PUD			(S5PC1XX_GPL0_BASE + 0x08)
#define S5PC1XX_GPL0DRV			(S5PC1XX_GPL0_BASE + 0x0c)
#define S5PC1XX_GPL0CONPDN		(S5PC1XX_GPL0_BASE + 0x10)
#define S5PC1XX_GPL0PUDPDN		(S5PC1XX_GPL0_BASE + 0x14)

#define S5PC1XX_GPL0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPL0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPL0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPL0_0_EBI_ADDR0	(0x2 << 0)
#define S5PC1XX_GPL0_1_EBI_ADDR1	(0x2 << 4)
#define S5PC1XX_GPL0_2_EBI_ADDR2	(0x2 << 8)
#define S5PC1XX_GPL0_3_EBI_ADDR3	(0x2 << 12)
#define S5PC1XX_GPL0_4_EBI_ADDR4	(0x2 << 16)
#define S5PC1XX_GPL0_5_EBI_ADDR5	(0x2 << 20)
#define S5PC1XX_GPL0_6_EBI_ADDR6	(0x2 << 24)
#define S5PC1XX_GPL0_7_EBI_ADDR7	(0x2 << 28)
