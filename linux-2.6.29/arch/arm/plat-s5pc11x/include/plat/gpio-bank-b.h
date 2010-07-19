/* linux/arch/arm/plat-s5pc11x/include/plat/gpio-bank-b.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank B register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_GPBCON			(S5PC11X_GPB_BASE + 0x00)
#define S5PC11X_GPBDAT			(S5PC11X_GPB_BASE + 0x04)
#define S5PC11X_GPBPUD			(S5PC11X_GPB_BASE + 0x08)
#define S5PC11X_GPBDRV			(S5PC11X_GPB_BASE + 0x0c)
#define S5PC11X_GPBCONPDN		(S5PC11X_GPB_BASE + 0x10)
#define S5PC11X_GPBPUDPDN		(S5PC11X_GPB_BASE + 0x14)

#define S5PC11X_GPB_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_GPB_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_GPB_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_GPB0_SPI_0_CLK		(0x2 << 0)
#define S5PC11X_GPB0_GPIO_INT_0		(0xf << 0)

#define S5PC11X_GPB1_SPI_0_nSS		(0x2 << 4)
#define S5PC11X_GPB1_GPIO_INT_1		(0xf << 4)

#define S5PC11X_GPB2_SPI_0_MISO		(0x2 << 8)
#define S5PC11X_GPB2_GPIO_INT_2		(0xf << 8)

#define S5PC11X_GPB3_SPI_0_MOSI		(0x2 << 12)
#define S5PC11X_GPB3_GPIO_INT_3		(0xf << 12)

#define S5PC11X_GPB4_SPI_1_CLK		(0x2 << 16)
#define S5PC11X_GPB4_GPIO_INT_4		(0xf << 16)

#define S5PC11X_GPB5_SPI_1_nSS		(0x2 << 20)
#define S5PC11X_GPB5_GPIO_INT_5		(0xf << 20)

#define S5PC11X_GPB6_SPI_1_MISO		(0x2 << 24)
#define S5PC11X_GPB6_GPIO_INT_6		(0xf << 24)

#define S5PC11X_GPB7_SPI_1_MOSI		(0x2 << 28)
#define S5PC11X_GPB7_GPIO_INT_7		(0xf << 28)

