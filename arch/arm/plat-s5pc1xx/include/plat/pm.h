/* linux/include/asm-arm/plat-s3c24xx/pm.h
 *
 * Copyright (c) 2004 Simtec Electronics
 *	Written by Ben Dooks, <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifdef CONFIG_PM

extern __init int s5pc1xx_pm_init(void);

#else

static inline int s5pc1xx_pm_init(void)
{
	return 0;
}
#endif

/* configuration for the IRQ mask over sleep */
extern unsigned long s5pc1xx_irqwake_intmask;
extern unsigned long s5pc1xx_irqwake_eintmask;

/* IRQ masks for IRQs allowed to go to sleep (see irq.c) */
extern unsigned long s5pc1xx_irqwake_intallow;
extern unsigned long s5pc1xx_irqwake_eintallow;

/* per-cpu sleep functions */

extern void (*pm_cpu_prep)(void);
extern void (*pm_cpu_sleep)(void);

/* Flags for PM Control */

extern unsigned long s5pc100_pm_flags;

/* from sleep.S */

extern int  s5pc100_cpu_save(unsigned long *saveblk);
extern void s5pc100_cpu_suspend(void);
extern void s5pc100_cpu_resume(void);

extern unsigned long s5pc100_sleep_save_phys;

/* sleep save info */

struct sleep_save {
	void __iomem	*reg;
	unsigned long	val;
};

struct sleep_save_phy {
	unsigned long	reg;
	unsigned long	val;
};

#define SAVE_ITEM(x) \
	{ .reg = (x) }

extern void s5pc1xx_pm_do_save_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count);
extern void s5pc1xx_pm_do_restore_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count);
extern void s5pc1xx_pm_do_save(struct sleep_save *ptr, int count);
extern void s5pc1xx_pm_do_restore(struct sleep_save *ptr, int count);

#ifdef CONFIG_PM
extern int s5pc1xx_irq_suspend(struct sys_device *dev, pm_message_t state);
extern int s5pc1xx_irq_resume(struct sys_device *dev);
#else
#define s5pc1xx_irq_suspend	NULL
#define s5pc1xx_irq_resume	NULL
#endif
