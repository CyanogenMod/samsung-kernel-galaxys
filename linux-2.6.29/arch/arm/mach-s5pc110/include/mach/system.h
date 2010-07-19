/* linux/arch/arm/mach-s5pc110/include/mach/system.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5PC110 - system implementation
 */

#include <mach/idle.h>

#include <mach/map.h>
#include <linux/clk.h>

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H __FILE__

void (*s5pc11x_idle)(void);

void s5pc11x_default_idle(void)
{
	printk("default idle function\n");
}

static void arch_idle(void)
{
	if(s5pc11x_idle != NULL)
		(s5pc11x_idle)();
	else
		s5pc11x_default_idle();
}

static void arch_reset(char mode)
{
#if 0	
	void __iomem * swreset ;

	swreset = ioremap(S5PC11X_PA_SWRESET,0x400);
	printk(" arch_reset()\n");
	
	if (swreset == NULL) 
	{
                printk("failed to ioremap() region in arch_reset()\n");
                return ;
        }
        
	__raw_writel(0x1,swreset);
	
	mdelay(5000);

#else
	int ret;
	void __iomem * wdt ;
	/*  Watch Reset is used */
	static struct clk		*wdt_clock;
	wdt_clock = clk_get(NULL, "watchdog");
	if (IS_ERR(wdt_clock)) {
		printk("failed to find watchdog clock source\n");
		ret = PTR_ERR(wdt_clock);
                return ;
	}

	clk_enable(wdt_clock);

	wdt = ioremap(S3C_PA_WDT,0x400);
        
        if (wdt == NULL) 
        {
                printk("failed to ioremap() region in arch_reset()\n");
                return ;
        }
        
        __raw_writel(0x1,wdt +0x4 );
        __raw_writel(0x8021,wdt);
	
        /* Never happened */
        mdelay(5000);
	
#endif

}

#endif /* __ASM_ARCH_IRQ_H */
