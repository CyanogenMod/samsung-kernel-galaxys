/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-e1.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank E1 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_GPE1CON			(S5PC11X_GPE1_BASE + 0x00)
#define S5PC11X_GPE1DAT			(S5PC11X_GPE1_BASE + 0x04)
#define S5PC11X_GPE1PUD			(S5PC11X_GPE1_BASE + 0x08)
#define S5PC11X_GPE1DRV			(S5PC11X_GPE1_BASE + 0x0c)
#define S5PC11X_GPE1CONPDN		(S5PC11X_GPE1_BASE + 0x10)
#define S5PC11X_GPE1PUDPDN		(S5PC11X_GPE1_BASE + 0x14)

#define S5PC11X_GPE1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_GPE1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_GPE1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_GPE1_0_CAM_A_DATA_5	(0x2 << 0)
#define S5PC11X_GPE1_0_GPIO_INT9_0	(0xf << 0)

#define S5PC11X_GPE1_1_CAM_A_DATA_6	(0x2 << 4)
#define S5PC11X_GPE1_1_GPIO_INT9_1	(0xf << 4)

#define S5PC11X_GPE1_2_CAM_A_DATA_7	(0x2 << 8)
#define S5PC11X_GPE1_2_GPIO_INT9_2	(0xf << 8)

#define S5PC11X_GPE1_3_CAM_A_CLKOUT	(0x2 << 12)
#define S5PC11X_GPE1_3_GPIO_INT9_3	(0xf << 12)

#define S5PC11X_GPE1_4_CAM_A_FIELD	(0x2 << 16)
#define S5PC11X_GPE1_4_GPIO_INT9_4	(0xf << 16)

