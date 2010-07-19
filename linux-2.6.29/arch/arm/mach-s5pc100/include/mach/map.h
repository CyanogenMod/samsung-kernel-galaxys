/* linux/arch/arm/mach-s5pc100/include/mach/map.h
 *
 * Copyright 2008 Samsung Electronics Co.
 * Copyright 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S5PC1XX - Memory map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_MAP_H
#define __ASM_ARCH_MAP_H __FILE__

#include <plat/map-base.h>

#define S5PC100_PA_UART		(0xEC000000)
#define S5PC1XX_PA_UART		S5PC100_PA_UART
#define S5PC1XX_PA_TIMER	(0xEA000000)
#define S5PC1XX_PA_IIC0		(0xEC100000)
#define S5PC1XX_PA_IIC1		(0xEC200000)
#define S5PC1XX_PA_GPIO		(0xE0300000)
#define S5PC1XX_PA_VIC0		(0xE4000000)
#define S5PC1XX_PA_VIC1		(0xE4100000)
#define S5PC1XX_PA_VIC2		(0xE4200000)
#define S5PC1XX_PA_SROMC	(0xE7000000)
#define S5PC1XX_PA_LCD	   	(0xEE000000)
#define S5PC1XX_PA_SYSTIMER   	(0xEA100000)
#define S5PC1XX_PA_ADC		(0xF3000000)
#define S5PC1XX_PA_RTC		(0xEA300000)
#define S5PC1XX_PA_IIS	   	(0xF2000000)
#define S5PC1XX_PA_NAND         (0xE7200000)
#define S5PC1XX_PA_USBHOST      (0xED400000)
#define S5PC1XX_PA_OTG          (0xED200000)
#define S5PC1XX_PA_OTGSFR       (0xED300000)
#define S5PC1XX_PA_FIMC0	(0xEE200000)
#define S5PC1XX_PA_FIMC1	(0xEE300000)
#define S5PC1XX_PA_FIMC2	(0xEE400000)
#define S5PC1XX_PA_DMA   	(0xE8100000)
#define S5PC1XX_PA_HSMMC(x)     (0xED800000 + ((x) * 0x100000))
#define S5PC1XX_PA_SPI0         (0xEC300000)
#define S5PC1XX_PA_SPI1         (0xEC400000)
#define S5PC1XX_PA_SPI2         (0xEC500000)
#define S5PC1XX_PA_KEYPAD	(0xF3100000)
#define S5PC1XX_PA_WDT          (0xEA200000)
#define S5PC1XX_PA_MFC		(0xF1000000)
#define S5PC1XX_PA_JPEG		(0xEE500000)
#define S5PC1XX_PA_AC97         (0xF2300000)
#define S5PC1XX_PA_G3D		(0xEF000000)
#define S5PC1XX_PA_ROTATOR	(0xEE100000)
#define S5PC1XX_PA_IEC		(0xE1100000)
#define S5PC1XX_PA_APC		(0xE1000000)

#define S5PC1XX_PA_SPDIF        (0xF2600000)            //add kwak for spdif
#define S5PC1XX_SZ_SPDIF        SZ_256                  //add kwak for spdif

/* UART */
#define S3C_PA_UART		S5PC1XX_PA_UART
#define S3C_PA_UART0		(S3C_PA_UART + 0x00)
#define S3C_PA_UART1		(S3C_PA_UART + 0x400)
#define S3C_PA_UART2		(S3C_PA_UART + 0x800)
#define S3C_PA_UART3		(S3C_PA_UART + 0xC00)
#define S3C_UART_OFFSET		(0x400)

/* See notes on UART VA mapping in debug-macro.S */
#define S3C_VA_UARTx(x)		(S3C_VA_UART + (S3C_PA_UART & 0xfffff) + ((x) * S3C_UART_OFFSET))

#define S3C_VA_UART0		S3C_VA_UARTx(0)
#define S3C_VA_UART1		S3C_VA_UARTx(1)
#define S3C_VA_UART2		S3C_VA_UARTx(2)
#define S3C_VA_UART3		S3C_VA_UARTx(3)

/* SYSCON */
#define S5PC1XX_PA_SYSCON	(0xE0100000)
#define S5PC1XX_VA_SYSCON       S3C_VA_SYS
#define S5PC1XX_SZ_SYSCON       SZ_2M  

/* GPIO */
#define S5PC1XX_VA_GPIO		S3C_ADDR(0x00500000)
#define S5PC1XX_SZ_GPIO		SZ_4K

/* SDRAM */
#define S5PC1XX_PA_SDRAM	(0x20000000)

/* SROMC */
#define S5PC1XX_VA_SROMC	S3C_VA_SROMC
#define S5PC1XX_SZ_SROMC	SZ_4K

/* LCD-FIMD */
#define S5PC1XX_VA_LCD	   	S3C_VA_LCD
#define S5PC1XX_SZ_LCD		SZ_1M

/* G2D */
#define S5PC1XX_PA_G2D	   	(0xEE800000)
#define S5PC1XX_SZ_G2D		SZ_1M

/* SYSTEM TIMER */
#define S5PC1XX_VA_SYSTIMER   	S3C_VA_SYSTIMER
#define S5PC1XX_SZ_SYSTIMER	SZ_1M

/* SMC9115 */
#define S5PC1XX_PA_SMC9115	(0x98000000)

/* I2S */
#define S3C_SZ_IIS		SZ_4K

/* CHIP ID */
#define S5PC1XX_PA_CHIPID	(0xE0000000)
#define S5PC1XX_VA_CHIPID	S3C_ADDR(0x00700000)

/* NAND flash controller */
#define S5PC1XX_VA_NAND		S3C_VA_NAND
#define S5PC1XX_SZ_NAND         SZ_1M

/* place VICs close together */
#define S3C_VA_VIC0		(S3C_VA_IRQ + 0x0)
#define S3C_VA_VIC1		(S3C_VA_IRQ + 0x10000)
#define S3C_VA_VIC2		(S3C_VA_IRQ + 0x20000)
#define S3C_VA_VIC3		(S3C_VA_IRQ + 0x30000)

/* USB Host */
#define S5PC1XX_SZ_USBHOST      SZ_1M

/* USB OTG */
#define S5PC1XX_VA_OTG          S3C_ADDR(0x00E00000)
#define S5PC1XX_SZ_OTG          SZ_1M

/* USB OTG SFR */
#define S5PC1XX_VA_OTGSFR       S3C_ADDR(0x00F00000)
#define S5PC1XX_SZ_OTGSFR       SZ_1M

/* HSMMC units */
#define S5PC1XX_PA_HSMMC0       S5PC1XX_PA_HSMMC(0)
#define S5PC1XX_PA_HSMMC1       S5PC1XX_PA_HSMMC(1)
#define S5PC1XX_PA_HSMMC2       S5PC1XX_PA_HSMMC(2)
#define S5PC1XX_SZ_HSMMC        SZ_1M

#define S3C_PA_HSMMC0           S5PC1XX_PA_HSMMC0
#define S3C_PA_HSMMC1           S5PC1XX_PA_HSMMC1
#define S3C_PA_HSMMC2           S5PC1XX_PA_HSMMC2


#define S3C_PA_SPI0		S5PC1XX_PA_SPI0
#define S3C_PA_SPI1		S5PC1XX_PA_SPI1
#define S3C_PA_SPI2		S5PC1XX_PA_SPI2
#define S3C_SZ_SPI0		SZ_4K
#define S3C_SZ_SPI1		SZ_4K
#define S3C_SZ_SPI2		SZ_4K

/* ONENAND CON*/
#define S5PC1XX_PA_ONENAND      (0xE7100000)
#define S5PC1XX_SZ_ONENAND      SZ_1M

/* KEYPAD IF */
#define S5PC1XX_SZ_KEYPAD	SZ_4K

#define S3C_PA_IIS		S5PC1XX_PA_IIS
#define S3C_PA_ADC		S5PC1XX_PA_ADC
#define S3C_PA_DMA		S5PC1XX_PA_DMA
#define S3C_PA_RTC              S5PC1XX_PA_RTC
#define S3C_PA_KEYPAD           S5PC1XX_PA_KEYPAD
#define S3C_SZ_KEYPAD           S5PC1XX_SZ_KEYPAD

/* WATCHDOG TIMER*/
#define S3C_PA_WDT              S5PC1XX_PA_WDT

/* MFC V4.0 */
#define S5PC1XX_SZ_MFC		SZ_4K

/* JPEG */
#define S5PC1XX_SZ_JPEG		SZ_1M

/* AC97 */
#define S5PC1XX_SZ_AC97         SZ_1M

/* G3D */
#define S5PC1XX_SZ_G3D		SZ_16M

/* spdif */
#define S5P_PA_SPDIF            S5PC1XX_PA_SPDIF        

/* Rotator */
#define S5PC1XX_SZ_ROTATOR	SZ_1M

/* MIPI CSIS */
#define S5PC1XX_PA_CSIS		(0xECC00000)
#define S5PC1XX_SZ_CSIS		SZ_1M

/* compatibiltiy defines. */
#define S3C_SZ_HSMMC		S5PC1XX_SZ_HSMMC

#define S3C_PA_TIMER		S5PC1XX_PA_TIMER
#define S3C_PA_IIC		S5PC1XX_PA_IIC0
#define S3C_PA_IIC1		S5PC1XX_PA_IIC1

#define S3C_VA_OTG              S5PC1XX_VA_OTG
#define S3C_PA_OTG              S5PC1XX_PA_OTG
#define S3C_SZ_OTG              S5PC1XX_SZ_OTG

#define S3C_VA_OTGSFR           S5PC1XX_VA_OTGSFR
#define S3C_PA_OTGSFR           S5PC1XX_PA_OTGSFR
#define S3C_SZ_OTGSFR           S5PC1XX_SZ_OTGSFR


#define S5PC1XX_VA_IEC		S3C_ADDR(0x00800000)

#define S5PC1XX_VA_APC		S3C_ADDR(0x00900000)


//[mkh: for tvout/hdmi
#define S5PC1XX_PA_TVOUT        (0xF0000000)
#define S5PC1XX_SZ_TVOUT        SZ_4M
#define S5PC1XX_PA_CLK_OTHER    (S5PC1XX_PA_SYSCON + 0x100000)
#define S5PC1XX_SZ_CLK_OTHER    SZ_4K
//]


#endif /* __ASM_ARCH_6400_MAP_H */
