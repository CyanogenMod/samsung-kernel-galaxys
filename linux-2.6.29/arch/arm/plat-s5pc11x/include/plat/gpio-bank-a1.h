/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-a1.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank A1 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC11X_GPA1CON			(S5PC11X_GPA1_BASE + 0x00)
#define S5PC11X_GPA1DAT			(S5PC11X_GPA1_BASE + 0x04)
#define S5PC11X_GPA1PUD			(S5PC11X_GPA1_BASE + 0x08)
#define S5PC11X_GPA1DRV			(S5PC11X_GPA1_BASE + 0x0c)
#define S5PC11X_GPA1CONPDN		(S5PC11X_GPA1_BASE + 0x10)
#define S5PC11X_GPA1PUDPDN		(S5PC11X_GPA1_BASE + 0x14)

#define S5PC11X_GPA1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC11X_GPA1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC11X_GPA1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC11X_GPA1_0_UART_2_RXD	(0x2 << 0)
#define S5PC11X_GPA1_0_UART_AUDIO_RXD	(0x4 << 0)
#define S5PC11X_GPA1_0_GPIO_INT2_0	(0xf << 0)

#define S5PC11X_GPA1_1_UART_2_TXD	(0x2 << 4)
#define S5PC11X_GPA1_1_UART_AUDIO_TXD	(0x4 << 4)
#define S5PC11X_GPA1_1_GPIO_INT2_1	(0xf << 4)

#define S5PC11X_GPA1_2_UART_3_RXD	(0x2 << 8)
#define S5PC11X_GPA1_2_UART_2_CTSn	(0x3 << 8)
#define S5PC11X_GPA1_2_UART_AUDIO_CTSn	(0x4 << 8)
#define S5PC11X_GPA1_2_GPIO_INT2_2	(0xf << 8)

#define S5PC11X_GPA1_3_UART_3_TXD	(0x2 << 12)
#define S5PC11X_GPA1_3_UART_2_RTSn	(0x3 << 12)
#define S5PC11X_GPA1_3_UART_AUDIO_RTSn	(0x4 << 12)
#define S5PC11X_GPA1_3_GPIO_INT2_3	(0xf << 12)

