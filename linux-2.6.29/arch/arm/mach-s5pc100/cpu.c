/* linux/arch/arm/mach-s5pc100/cpu.c
 *
 * Copyright 2008 Samsung Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/proc-fns.h>
#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/idle.h>
#include <mach/map.h>

#include <plat/cpu-freq.h>
#include <plat/regs-serial.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/sdhci.h>
#include <plat/iic-core.h>
#include <plat/s5pc100.h>

#include <plat/regs-power.h>
#include <plat/regs-clock.h>

#undef T32_PROBE_DEBUGGING

#if defined(T32_PROBE_DEBUGGING)
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#endif

/* Initial IO mappings */

static struct map_desc s5pc100_iodesc[] __initdata = {
	IODESC_ENT(LCD),
	IODESC_ENT(SROMC),
	IODESC_ENT(SYSTIMER),
        IODESC_ENT(OTG),
        IODESC_ENT(OTGSFR),
        IODESC_ENT(SYSCON),
        IODESC_ENT(GPIO),
        IODESC_ENT(NAND),
	//IODESC_ENT(HOSTIFB),
};

/* s5pc100_map_io
 *
 * register the standard cpu IO areas
*/

static void s5pc100_idle(void)
{
#if !defined(CONFIG_MMC_SDHCI_S3C) && !defined(CONFIG_MMC_SDHCI_MODULE)
	unsigned int tmp;

#if defined(T32_PROBE_DEBUGGING)
/* debugging with T32  GPIO port GPD1 which is connected with 2 pin of J1 connector */
	gpio_direction_output(S5PC1XX_GPD(1), 0);
#endif
/*
 * 1. Set CFG_STANDBYWFI field of PWR_CFG to 2¡¯b01.
 * 2. Set PMU_INT_DISABLE bit of OTHERS register to 1¡¯b1 to prevent interrupts from
 *    occurring while entering IDLE mode.
 * 3. Execute Wait For Interrupt instruction (WFI).
*/
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	tmp |= S5P_CFG_WFI_IDLE;
	__raw_writel(tmp, S5P_PWR_CFG);

	cpu_do_idle();

#if defined(T32_PROBE_DEBUGGING)
	gpio_direction_output(S5PC1XX_GPD(1), 1);
#endif
#endif
}

/* set_gpio_monitor_cpuidle
 *
 * set gpio output to monitor cpuidle operation
*/
#if defined(CONFIG_CPU_IDLE_MONITORING)
static void set_gpio_monitor_cpuidle(void)
{
	unsigned int tmp;

	// To do cpuidle monitoring
	tmp = __raw_readl(S5PC1XX_GPDCON) & ~(0xF << 4); // set GPD[1] to output
	tmp |= (0x1 << 4);
	__raw_writel(tmp, S5PC1XX_GPDCON);
	tmp = __raw_readl(S5PC1XX_GPDPUD) & ~(0x3 << 2); // disable pull-up down
	__raw_writel(tmp, S5PC1XX_GPDPUD);
	
	// To do LED blinking
	tmp = __raw_readl(S5PC1XX_GPH1CON) & ~(0xF << 16); // set GPH1[4] to output
	tmp |= (0x1 << 16);
	__raw_writel(tmp, S5PC1XX_GPH1CON);
	tmp = __raw_readl(S5PC1XX_GPH1PUD) & ~(0x3 << 8); // disable pull-up down
	__raw_writel(tmp, S5PC1XX_GPH1PUD);
}
#endif

/* s5pc100_map_io
 *
 * register the standard cpu IO areas
*/
void __init s5pc100_map_io(void)
{
	iotable_init(s5pc100_iodesc, ARRAY_SIZE(s5pc100_iodesc));

        /* HS-MMC Platform data */

        s3c6410_default_sdhci0();
        s3c6410_default_sdhci1();

	/* set s5pc100 idle function */

	s5pc1xx_idle = s5pc100_idle;


#if 0
	/* the i2c devices are directly compatible with s3c2440 */
	s3c_i2c0_setname("s3c2440-i2c");
	s3c_i2c1_setname("s3c2440-i2c");
#endif
#if defined(CONFIG_CPU_IDLE_MONITORING)
	set_gpio_monitor_cpuidle();
#endif
}

void __init s5pc100_init_clocks(int xtal)
{
	printk(KERN_DEBUG "%s: initialising clocks\n", __func__);
	s3c24xx_register_baseclocks(xtal);
	s5pc1xx_register_clocks();
	s5pc100_register_clocks();
	s5pc100_setup_clocks();
#if defined(CONFIG_HAVE_PWM)
        s3c_pwmclk_init();
#endif
}

void __init s5pc100_init_irq(void)
{
	/* VIC0, VIC1, and VIC2 are fully populated. */
	s5pc1xx_init_irq(~0, ~0, ~0, 0);
}

struct sysdev_class s5pc100_sysclass = {
	.name	= "s5pc100-core",
};

static struct sys_device s5pc100_sysdev = {
	.cls	= &s5pc100_sysclass,
};

static int __init s5pc100_core_init(void)
{
	return sysdev_class_register(&s5pc100_sysclass);
}

core_initcall(s5pc100_core_init);

int __init s5pc100_init(void)
{
	printk("S5PC100: Initialising architecture\n");

#if defined (CONFIG_S3C_SIR)
        s3c24xx_uart_src[3]->name = "s3c-irda";
#endif

	return sysdev_register(&s5pc100_sysdev);
}
