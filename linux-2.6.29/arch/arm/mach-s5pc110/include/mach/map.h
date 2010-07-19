/* linux/arch/arm/mach-s5pc110/include/mach/map.h
 *
 * Copyright 2008 Samsung Electronics Co.
 * Copyright 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S5PC11X - Memory map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_MAP_H
#define __ASM_ARCH_MAP_H __FILE__

#include <plat/map-base.h>

#define S5PC110_PA_UART		(0xE2900000)
#define S5PC11X_PA_UART		S5PC110_PA_UART
#define S5PC11X_PA_TIMER	(0xE2500000)
#define S5PC11X_PA_IIC0		(0xE1800000)
#define S5PC11X_PA_IIC1		(0xFAB00000)
#define S5PC11X_PA_IIC2		(0xE1A00000)
#define S5PC11X_PA_IIC_HDMI_PHY 	(0xFA900000)
#define S5PC11X_PA_GPIO		(0xE0200000)
#define S5PC11X_PA_CFCON	(0xE8200000)
#define S5PC11X_PA_VIC0		(0xF2000000)
#define S5PC11X_PA_VIC1		(0xF2100000)
#define S5PC11X_PA_VIC2		(0xF2200000)
#define S5PC11X_PA_VIC3		(0xF2300000)
#define S5PC11X_PA_SROMC	(0xE8000000)
#define S5PC11X_PA_LCD	   	(0xF8000000)
#define S5PC11X_PA_SYSTIMER   	(0xE2600000)
#define S5PC11X_PA_ADC		(0xE1700000)
#define S5PC11X_PA_ADC1		(0xE1701000)
#define S5PC11X_PA_RTC		(0xE2800000)
#define S5PC1XX_PA_IIS_V32   	(0xE2100000)
#define S5PC1XX_PA_IIS_V50   	(0xEEE30000)
#define S5PC11X_PA_AUDSS   	(0xEEE10000)
#define S5PC11X_PA_NAND         (0xB0E00000)
#define S5PC11X_PA_USBHOST      (0xEC300000)
#define S5PC11X_PA_OTG          (0xEC000000)

#define S5PC11X_PA_OTGSFR       (0xEC100000)
#define S5PC11X_PA_FIMC0	(0xFB200000)
#define S5PC11X_PA_FIMC1	(0xFB300000)
#define S5PC11X_PA_FIMC2	(0xFB400000)
#define S5PC11X_PA_DMA   	(0xFA200000)
#define S5PC11X_PA_PDMA   	(0xE0900000)
#define S5PC11X_PA_HSMMC(x)     (0xEB000000 + ((x) * 0x100000))
#define S5PC11X_PA_SPI0         (0xE1300000)
#define S5PC11X_PA_SPI1         (0xE1400000)
#define S5PC11X_PA_SPI2         (0xE1500000)
#define S5PC11X_PA_KEYPAD	(0xE1600000)
#define S5PC11X_PA_WDT          (0xE2700000)
#define S5PC11X_PA_MFC		(0xF1700000)
#define S5PC11X_PA_JPEG		(0xFB600000)
#define S5PC11X_PA_AC97         (0xE2200000)
#define S5PC11X_PA_G3D		(0xF3000000)
#define S5PC11X_PA_ROTATOR	(0xFB100000)
#define S5PC11X_PA_IPC	(0xFB700000)

/* UART */
#define S3C_PA_UART		S5PC11X_PA_UART
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
#define S5PC11X_PA_SYSCON	(0xE0100000)
#define S5PC11X_VA_SYSCON       S3C_VA_SYS
#define S5PC11X_SZ_SYSCON       SZ_2M  

/* SWRESET */
#define S5PC11X_PA_SWRESET	(0xE0102000)
#define S5PC11X_SZ_SWRESET	SZ_16

/* GPIO */
#define S5PC11X_VA_GPIO		S3C_ADDR(0x00500000)
#define S5PC11X_SZ_GPIO		SZ_4K

/* CFCON */
#define S5PC11X_SZ_CFCON	SZ_1M

/* SDRAM */
#define S5PC11X_PA_SDRAM	(0x30000000)

/* SROMC */
#define S5PC11X_VA_SROMC	S3C_VA_SROMC
#define S5PC11X_SZ_SROMC	SZ_4K

/* LCD-FIMD */
#define S5PC11X_VA_LCD	   	S3C_VA_LCD
#define S5PC11X_SZ_LCD		SZ_1M

/* G2D */
#define S5PC11X_PA_G2D	   	(0xEE800000)
#define S5PC11X_SZ_G2D		SZ_1M

/* SYSTEM TIMER */
#define S5PC11X_VA_SYSTIMER   	S3C_VA_SYSTIMER
#define S5PC11X_SZ_SYSTIMER	SZ_1M

/* SMC9115 */
#define S5PC11X_PA_DM9000	(0xA0000000)

/* Audio SubSystem */
#define S5PC11X_VA_AUDSS   	S3C_VA_AUDSS
#define S5PC11X_SZ_AUDSS	SZ_1M

/* I2S */
#define S3C_SZ_IIS		SZ_4K

/*PCM*/
#define S5PC11X_PA_PCM0         (0xE2300000)
#define S5PC11X_PA_PCM1         (0xE1200000)
#define S5PC11X_PA_PCM2         (0xE2B00000)

/* CHIP ID */
#define S5PC11X_PA_CHIPID	(0xE0000000)
#define S5PC11X_VA_CHIPID	S3C_ADDR(0x00700000)

/* NAND flash controller */
#define S5PC11X_VA_NAND		S3C_VA_NAND
#define S5PC11X_SZ_NAND         SZ_1M

/* place VICs close together */
#define S3C_VA_VIC0		(S3C_VA_IRQ + 0x0)
#define S3C_VA_VIC1		(S3C_VA_IRQ + 0x10000)
#define S3C_VA_VIC2		(S3C_VA_IRQ + 0x20000)
#define S3C_VA_VIC3		(S3C_VA_IRQ + 0x30000)

/* USB Host */
#define S5PC11X_SZ_USBHOST      SZ_1M

/* USB OTG */
#define S5PC11X_VA_OTG          S3C_ADDR(0x00E00000)
#define S5PC11X_SZ_OTG          SZ_1M

/* USB OTG SFR */
#define S5PC11X_VA_OTGSFR       S3C_ADDR(0x00F00000)
#define S5PC11X_SZ_OTGSFR       SZ_1M

/* HSMMC units */
#define S5PC11X_PA_HSMMC0       S5PC11X_PA_HSMMC(0)
#define S5PC11X_PA_HSMMC1       S5PC11X_PA_HSMMC(1)
#define S5PC11X_PA_HSMMC2       S5PC11X_PA_HSMMC(2)
#define S5PC11X_PA_HSMMC3       S5PC11X_PA_HSMMC(3)
#define S5PC11X_SZ_HSMMC        SZ_1M

#define S3C_PA_HSMMC0           S5PC11X_PA_HSMMC0
#define S3C_PA_HSMMC1           S5PC11X_PA_HSMMC1
#define S3C_PA_HSMMC2           S5PC11X_PA_HSMMC2
#define S3C_PA_HSMMC3           S5PC11X_PA_HSMMC3

#define S3C_PA_SPI0		S5PC11X_PA_SPI0
#define S3C_PA_SPI1		S5PC11X_PA_SPI1
#define S3C_PA_SPI2		S5PC11X_PA_SPI2
#define S3C_SZ_SPI0		SZ_4K
#define S3C_SZ_SPI1		SZ_4K
#define S3C_SZ_SPI2		SZ_4K

/* ONENAND CON*/
#define S5PC11X_PA_ONENAND      (0xE7100000)
#define S5PC11X_SZ_ONENAND      SZ_1M

/* KEYPAD IF */
#define S5PC11X_SZ_KEYPAD	SZ_4K

#define S3C_PA_IIS_V32		S5PC1XX_PA_IIS_V32
#define S3C_PA_IIS_V50		S5PC1XX_PA_IIS_V50
#define S3C_PA_ADC		S5PC11X_PA_ADC
#define S3C_PA_ADC1		S5PC11X_PA_ADC1
#define S3C_PA_DMA		S5PC11X_PA_DMA
#define S3C_PA_PDMA		S5PC11X_PA_PDMA
#define S3C_PA_RTC              S5PC11X_PA_RTC
#define S3C_PA_KEYPAD           S5PC11X_PA_KEYPAD
#define S3C_SZ_KEYPAD           S5PC11X_SZ_KEYPAD

/* WATCHDOG TIMER*/
#define S3C_PA_WDT              S5PC11X_PA_WDT

/* MFC V4.0 & V5.0 */
#define S5PC11X_SZ_MFC		SZ_1M

/* JPEG */
#define S5PC11X_SZ_JPEG		SZ_1M

/* AC97 */
#define S5PC11X_SZ_AC97         SZ_1M

/* G3D */
#define S5PC11X_SZ_G3D		SZ_16M

/* Rotator */
#define S5PC11X_SZ_ROTATOR	SZ_1M

/* MIPI CSIS */
#define S5PC11X_PA_CSIS		(0xFA600000)
#define S5PC11X_SZ_CSIS		SZ_1M

/* IPC */
#define S5PC11X_SZ_IPC		SZ_1M

/* TVOUT */
#define S5PC11X_PA_TVENC        (0xF9000000)
#define S5PC11X_SZ_TVENC        SZ_1M
#define S5PC11X_PA_VP           (0xF9100000)
#define S5PC11X_SZ_VP           SZ_1M
#define S5PC11X_PA_MIXER        (0xF9200000)
#define S5PC11X_SZ_MIXER        SZ_1M
#define S5PC11X_PA_HDMI         (0xFA100000)
#define S5PC11X_SZ_HDMI         SZ_1M
#define I2C_HDMI_PHY_BASE	(0xFA900000)
#define I2C_HDMI_SZ_PHY_BASE	SZ_1K

/* compatibiltiy defines. */
#define S3C_SZ_HSMMC		S5PC11X_SZ_HSMMC

#define S3C_PA_TIMER		S5PC11X_PA_TIMER

#define S3C_PA_IIC		S5PC11X_PA_IIC0
#define S3C_PA_IIC1		S5PC11X_PA_IIC1
#define S3C_PA_IIC2		S5PC11X_PA_IIC2
#define S3C_PA_IIC_HDMI_PHY 	S5PC11X_PA_IIC_HDMI_PHY

#define S3C_VA_OTG              S5PC11X_VA_OTG
#define S3C_PA_OTG              S5PC11X_PA_OTG
#define S3C_SZ_OTG              S5PC11X_SZ_OTG

#define S3C_VA_OTGSFR           S5PC11X_VA_OTGSFR
#define S3C_PA_OTGSFR           S5PC11X_PA_OTGSFR
#define S3C_SZ_OTGSFR           S5PC11X_SZ_OTGSFR


#define S5PC11X_VA_IEC		S3C_ADDR(0x00800000)

#define S5PC11X_VA_APC		S3C_ADDR(0x00900000)

#define S5PC11X_PA_SPDIF        (0xE1100000)            
#define S5P_PA_SPDIF    	S5PC11X_PA_SPDIF
#define S5P_SZ_SPDIF 		SZ_256                 

#define S5PC11X_PA_TSI        (0xEB400000)            
#define S5P_PA_TSI	       S5PC11X_PA_TSI
#define S5P_SZ_TSI 		SZ_256                 

#define S5P_PA_DMC0		(0xF0000000)
#define S5P_VA_DMC0		S3C_ADDR(0x00a00000)

#define S5P_PA_DMC1		(0xF1400000)
#define S5P_VA_DMC1		S3C_ADDR(0x00b00000)

#define S3C_VA_RTC		S3C_ADDR(0x00c00000)




#define ONEDRAM1G_SHARED_AREA_PHYS  0x35000000
#define ONEDRAM1G_SHARED_AREA_VIRT      0xFC800000
#define ONEDRAM1G_SHARED_AREA_SIZE      (SZ_4M*4)



#endif /* __ASM_ARCH_6400_MAP_H */
