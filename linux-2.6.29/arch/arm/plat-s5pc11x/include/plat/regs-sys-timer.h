/* arch/arm/plat-s5pc1xx/include/plat/regs-sys-timer.h
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S5PC1XX System Timer configuration
*/

#ifndef __ASM_ARCH_REGS_SYS_TIMER_H
#define __ASM_ARCH_REGS_SYS_TIMER_H

#define S3C_SYSTIMERREG(x)		(S3C_VA_SYSTIMER + (x))

#define S3C_SYSTIMER_TCFG		S3C_SYSTIMERREG(0x00)
#define S3C_SYSTIMER_TCON		S3C_SYSTIMERREG(0x04)
#define S3C_SYSTIMER_TCNTB		S3C_SYSTIMERREG(0x08)
#define S3C_SYSTIMER_TCNTO		S3C_SYSTIMERREG(0x0c)

#define S3C_SYSTIMER_TFCNTB		S3C_SYSTIMERREG(0x10)
#define S3C_SYSTIMER_ICNTB		S3C_SYSTIMERREG(0x18)
#define S3C_SYSTIMER_ICNTO		S3C_SYSTIMERREG(0x1c)
#define S3C_SYSTIMER_INT_CSTAT		S3C_SYSTIMERREG(0x20)

/* Value for TCFG */
#define S3C_SYSTIMER_TCLK_MASK		(3<<12)
#define S3C_SYSTIMER_TCLK_XXTI		(0<<12)
#define S3C_SYSTIMER_TCLK_RTC		(1<<12)
#define S3C_SYSTIMER_TCLK_USB		(2<<12)
#define S3C_SYSTIMER_TCLK_PCLK		(3<<12)

#define S3C_SYSTIMER_DIV_MASK		(7<<8)
#define S3C_SYSTIMER_DIV_1		(0<<8)
#define S3C_SYSTIMER_DIV_2		(1<<8)
#define S3C_SYSTIMER_DIV_4		(2<<8)
#define S3C_SYSTIMER_DIV_8		(3<<8)
#define S3C_SYSTIMER_DIV_16		(4<<8)

#define S3C_SYSTIMER_TARGET_HZ		1000
#define S3C_SYSTIMER_PRESCALER		5
#define S3C_SYSTIMER_PRESCALER_MASK	(0x3f<<0)

/* value for TCON */
#define S3C_SYSTIMER_INT_AUTO		(1<<5)
#define S3C_SYSTIMER_INT_IMM		(1<<4)
#define S3C_SYSTIMER_INT_START		(1<<3)
#define S3C_SYSTIMER_AUTO_RELOAD	(1<<2)
#define S3C_SYSTIMER_IMM_UPDATE		(1<<1)
#define S3C_SYSTIMER_START		(1<<0)

/* Value for INT_CSTAT */
#define S3C_SYSTIMER_INT_IWIE		(1<<9)
#define S3C_SYSTIMER_INT_TWIE		(1<<10)
#define S3C_SYSTIMER_INT_ICNTEIE	(1<<6)
#define S3C_SYSTIMER_INT_TCON		(1<<5)
#define S3C_SYSTIMER_INT_ICNTB		(1<<4)
#define S3C_SYSTIMER_INT_TCNTB		(1<<2)
#define S3C_SYSTIMER_INT_STATS		(1<<1)
#define S3C_SYSTIMER_INT_EN		(1<<0)

#endif /*  __ASM_ARCH_REGS_TIMER_H */
