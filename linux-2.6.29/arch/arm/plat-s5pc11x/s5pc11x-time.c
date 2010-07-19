/* linux/arch/arm/plat-s5pc11x/s5pc11x-time.c
 *
 * Copyright (C) 2003-2005 Simtec Electronics
 *	Jongpill Lee, <boyko.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * This code is based on plat-s3c/time.c
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/leds.h>
#include <asm/mach-types.h>

#include <asm/irq.h>
#include <asm/mach/time.h>

#include <mach/map.h>
#include <mach/regs-irq.h>
#include <mach/tick.h>

#include <plat/regs-sys-timer.h>
#include <plat/clock.h>
#include <plat/cpu.h>

#include <plat/regs-gpio.h>
#include <plat/gpio-bank-a1.h>

#include <plat/gpio-bank-d.h>
#include <plat/regs-clock.h>

static unsigned long timer_startval;
static unsigned long timer_usec_ticks;
static unsigned long timer_icnt;

#ifndef TICK_MAX
#define TICK_MAX (0xffff)
#endif

#define TIMER_USEC_SHIFT 16

static unsigned int s5pc11x_systimer_read(unsigned int *reg_offset)
{
	return __raw_readl(reg_offset);
}

static unsigned int s5pc11x_systimer_write(unsigned int *reg_offset, unsigned int value)
{
	unsigned int temp_regs;

	__raw_writel(value, reg_offset);

	if (reg_offset == S3C_SYSTIMER_TCON) {
		while(!(__raw_readl(S3C_SYSTIMER_INT_CSTAT) & S3C_SYSTIMER_INT_TCON));
		temp_regs = __raw_readl(S3C_SYSTIMER_INT_CSTAT);
		temp_regs |= S3C_SYSTIMER_INT_TCON;
		__raw_writel(temp_regs, S3C_SYSTIMER_INT_CSTAT);

	} else if (reg_offset == S3C_SYSTIMER_ICNTB) {
		while(!(__raw_readl(S3C_SYSTIMER_INT_CSTAT) & S3C_SYSTIMER_INT_ICNTB));
		temp_regs = __raw_readl(S3C_SYSTIMER_INT_CSTAT);
		temp_regs |= S3C_SYSTIMER_INT_ICNTB;
		__raw_writel(temp_regs, S3C_SYSTIMER_INT_CSTAT);

	} else if (reg_offset == S3C_SYSTIMER_TCNTB) {
		while(!(__raw_readl(S3C_SYSTIMER_INT_CSTAT) & S3C_SYSTIMER_INT_TCNTB));
		temp_regs = __raw_readl(S3C_SYSTIMER_INT_CSTAT);
		temp_regs |= S3C_SYSTIMER_INT_TCNTB;
		__raw_writel(temp_regs, S3C_SYSTIMER_INT_CSTAT);
	}

	return 0;
}

/*
 * S5PC11X has system timer to use as OS tick Timer.
 * System Timer provides two distincive feature. Accurate timer which provides
 * exact 1ms time tick at any power mode except sleep mode. Second one is chageable
 * interrupt interval without stopping reference tick timer.
 */

/* 
 * timer_mask_usec_ticks
 *
 * given a clock and divisor, make the value to pass into timer_ticks_to_usec
 * to scale the ticks into usecs
 */
static inline unsigned long timer_mask_usec_ticks(unsigned long scaler, unsigned long pclk)
{
	unsigned long den = pclk / 1000;

	return ((1000 << TIMER_USEC_SHIFT) * scaler + (den >> 1)) / den;
}

/* 
 * timer_ticks_to_usec
 *
 * convert timer ticks to usec.
 */
static inline unsigned long timer_ticks_to_usec(unsigned long ticks)
{
	unsigned long res;

	res = ticks * timer_usec_ticks;
	res += 1 << (TIMER_USEC_SHIFT - 4);	/* round up slightly */

	return res >> TIMER_USEC_SHIFT;
}

/*
 * Returns microsecond  since last clock interrupt.  Note that interrupts
 * will have been disabled by do_gettimeoffset()
 * IRQs are disabled before entering here from do_gettimeofday()
 */
static unsigned long s5pc11x_gettimeoffset (void)
{
	unsigned long tdone;
	unsigned long tval;
	unsigned long clk_tick_totcnt;

	clk_tick_totcnt = (timer_icnt + 1) * timer_startval;

	/* work out how many ticks have gone since last timer interrupt */
	tval = s5pc11x_systimer_read(S3C_SYSTIMER_ICNTO) * timer_startval;
	tval += s5pc11x_systimer_read(S3C_SYSTIMER_TCNTO);

	tdone = clk_tick_totcnt - tval;

	/* check to see if there is an interrupt pending */
	if (s5pc11x_ostimer_pending()) {
		/* re-read the timer, and try and fix up for the missed
		 * interrupt. Note, the interrupt may go off before the
		 * timer has re-loaded from wrapping.
		 */

		tval = s5pc11x_systimer_read(S3C_SYSTIMER_ICNTO) * timer_startval;
		tval += s5pc11x_systimer_read(S3C_SYSTIMER_TCNTO);

		tdone = clk_tick_totcnt - tval;

		if (tval != 0)
			tdone += clk_tick_totcnt;
	}

	return timer_ticks_to_usec(tdone);
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t s5pc11x_timer_interrupt(int irq, void *dev_id)
{
	volatile unsigned int temp_cstat;

	temp_cstat = s5pc11x_systimer_read(S3C_SYSTIMER_INT_CSTAT);
	temp_cstat |= S3C_SYSTIMER_INT_STATS;

	s5pc11x_systimer_write(S3C_SYSTIMER_INT_CSTAT, temp_cstat);
#ifndef CONFIG_GENERIC_CLOCKEVENTS
	timer_tick();
#endif
	return IRQ_HANDLED;
}

static struct irqaction s5pc11x_timer_irq = {
	.name		= "S5PC11X System Timer",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= s5pc11x_timer_interrupt,
};

/*
 * Set up timer interrupt, and return the current time in seconds.
 *
 */
static void s5pc11x_timer_setup (void)
{
	unsigned long tcon;
	unsigned long tcnt;
	unsigned long tcfg;

	/* clock configuration setting and enable */
	unsigned long pclk;
	struct clk *clk;

	tcnt = TICK_MAX;  /* default value for tcnt */

	/* initialize system timer clock */
	tcfg = s5pc11x_systimer_read(S3C_SYSTIMER_TCFG);

	tcfg &= ~S3C_SYSTIMER_TCLK_MASK;
	tcfg |= S3C_SYSTIMER_TCLK_PCLK;

	s5pc11x_systimer_write(S3C_SYSTIMER_TCFG, tcfg);

	/* TCFG must not be changed at run-time. If you want to change TCFG, stop timer(TCON[0] = 0) */
	s5pc11x_systimer_write(S3C_SYSTIMER_TCON, 0);

	/* read the current timer configuration bits */
	tcon = s5pc11x_systimer_read(S3C_SYSTIMER_TCON);
	tcfg = s5pc11x_systimer_read(S3C_SYSTIMER_TCFG);

	clk = clk_get(NULL, "systimer");
	if (IS_ERR(clk))
		panic("failed to get clock for system timer");

	clk_enable(clk);

	pclk = clk_get_rate(clk);

	/* configure clock tick */
	timer_usec_ticks = timer_mask_usec_ticks(S3C_SYSTIMER_PRESCALER, pclk);

	tcfg &= ~S3C_SYSTIMER_TCLK_MASK;
	tcfg |= S3C_SYSTIMER_TCLK_PCLK;
	tcfg &= ~S3C_SYSTIMER_PRESCALER_MASK;
	tcfg |= S3C_SYSTIMER_PRESCALER - 1;

	tcnt = ((pclk / S3C_SYSTIMER_PRESCALER) / S3C_SYSTIMER_TARGET_HZ) - 1;

	/* check to see if timer is within 16bit range... */
	if (tcnt > TICK_MAX) {
		panic("setup_timer: HZ is too small, cannot configure timer!");
		return;
	}

	s5pc11x_systimer_write(S3C_SYSTIMER_TCFG, tcfg);

	timer_startval = tcnt;
	s5pc11x_systimer_write(S3C_SYSTIMER_TCNTB, tcnt);

	/* set Interrupt tick value */
	timer_icnt = (S3C_SYSTIMER_TARGET_HZ / HZ) - 1;
	s5pc11x_systimer_write(S3C_SYSTIMER_ICNTB, timer_icnt);

	tcon = S3C_SYSTIMER_INT_AUTO | S3C_SYSTIMER_START | S3C_SYSTIMER_INT_START | S3C_SYSTIMER_AUTO_RELOAD;
	s5pc11x_systimer_write(S3C_SYSTIMER_TCON, tcon);

	printk("timer tcon=%08lx, tcnt %04lx, icnt %04lx, tcfg %08lx, usec %08lx\n",
	       tcon, tcnt, timer_icnt, tcfg, timer_usec_ticks);

	/* Interrupt Start and Enable */
	s5pc11x_systimer_write(S3C_SYSTIMER_INT_CSTAT, (S3C_SYSTIMER_INT_ICNTEIE|S3C_SYSTIMER_INT_EN));
}

static void __init s5pc11x_timer_init(void)
{
	s5pc11x_timer_setup();
	setup_irq(IRQ_SYSTIMER, &s5pc11x_timer_irq);
}

struct sys_timer s5pc11x_timer = {
	.init		= s5pc11x_timer_init,
#ifndef CONFIG_GENERIC_TIME
	.offset		= s5pc11x_gettimeoffset,
#endif
	.resume		= s5pc11x_timer_setup
};

