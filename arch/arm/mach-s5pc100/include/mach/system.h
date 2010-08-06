/* linux/arch/arm/mach-s5pc100/include/mach/system.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5PC100 - system implementation
 */

 #include <mach/idle.h>

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H __FILE__
// 2009.06.02 added by hyunkyung
#include <linux/io.h>
#include <asm/sizes.h>

void (*s5pc1xx_idle)(void);

void s5pc1xx_default_idle(void)
{
	printk("default idle function\n");
}

static void arch_idle(void)
{
	if(s5pc1xx_idle != NULL)
		(s5pc1xx_idle)();
	else
		s5pc1xx_default_idle();
}

static void arch_reset(char mode)
{
	// 2009.06.02 added by hyunkyung
	void __iomem *s5p_clock_controll_base;

	s5p_clock_controll_base = ioremap(0xE0200000, SZ_64K); 
	if (s5p_clock_controll_base == NULL) {
                printk("failed to ioremap() region in arch_reset()\n");
                return;
        }

	printk("arch_reset() is called\n");
	if (mode == 's') {
                cpu_reset(0);
        }

        /*if (s5pc1xx_reset_hook)
                s5pc1xx_reset_hook();*/

	// set S5P_SWREST register to 0xC100 to assert reset
        __raw_writel(0xC100, s5p_clock_controll_base);

        /* wait for reset to assert... */
        mdelay(5000);

        printk(KERN_ERR "Watchdog reset failed to assert reset\n");

        /* we'll take a jump through zero as a poor second */
        cpu_reset(0);
	
}

#endif /* __ASM_ARCH_IRQ_H */
