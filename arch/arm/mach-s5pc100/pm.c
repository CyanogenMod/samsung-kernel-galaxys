/* linux/arch/arm/mach-s5pc100/pm.c
 *
 * Copyright (c) 2006 Samsung Electronics
 *
 *
 * S3C6410 (and compatible) Power Manager (Suspend-To-RAM) support
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/sysdev.h>
#include <linux/io.h>

#include <mach/hardware.h>

#include <asm/mach-types.h>

#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>
#include <plat/cpu.h>
#include <plat/pm.h>

#define DBG(fmt...) printk(KERN_DEBUG fmt)

void s5pc100_cpu_suspend(void)
{
	unsigned long tmp;

	/* issue the standby signal into the pm unit. Note, we
	 * issue a write-buffer drain just in case */

	tmp = 0;
/*
 * MCR p15,0,<Rd>,c7,c10,5 ; Data Memory Barrier Operation.
 * MCR p15,0,<Rd>,c7,c10,4 ; Data Synchronization Barrier operation.
 * MCR p15,0,<Rd>,c7,c0,4 ; Wait For Interrupt.
 */

	asm("b 1f\n\t"
	    ".align 5\n\t"
	    "1:\n\t"
	    "mcr p15, 0, %0, c7, c10, 5\n\t"
	    "mcr p15, 0, %0, c7, c10, 4\n\t"
	    ".word 0xe320f003" :: "r" (tmp));

	/* we should never get past here */

	panic("sleep resumed to originator?");
}

static void s5pc100_pm_prepare(void)
{

}

static int s5pc100_pm_add(struct sys_device *sysdev)
{
	pm_cpu_prep = s5pc100_pm_prepare;
	pm_cpu_sleep = s5pc100_cpu_suspend;

	return 0;
}

static struct sleep_save s5pc100_sleep[] = {

};

static int s5pc100_pm_suspend(struct sys_device *dev, pm_message_t state)
{
	s5pc1xx_pm_do_save(s5pc100_sleep, ARRAY_SIZE(s5pc100_sleep));
	return 0;
}

static int s5pc100_pm_resume(struct sys_device *dev)
{
	s5pc1xx_pm_do_restore(s5pc100_sleep, ARRAY_SIZE(s5pc100_sleep));
	return 0;
}

static struct sysdev_driver s5pc100_pm_driver = {
	.add		= s5pc100_pm_add,
	.resume		= s5pc100_pm_resume,
};

static __init int s5pc100_pm_drvinit(void)
{
	return sysdev_driver_register(&s5pc100_sysclass, &s5pc100_pm_driver);
}

arch_initcall(s5pc100_pm_drvinit);

