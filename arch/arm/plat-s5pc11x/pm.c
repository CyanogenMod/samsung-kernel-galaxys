/* linux/arch/arm/plat-s5pc11x/pm.c
 *
 * Copyright (c) 2004,2009 Simtec Electronics
 *	boyko.lee <boyko.lee@samsung.com>
 *
 * S5PC11X Power Manager (Suspend-To-RAM) support
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
 *
 * Parts based on arch/arm/mach-pxa/pm.c
 *
 * Thanks to Dimitry Andric for debugging
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>
#include <plat/regs-power.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-audss.h>
#include <plat/s5pc11x-dvfs.h>
#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <asm/gpio.h>
#include <mach/sec_jack.h>
#include <asm/mach/time.h>

#include <plat/pm.h>

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
static struct wake_lock pm_wake_lock;
#endif

#ifdef CONFIG_PM_PWR_GATING
#include <linux/regulator/max8998.h>
static DEFINE_MUTEX(power_lock);
#endif

// MIDAS@SPINLOCK
DEFINE_SPINLOCK(power_gating_lock);

extern void s3c_config_sleep_gpio(void );

/* for external use */
int s5pc11x_pm_eint_registered =0;
unsigned long s5pc1xx_pm_flags;
void __iomem *weint_base;


#define PFX "s5pc11x-pm: "
static struct sleep_save core_save[] = {
/* Clock source */
	SAVE_ITEM(S5P_CLK_SRC0),
	SAVE_ITEM(S5P_CLK_SRC1),
	SAVE_ITEM(S5P_CLK_SRC2),
	SAVE_ITEM(S5P_CLK_SRC3),
	SAVE_ITEM(S5P_CLK_SRC4),
	SAVE_ITEM(S5P_CLK_SRC5),
	SAVE_ITEM(S5P_CLK_SRC6),
/* Clock source Mask */
	SAVE_ITEM(S5P_CLK_SRC_MASK0),
	SAVE_ITEM(S5P_CLK_SRC_MASK1),
/* Clock Divider */
	SAVE_ITEM(S5P_CLK_DIV0),
	SAVE_ITEM(S5P_CLK_DIV1),
	SAVE_ITEM(S5P_CLK_DIV2),
	SAVE_ITEM(S5P_CLK_DIV3),
	SAVE_ITEM(S5P_CLK_DIV4),
	SAVE_ITEM(S5P_CLK_DIV5),
	SAVE_ITEM(S5P_CLK_DIV6),
	SAVE_ITEM(S5P_CLK_DIV7),
/* Clock Main Main Gate */
	SAVE_ITEM(S5P_CLKGATE_MAIN0),
	SAVE_ITEM(S5P_CLKGATE_MAIN1),
	SAVE_ITEM(S5P_CLKGATE_MAIN2),
/* Clock source Peri Gate */
	SAVE_ITEM(S5P_CLKGATE_PERI0),
	SAVE_ITEM(S5P_CLKGATE_PERI1),
/* Clock source SCLK Gate */
	SAVE_ITEM(S5P_CLKGATE_SCLK0),
	SAVE_ITEM(S5P_CLKGATE_SCLK1),
/* Clock IP Clock gate */
	SAVE_ITEM(S5P_CLKGATE_IP0),
	SAVE_ITEM(S5P_CLKGATE_IP1),
	SAVE_ITEM(S5P_CLKGATE_IP2),
	SAVE_ITEM(S5P_CLKGATE_IP3),
	SAVE_ITEM(S5P_CLKGATE_IP4),
/* Clock Block and Bus gate */
	SAVE_ITEM(S5P_CLKGATE_BLOCK),
	SAVE_ITEM(S5P_CLKGATE_BUS0),
	SAVE_ITEM(S5P_CLKGATE_BUS1),	
/* Clock ETC */
	SAVE_ITEM(S5P_CLK_OUT),
	SAVE_ITEM(S5P_MDNIE_SEL),
/*AUDSS*/
//I2S driver does Audio subsystem clock gating 
#if 0
	SAVE_ITEM(S5P_CLKSRC_AUDSS),
	SAVE_ITEM(S5P_CLKDIV_AUDSS),
	SAVE_ITEM(S5P_CLKGATE_AUDSS),
#endif
};

static struct sleep_save gpio_save[] = {
	SAVE_ITEM(S5PC11X_GPA0DAT),
	SAVE_ITEM(S5PC11X_GPA0CON),		
	SAVE_ITEM(S5PC11X_GPA0PUD),		
	SAVE_ITEM(S5PC11X_GPA0DRV),		
	SAVE_ITEM(S5PC11X_GPA0CONPDN),
	SAVE_ITEM(S5PC11X_GPA0PUDPDN),
	SAVE_ITEM(S5PC11X_GPA1DAT),		
	SAVE_ITEM(S5PC11X_GPA1CON),		
	SAVE_ITEM(S5PC11X_GPA1PUD),		
	SAVE_ITEM(S5PC11X_GPA1DRV),		
	SAVE_ITEM(S5PC11X_GPA1CONPDN),
	SAVE_ITEM(S5PC11X_GPA1PUDPDN),
	SAVE_ITEM(S5PC11X_GPBDAT),		
	SAVE_ITEM(S5PC11X_GPBCON),		
	SAVE_ITEM(S5PC11X_GPBPUD),		
	SAVE_ITEM(S5PC11X_GPBDRV),		
	SAVE_ITEM(S5PC11X_GPBCONPDN),	
	SAVE_ITEM(S5PC11X_GPBPUDPDN),	
	SAVE_ITEM(S5PC11X_GPC0DAT),		
	SAVE_ITEM(S5PC11X_GPC0CON),		
	SAVE_ITEM(S5PC11X_GPC0PUD),		
	SAVE_ITEM(S5PC11X_GPC0DRV),		
	SAVE_ITEM(S5PC11X_GPC0CONPDN),
	SAVE_ITEM(S5PC11X_GPC0PUDPDN),
	SAVE_ITEM(S5PC11X_GPC1DAT),		
	SAVE_ITEM(S5PC11X_GPC1CON),		
	SAVE_ITEM(S5PC11X_GPC1PUD),		
	SAVE_ITEM(S5PC11X_GPC1DRV),		
	SAVE_ITEM(S5PC11X_GPC1CONPDN),
	SAVE_ITEM(S5PC11X_GPC1PUDPDN),
	SAVE_ITEM(S5PC11X_GPD0DAT),		
	SAVE_ITEM(S5PC11X_GPD0CON),		
	SAVE_ITEM(S5PC11X_GPD0PUD),		
	SAVE_ITEM(S5PC11X_GPD0DRV),		
	SAVE_ITEM(S5PC11X_GPD0CONPDN),
	SAVE_ITEM(S5PC11X_GPD0PUDPDN),
	SAVE_ITEM(S5PC11X_GPD1DAT),		
	SAVE_ITEM(S5PC11X_GPD1CON),		
	SAVE_ITEM(S5PC11X_GPD1PUD),		
	SAVE_ITEM(S5PC11X_GPD1DRV),		
	SAVE_ITEM(S5PC11X_GPD1CONPDN),
	SAVE_ITEM(S5PC11X_GPD1PUDPDN),
	SAVE_ITEM(S5PC11X_GPE0DAT),		
	SAVE_ITEM(S5PC11X_GPE0CON),		
	SAVE_ITEM(S5PC11X_GPE0PUD),		
	SAVE_ITEM(S5PC11X_GPE0DRV),		
	SAVE_ITEM(S5PC11X_GPE0CONPDN),
	SAVE_ITEM(S5PC11X_GPE0PUDPDN),
	SAVE_ITEM(S5PC11X_GPE1DAT),		
	SAVE_ITEM(S5PC11X_GPE1CON),		
	SAVE_ITEM(S5PC11X_GPE1PUD),		
	SAVE_ITEM(S5PC11X_GPE1DRV),		
	SAVE_ITEM(S5PC11X_GPE1CONPDN),
	SAVE_ITEM(S5PC11X_GPE1PUDPDN),
	SAVE_ITEM(S5PC11X_GPF0DAT),		
	SAVE_ITEM(S5PC11X_GPF0CON),		
	SAVE_ITEM(S5PC11X_GPF0PUD),		
	SAVE_ITEM(S5PC11X_GPF0DRV),		
	SAVE_ITEM(S5PC11X_GPF0CONPDN),
	SAVE_ITEM(S5PC11X_GPF0PUDPDN),
	SAVE_ITEM(S5PC11X_GPF1DAT),		
	SAVE_ITEM(S5PC11X_GPF1CON),		
	SAVE_ITEM(S5PC11X_GPF1PUD),		
	SAVE_ITEM(S5PC11X_GPF1DRV),		
	SAVE_ITEM(S5PC11X_GPF1CONPDN),
	SAVE_ITEM(S5PC11X_GPF1PUDPDN),
	SAVE_ITEM(S5PC11X_GPF2DAT),		
	SAVE_ITEM(S5PC11X_GPF2CON),		
	SAVE_ITEM(S5PC11X_GPF2PUD),		
	SAVE_ITEM(S5PC11X_GPF2DRV),		
	SAVE_ITEM(S5PC11X_GPF2CONPDN),
	SAVE_ITEM(S5PC11X_GPF2PUDPDN),
	SAVE_ITEM(S5PC11X_GPF3DAT),		
	SAVE_ITEM(S5PC11X_GPF3CON),		
	SAVE_ITEM(S5PC11X_GPF3PUD),		
	SAVE_ITEM(S5PC11X_GPF3DRV),		
	SAVE_ITEM(S5PC11X_GPF3CONPDN),
	SAVE_ITEM(S5PC11X_GPF3PUDPDN),
	SAVE_ITEM(S5PC11X_GPG0DAT),		
	SAVE_ITEM(S5PC11X_GPG0CON),		
	SAVE_ITEM(S5PC11X_GPG0PUD),		
	SAVE_ITEM(S5PC11X_GPG0DRV),		
	SAVE_ITEM(S5PC11X_GPG0CONPDN),
	SAVE_ITEM(S5PC11X_GPG0PUDPDN),
	SAVE_ITEM(S5PC11X_GPG1DAT),		
	SAVE_ITEM(S5PC11X_GPG1CON),		
	SAVE_ITEM(S5PC11X_GPG1PUD),		
	SAVE_ITEM(S5PC11X_GPG1DRV),		
	SAVE_ITEM(S5PC11X_GPG1CONPDN),
	SAVE_ITEM(S5PC11X_GPG1PUDPDN),
	SAVE_ITEM(S5PC11X_GPG2DAT),		
	SAVE_ITEM(S5PC11X_GPG2CON),		
	SAVE_ITEM(S5PC11X_GPG2PUD),		
	SAVE_ITEM(S5PC11X_GPG2DRV),		
	SAVE_ITEM(S5PC11X_GPG2CONPDN),
	SAVE_ITEM(S5PC11X_GPG2PUDPDN),
	SAVE_ITEM(S5PC11X_GPG3DAT),		
	SAVE_ITEM(S5PC11X_GPG3CON),		
	SAVE_ITEM(S5PC11X_GPG3PUD),		
	SAVE_ITEM(S5PC11X_GPG3DRV),		
	SAVE_ITEM(S5PC11X_GPG3CONPDN),
	SAVE_ITEM(S5PC11X_GPG3PUDPDN),
#ifndef  S5PC11X_ALIVEGPIO_STORE // Moving GPH0,1,2,3 to alive group for EVT0 fix
	SAVE_ITEM(S5PC11X_GPH0DAT),		
	SAVE_ITEM(S5PC11X_GPH0CON),		
	SAVE_ITEM(S5PC11X_GPH0PUD),		
	SAVE_ITEM(S5PC11X_GPH0DRV),		
	SAVE_ITEM(S5PC11X_GPH0CONPDN),
	SAVE_ITEM(S5PC11X_GPH0PUDPDN),
	SAVE_ITEM(S5PC11X_GPH1DAT),		
	SAVE_ITEM(S5PC11X_GPH1CON),		
	SAVE_ITEM(S5PC11X_GPH1PUD),		
	SAVE_ITEM(S5PC11X_GPH1DRV),		
	SAVE_ITEM(S5PC11X_GPH1CONPDN),
	SAVE_ITEM(S5PC11X_GPH1PUDPDN),
	SAVE_ITEM(S5PC11X_GPH2DAT),		
	SAVE_ITEM(S5PC11X_GPH2CON),		
	SAVE_ITEM(S5PC11X_GPH2PUD),		
	SAVE_ITEM(S5PC11X_GPH2DRV),		
	SAVE_ITEM(S5PC11X_GPH2CONPDN),
	SAVE_ITEM(S5PC11X_GPH2PUDPDN),
	SAVE_ITEM(S5PC11X_GPH3DAT),		
	SAVE_ITEM(S5PC11X_GPH3CON),		
	SAVE_ITEM(S5PC11X_GPH3PUD),		
	SAVE_ITEM(S5PC11X_GPH3DRV),		
	SAVE_ITEM(S5PC11X_GPH3CONPDN),
	SAVE_ITEM(S5PC11X_GPH3PUDPDN),
#endif
	SAVE_ITEM(S5PC11X_GPIDAT),		
	SAVE_ITEM(S5PC11X_GPICON),		
	SAVE_ITEM(S5PC11X_GPIPUD),		
	SAVE_ITEM(S5PC11X_GPIDRV),		
	SAVE_ITEM(S5PC11X_GPICONPDN),
	SAVE_ITEM(S5PC11X_GPIPUDPDN),
	SAVE_ITEM(S5PC11X_GPJ0DAT),		
	SAVE_ITEM(S5PC11X_GPJ0CON),		
	SAVE_ITEM(S5PC11X_GPJ0PUD),		
	SAVE_ITEM(S5PC11X_GPJ0DRV),		
	SAVE_ITEM(S5PC11X_GPJ0CONPDN),
	SAVE_ITEM(S5PC11X_GPJ0PUDPDN),
	SAVE_ITEM(S5PC11X_GPJ1DAT),		
	SAVE_ITEM(S5PC11X_GPJ1CON),		
	SAVE_ITEM(S5PC11X_GPJ1PUD),		
	SAVE_ITEM(S5PC11X_GPJ1DRV),		
	SAVE_ITEM(S5PC11X_GPJ1CONPDN),
	SAVE_ITEM(S5PC11X_GPJ1PUDPDN),
	SAVE_ITEM(S5PC11X_GPJ2DAT),		
	SAVE_ITEM(S5PC11X_GPJ2CON),		
	SAVE_ITEM(S5PC11X_GPJ2PUD),		
	SAVE_ITEM(S5PC11X_GPJ2DRV),		
	SAVE_ITEM(S5PC11X_GPJ2CONPDN),
	SAVE_ITEM(S5PC11X_GPJ2PUDPDN),
	SAVE_ITEM(S5PC11X_GPJ3DAT),		
	SAVE_ITEM(S5PC11X_GPJ3CON),		
	SAVE_ITEM(S5PC11X_GPJ3PUD),		
	SAVE_ITEM(S5PC11X_GPJ3DRV),		
	SAVE_ITEM(S5PC11X_GPJ3CONPDN),
	SAVE_ITEM(S5PC11X_GPJ3PUDPDN),
	SAVE_ITEM(S5PC11X_GPJ4DAT),		
	SAVE_ITEM(S5PC11X_GPJ4CON),		
	SAVE_ITEM(S5PC11X_GPJ4PUD),		
	SAVE_ITEM(S5PC11X_GPJ4DRV),		
	SAVE_ITEM(S5PC11X_GPJ4CONPDN),
	SAVE_ITEM(S5PC11X_GPJ4PUDPDN),
	SAVE_ITEM(S5PC11X_MP01DAT),		
	SAVE_ITEM(S5PC11X_MP01CON),		
	SAVE_ITEM(S5PC11X_MP01PUD),		
	SAVE_ITEM(S5PC11X_MP01DRV),		
	SAVE_ITEM(S5PC11X_MP01CONPDN),
	SAVE_ITEM(S5PC11X_MP01PUDPDN),
	SAVE_ITEM(S5PC11X_MP02DAT),		
	SAVE_ITEM(S5PC11X_MP02CON),		
	SAVE_ITEM(S5PC11X_MP02PUD),		
	SAVE_ITEM(S5PC11X_MP02DRV),		
	SAVE_ITEM(S5PC11X_MP02CONPDN),
	SAVE_ITEM(S5PC11X_MP02PUDPDN),
	SAVE_ITEM(S5PC11X_MP03DAT),		
	SAVE_ITEM(S5PC11X_MP03CON),		
	SAVE_ITEM(S5PC11X_MP03PUD),		
	SAVE_ITEM(S5PC11X_MP03DRV),		
	SAVE_ITEM(S5PC11X_MP03CONPDN),
	SAVE_ITEM(S5PC11X_MP03PUDPDN),
	SAVE_ITEM(S5PC11X_MP04DAT),		
	SAVE_ITEM(S5PC11X_MP04CON),		
	SAVE_ITEM(S5PC11X_MP04PUD),		
	SAVE_ITEM(S5PC11X_MP04DRV),		
	SAVE_ITEM(S5PC11X_MP04CONPDN),
	SAVE_ITEM(S5PC11X_MP04PUDPDN),
	SAVE_ITEM(S5PC11X_MP05DAT),		
	SAVE_ITEM(S5PC11X_MP05CON),		
	SAVE_ITEM(S5PC11X_MP05PUD),		
	SAVE_ITEM(S5PC11X_MP05DRV),		
	SAVE_ITEM(S5PC11X_MP05CONPDN),
	SAVE_ITEM(S5PC11X_MP05PUDPDN),
	SAVE_ITEM(S5PC11X_MP06DAT),		
	SAVE_ITEM(S5PC11X_MP06CON),		
	SAVE_ITEM(S5PC11X_MP06PUD),		
	SAVE_ITEM(S5PC11X_MP06DRV),		
	SAVE_ITEM(S5PC11X_MP06CONPDN),
	SAVE_ITEM(S5PC11X_MP06PUDPDN),
	SAVE_ITEM(S5PC11X_MP07DAT),		
	SAVE_ITEM(S5PC11X_MP07CON),		
	SAVE_ITEM(S5PC11X_MP07PUD),		
	SAVE_ITEM(S5PC11X_MP07DRV),		
	SAVE_ITEM(S5PC11X_MP07CONPDN),
	SAVE_ITEM(S5PC11X_MP07PUDPDN),
	SAVE_ITEM(S5PC11X_MP10DAT),		
	SAVE_ITEM(S5PC11X_MP10CON),		
	SAVE_ITEM(S5PC11X_MP10PUD),		
	SAVE_ITEM(S5PC11X_MP10DRV),		
	SAVE_ITEM(S5PC11X_MP10CONPDN),
	SAVE_ITEM(S5PC11X_MP10PUDPDN),
	SAVE_ITEM(S5PC11X_MP11DAT),		
	SAVE_ITEM(S5PC11X_MP11CON),		
	SAVE_ITEM(S5PC11X_MP11PUD),		
	SAVE_ITEM(S5PC11X_MP11DRV),		
	SAVE_ITEM(S5PC11X_MP11CONPDN),
	SAVE_ITEM(S5PC11X_MP11PUDPDN),
	SAVE_ITEM(S5PC11X_MP12DAT),	
	SAVE_ITEM(S5PC11X_MP12CON),	
	SAVE_ITEM(S5PC11X_MP12PUD),	
	SAVE_ITEM(S5PC11X_MP12DRV),	
	SAVE_ITEM(S5PC11X_MP12CONPDN),
	SAVE_ITEM(S5PC11X_MP12PUDPDN),
	SAVE_ITEM(S5PC11X_MP13DAT),
	SAVE_ITEM(S5PC11X_MP13CON),
	SAVE_ITEM(S5PC11X_MP13PUD),
	SAVE_ITEM(S5PC11X_MP13DRV),
	SAVE_ITEM(S5PC11X_MP13CONPDN),
	SAVE_ITEM(S5PC11X_MP13PUDPDN),
	SAVE_ITEM(S5PC11X_MP14DAT),
	SAVE_ITEM(S5PC11X_MP14CON),
	SAVE_ITEM(S5PC11X_MP14PUD),
	SAVE_ITEM(S5PC11X_MP14DRV),
	SAVE_ITEM(S5PC11X_MP14CONPDN),
	SAVE_ITEM(S5PC11X_MP14PUDPDN),
	SAVE_ITEM(S5PC11X_MP15DAT),
	SAVE_ITEM(S5PC11X_MP15CON),
	SAVE_ITEM(S5PC11X_MP15PUD),
	SAVE_ITEM(S5PC11X_MP15DRV),
	SAVE_ITEM(S5PC11X_MP15CONPDN),
	SAVE_ITEM(S5PC11X_MP15PUDPDN),
	SAVE_ITEM(S5PC11X_MP16DAT),
	SAVE_ITEM(S5PC11X_MP16CON),
	SAVE_ITEM(S5PC11X_MP16PUD),
	SAVE_ITEM(S5PC11X_MP16DRV),
	SAVE_ITEM(S5PC11X_MP16CONPDN),
	SAVE_ITEM(S5PC11X_MP16PUDPDN),
	SAVE_ITEM(S5PC11X_MP17DAT),
	SAVE_ITEM(S5PC11X_MP17CON),
	SAVE_ITEM(S5PC11X_MP17PUD),
	SAVE_ITEM(S5PC11X_MP17DRV),
	SAVE_ITEM(S5PC11X_MP17CONPDN),
	SAVE_ITEM(S5PC11X_MP17PUDPDN),
	SAVE_ITEM(S5PC11X_MP18DAT),
	SAVE_ITEM(S5PC11X_MP18CON),
	SAVE_ITEM(S5PC11X_MP18PUD),
	SAVE_ITEM(S5PC11X_MP18DRV),
	SAVE_ITEM(S5PC11X_MP18CONPDN),
	SAVE_ITEM(S5PC11X_MP18PUDPDN),
	SAVE_ITEM(S5PC11X_MP20DAT),
	SAVE_ITEM(S5PC11X_MP20CON),
	SAVE_ITEM(S5PC11X_MP20PUD),
	SAVE_ITEM(S5PC11X_MP20DRV),
	SAVE_ITEM(S5PC11X_MP20CONPDN),
	SAVE_ITEM(S5PC11X_MP20PUDPDN),
	SAVE_ITEM(S5PC11X_MP21DAT),
	SAVE_ITEM(S5PC11X_MP21CON),
	SAVE_ITEM(S5PC11X_MP21PUD),
	SAVE_ITEM(S5PC11X_MP21DRV),
	SAVE_ITEM(S5PC11X_MP21CONPDN),
	SAVE_ITEM(S5PC11X_MP21PUDPDN),
	SAVE_ITEM(S5PC11X_MP22DAT),
	SAVE_ITEM(S5PC11X_MP22CON),
	SAVE_ITEM(S5PC11X_MP22PUD),
	SAVE_ITEM(S5PC11X_MP22DRV),
	SAVE_ITEM(S5PC11X_MP22CONPDN),
	SAVE_ITEM(S5PC11X_MP22PUDPDN),
	SAVE_ITEM(S5PC11X_MP23DAT),
	SAVE_ITEM(S5PC11X_MP23CON),
	SAVE_ITEM(S5PC11X_MP23PUD),
	SAVE_ITEM(S5PC11X_MP23DRV),
	SAVE_ITEM(S5PC11X_MP23CONPDN),
	SAVE_ITEM(S5PC11X_MP23PUDPDN),
	SAVE_ITEM(S5PC11X_MP24DAT),
	SAVE_ITEM(S5PC11X_MP24CON),
	SAVE_ITEM(S5PC11X_MP24PUD),
	SAVE_ITEM(S5PC11X_MP24DRV),
	SAVE_ITEM(S5PC11X_MP24CONPDN),
	SAVE_ITEM(S5PC11X_MP24PUDPDN),
	SAVE_ITEM(S5PC11X_MP25DAT),		
	SAVE_ITEM(S5PC11X_MP25CON),		
	SAVE_ITEM(S5PC11X_MP25PUD),		
	SAVE_ITEM(S5PC11X_MP25DRV),		
	SAVE_ITEM(S5PC11X_MP25CONPDN),
	SAVE_ITEM(S5PC11X_MP25PUDPDN),
	SAVE_ITEM(S5PC11X_MP26DAT),		
	SAVE_ITEM(S5PC11X_MP26CON),		
	SAVE_ITEM(S5PC11X_MP26PUD),		
	SAVE_ITEM(S5PC11X_MP26DRV),		
	SAVE_ITEM(S5PC11X_MP26CONPDN),
	SAVE_ITEM(S5PC11X_MP26PUDPDN),
	SAVE_ITEM(S5PC11X_MP27DAT),		
	SAVE_ITEM(S5PC11X_MP27CON),		
	SAVE_ITEM(S5PC11X_MP27PUD),		
	SAVE_ITEM(S5PC11X_MP27DRV),		
	SAVE_ITEM(S5PC11X_MP27CONPDN),
	SAVE_ITEM(S5PC11X_MP27PUDPDN),
	SAVE_ITEM(S5PC11X_MP28DAT),		
	SAVE_ITEM(S5PC11X_MP28CON),		
	SAVE_ITEM(S5PC11X_MP28PUD),		
	SAVE_ITEM(S5PC11X_MP28DRV),		
	SAVE_ITEM(S5PC11X_MP28CONPDN),
	SAVE_ITEM(S5PC11X_MP28PUDPDN),
#ifndef S5PC11X_ALIVEGPIO_STORE   // Moving to alive block for EVT0 workaround
	// external interrupt stuff
	
	SAVE_ITEM(S5PC11X_EINT0CON),
	SAVE_ITEM(S5PC11X_EINT1CON),
	SAVE_ITEM(S5PC11X_EINT2CON),
	SAVE_ITEM(S5PC11X_EINT3CON),

	SAVE_ITEM(S5PC11X_EINT0MASK),
	SAVE_ITEM(S5PC11X_EINT1MASK),
	SAVE_ITEM(S5PC11X_EINT2MASK),
	SAVE_ITEM(S5PC11X_EINT3MASK),

	SAVE_ITEM(S5PC11X_EINT0FLTCON0),
	SAVE_ITEM(S5PC11X_EINT0FLTCON1),
	SAVE_ITEM(S5PC11X_EINT1FLTCON0),
	SAVE_ITEM(S5PC11X_EINT1FLTCON1),
	SAVE_ITEM(S5PC11X_EINT2FLTCON0),
	SAVE_ITEM(S5PC11X_EINT2FLTCON1),
	SAVE_ITEM(S5PC11X_EINT3FLTCON0),
	SAVE_ITEM(S5PC11X_EINT3FLTCON1),
#endif
	// gpio interrupt stuff
	
	SAVE_ITEM(S5PC11X_GPA0INT_CON),
	SAVE_ITEM(S5PC11X_GPA1INT_CON),
	SAVE_ITEM(S5PC11X_GPBINT_CON),
	SAVE_ITEM(S5PC11X_GPC0INT_CON),
	SAVE_ITEM(S5PC11X_GPC1INT_CON),
	SAVE_ITEM(S5PC11X_GPD0INT_CON),
	SAVE_ITEM(S5PC11X_GPD1INT_CON),
	SAVE_ITEM(S5PC11X_GPE0INT_CON),
	SAVE_ITEM(S5PC11X_GPE1INT_CON),
	SAVE_ITEM(S5PC11X_GPF0INT_CON),
	SAVE_ITEM(S5PC11X_GPF1INT_CON),
	SAVE_ITEM(S5PC11X_GPF2INT_CON),
	SAVE_ITEM(S5PC11X_GPF3INT_CON),
	SAVE_ITEM(S5PC11X_GPG0INT_CON),
	SAVE_ITEM(S5PC11X_GPG1INT_CON),
	SAVE_ITEM(S5PC11X_GPG2INT_CON),
	SAVE_ITEM(S5PC11X_GPG3INT_CON),
	SAVE_ITEM(S5PC11X_GPJ0INT_CON),
	SAVE_ITEM(S5PC11X_GPJ1INT_CON),
	SAVE_ITEM(S5PC11X_GPJ2INT_CON),
	SAVE_ITEM(S5PC11X_GPJ3INT_CON),
	SAVE_ITEM(S5PC11X_GPJ4INT_CON),


	SAVE_ITEM(S5PC11X_GPA0INT_MASK),
	SAVE_ITEM(S5PC11X_GPA1INT_MASK),
	SAVE_ITEM(S5PC11X_GPBINT_MASK),
	SAVE_ITEM(S5PC11X_GPC0INT_MASK),
	SAVE_ITEM(S5PC11X_GPC1INT_MASK),
	SAVE_ITEM(S5PC11X_GPD0INT_MASK),
	SAVE_ITEM(S5PC11X_GPD1INT_MASK),
	SAVE_ITEM(S5PC11X_GPE0INT_MASK),
	SAVE_ITEM(S5PC11X_GPE1INT_MASK),
	SAVE_ITEM(S5PC11X_GPF0INT_MASK),
	SAVE_ITEM(S5PC11X_GPF1INT_MASK),
	SAVE_ITEM(S5PC11X_GPF2INT_MASK),
	SAVE_ITEM(S5PC11X_GPF3INT_MASK),
	SAVE_ITEM(S5PC11X_GPG0INT_MASK),
	SAVE_ITEM(S5PC11X_GPG1INT_MASK),
	SAVE_ITEM(S5PC11X_GPG2INT_MASK),
	SAVE_ITEM(S5PC11X_GPG3INT_MASK),
	SAVE_ITEM(S5PC11X_GPJ0INT_MASK),
	SAVE_ITEM(S5PC11X_GPJ1INT_MASK),
	SAVE_ITEM(S5PC11X_GPJ2INT_MASK),
	SAVE_ITEM(S5PC11X_GPJ3INT_MASK),
	SAVE_ITEM(S5PC11X_GPJ4INT_MASK),

	SAVE_ITEM(S5PC11X_GPA0INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPA0INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPA1INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPA1INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPBINT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPBINT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPC0INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPC0INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPC1INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPC1INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPD0INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPD0INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPD1INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPD1INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPE0INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPE0INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPE1INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPE1INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPF0INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPF0INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPF1INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPF1INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPF2INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPF2INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPF3INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPF3INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPG0INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPG0INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPG1INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPG1INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPG2INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPG2INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPG3INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPG3INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPJ0INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPJ0INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPJ1INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPJ1INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPJ2INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPJ2INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPJ3INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPJ3INT_FLTCON1),
	SAVE_ITEM(S5PC11X_GPJ4INT_FLTCON0),
	SAVE_ITEM(S5PC11X_GPJ4INT_FLTCON1),

#if 0	
	SAVE_ITEM(S5PC11X_GPH0CON),		
	SAVE_ITEM(S5PC11X_GPH1CON),		
	SAVE_ITEM(S5PC11X_GPH2CON),		
	SAVE_ITEM(S5PC11X_GPH3CON),		
#endif

};

#ifdef S5PC11X_ALIVEGPIO_STORE
static struct sleep_save gpio_save_alive[] = {
	SAVE_ITEM(S5PC11X_GPH0DAT),		
	SAVE_ITEM(S5PC11X_GPH0CON),		
	SAVE_ITEM(S5PC11X_GPH0PUD),		
	SAVE_ITEM(S5PC11X_GPH0DRV),		
	SAVE_ITEM(S5PC11X_GPH0CONPDN),
	SAVE_ITEM(S5PC11X_GPH0PUDPDN),
	SAVE_ITEM(S5PC11X_GPH1DAT),		
	SAVE_ITEM(S5PC11X_GPH1CON),		
	SAVE_ITEM(S5PC11X_GPH1PUD),		
	SAVE_ITEM(S5PC11X_GPH1DRV),		
	SAVE_ITEM(S5PC11X_GPH1CONPDN),
	SAVE_ITEM(S5PC11X_GPH1PUDPDN),
	SAVE_ITEM(S5PC11X_GPH2DAT),		
	SAVE_ITEM(S5PC11X_GPH2CON),		
	SAVE_ITEM(S5PC11X_GPH2PUD),		
	SAVE_ITEM(S5PC11X_GPH2DRV),		
	SAVE_ITEM(S5PC11X_GPH2CONPDN),
	SAVE_ITEM(S5PC11X_GPH2PUDPDN),
	SAVE_ITEM(S5PC11X_GPH3DAT),		
	SAVE_ITEM(S5PC11X_GPH3CON),		
	SAVE_ITEM(S5PC11X_GPH3PUD),		
	SAVE_ITEM(S5PC11X_GPH3DRV),		
	SAVE_ITEM(S5PC11X_GPH3CONPDN),
	SAVE_ITEM(S5PC11X_GPH3PUDPDN),

	SAVE_ITEM(S5PC11X_EINT0CON),
	SAVE_ITEM(S5PC11X_EINT1CON),
	SAVE_ITEM(S5PC11X_EINT2CON),
	SAVE_ITEM(S5PC11X_EINT3CON),

	SAVE_ITEM(S5PC11X_EINT0MASK),
	SAVE_ITEM(S5PC11X_EINT1MASK),
	SAVE_ITEM(S5PC11X_EINT2MASK),
	SAVE_ITEM(S5PC11X_EINT3MASK),

	SAVE_ITEM(S5PC11X_EINT0FLTCON0),
	SAVE_ITEM(S5PC11X_EINT0FLTCON1),
	SAVE_ITEM(S5PC11X_EINT1FLTCON0),
	SAVE_ITEM(S5PC11X_EINT1FLTCON1),
	SAVE_ITEM(S5PC11X_EINT2FLTCON0),
	SAVE_ITEM(S5PC11X_EINT2FLTCON1),
	SAVE_ITEM(S5PC11X_EINT3FLTCON0),
	SAVE_ITEM(S5PC11X_EINT3FLTCON1),
};
#endif //S5PC11X_ALIVEGPIO_STORE

/* this lot should be really saved by the IRQ code */
/* VICXADDRESSXX initilaization to be needed */
static struct sleep_save irq_save[] = {
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC3REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC3REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC110_VIC3REG(VIC_INT_SOFT)),
};

static struct sleep_save sromc_save[] = {
	SAVE_ITEM(S5PC11X_SROM_BW),
	SAVE_ITEM(S5PC11X_SROM_BC0),
	SAVE_ITEM(S5PC11X_SROM_BC1),
	SAVE_ITEM(S5PC11X_SROM_BC2),
	SAVE_ITEM(S5PC11X_SROM_BC3),
	SAVE_ITEM(S5PC11X_SROM_BC4),
	SAVE_ITEM(S5PC11X_SROM_BC5),
};

#define S5P_UINTP   0x30
#define S5P_UINTSP  0x34
#define S5P_UINTM   0x38

#define SAVE_UART(va) \
	SAVE_ITEM((va) + S3C2410_ULCON), \
	SAVE_ITEM((va) + S3C2410_UCON), \
	SAVE_ITEM((va) + S3C2410_UFCON), \
	SAVE_ITEM((va) + S3C2410_UMCON), \
	SAVE_ITEM((va) + S3C2410_UBRDIV), \
	SAVE_ITEM((va) + S3C2410_UDIVSLOT), \
	SAVE_ITEM((va) + S3C2410_UINTMSK)


static struct sleep_save uart_save[] = {
	SAVE_UART(S3C24XX_VA_UART2),
};


#define DBG(fmt...)
//#define DBG(fmt...) printk(fmt)

#define s5pc11x_pm_debug_init() do { } while(0)
#define s5pc11x_pm_check_prepare() do { } while(0)
#define s5pc11x_pm_check_restore() do { } while(0)
#define s5pc11x_pm_check_store()   do { } while(0)


#ifdef CONFIG_PM_PWR_GATING

unsigned int g_power_domain_lock_token = 0;
//unsigned int * g_power_domain_reg = 0; //global variable to pass register address
//unsigned int g_power_domain_val = 0; //global variable to pass register value
extern int dvs_initilized;

void s5pc110_lock_power_domain(unsigned int nToken)
{
	if(nToken > I2S0_DOMAIN_LOCK_TOKEN)
	{
		printk(KERN_ERR "Lock power called with invalid parameter\n");
		return;	
	}
	mutex_lock(&power_lock);
	g_power_domain_lock_token |= nToken;
	mutex_unlock(&power_lock);
	DBG("Lock power called with token %d\n", nToken);	
}

void s5pc110_unlock_power_domain(unsigned int nToken)
{
	if(nToken > I2S0_DOMAIN_LOCK_TOKEN)
	{
		printk(KERN_ERR "Unlock power called with invalid parameter\n");
		return;	
	}
	mutex_lock(&power_lock);
	g_power_domain_lock_token &= ~nToken;
	mutex_unlock(&power_lock);
	DBG("Unlock power called with token %d\n", nToken);

	/*check if we can power off this domain*/
	if(nToken == MFC_DOMAIN_LOCK_TOKEN)
		s5p_power_gating(S5PC110_POWER_DOMAIN_MFC, DOMAIN_LP_MODE);
	else if(nToken == G3D_DOMAIN_LOCK_TOKEN)	
		s5p_power_gating(S5PC110_POWER_DOMAIN_G3D, DOMAIN_LP_MODE);	
	else if((nToken >= FIMD_DOMAIN_LOCK_TOKEN) && (nToken <= DSIM_DOMAIN_LOCK_TOKEN))	
		s5p_power_gating(S5PC110_POWER_DOMAIN_LCD, DOMAIN_LP_MODE);	
	else if((nToken >= VP_DOMAIN_LOCK_TOKEN) && (nToken <= HDMI_DOMAIN_LOCK_TOKEN))	
		s5p_power_gating(S5PC110_POWER_DOMAIN_TV, DOMAIN_LP_MODE);	
	else if((nToken >= FIMC0_DOMAIN_LOCK_TOKEN) && (nToken <= CSIS_DOMAIN_LOCK_TOKEN))	
		s5p_power_gating(S5PC110_POWER_DOMAIN_CAM, DOMAIN_LP_MODE);	
	else if(nToken == I2S0_DOMAIN_LOCK_TOKEN)	
		s5p_power_gating(S5PC110_POWER_DOMAIN_AUDIO, DOMAIN_LP_MODE);
}

/*
 * s5p_domain_off_check()
 *
 * check if the power domain is off or not
 * 
*/
int s5p_domain_off_check(unsigned int power_domain)
{
	unsigned int poweroff = 0;

    switch( power_domain )
    {
	case S5PC110_POWER_DOMAIN_MFC: //MFC off
		if (!(readl(S5P_CLKGATE_IP0) & POWER_DOMAIN_MFC_CLOCK_SET))
	        {	
			if(!(g_power_domain_lock_token & MFC_DOMAIN_LOCK_TOKEN))
				poweroff = 1;
		}
		break;

	case S5PC110_POWER_DOMAIN_G3D: //G3D off
		if (!(readl(S5P_CLKGATE_IP0) & POWER_DOMAIN_G3D_CLOCK_SET))
	        {
			if(!(g_power_domain_lock_token & G3D_DOMAIN_LOCK_TOKEN))
#if 1 // for g3d power gating test on g3d driver			
				poweroff = 1;
#else
				poweroff = 0;
#endif
		}
		break;

	case S5PC110_POWER_DOMAIN_LCD: //LCD off
		if (!(readl(S5P_CLKGATE_IP1) & POWER_DOMAIN_LCD_CLOCK_SET))
	        {
			if(!(g_power_domain_lock_token & (LCD_DOMAIN_LOCK_TOKEN_SET)))
				poweroff = 1;
		}
		break;

	case S5PC110_POWER_DOMAIN_TV: //TV off
		if (!(readl(S5P_CLKGATE_IP1) & POWER_DOMAIN_TV_CLOCK_SET))
	        {
			if(!(g_power_domain_lock_token & (TV_DOMAIN_LOCK_TOKEN_SET)))
				poweroff = 1;
		}
		break;


	case S5PC110_POWER_DOMAIN_CAM: //CAM off
		if (!(readl(S5P_CLKGATE_IP0) & POWER_DOMAIN_CAMERA_CLOCK_SET))
	        {
			if(!(g_power_domain_lock_token & (CAMERA_DOMAIN_LOCK_TOKEN_SET)))
				poweroff = 1;
		}
		break;

	case S5PC110_POWER_DOMAIN_AUDIO: //AUDIO off
#if 1 // for audio pwr gating is not support on chip -> enable for evt1 only
		if(!(readl(S5P_CLKGATE_IP3) &((S5P_CLKGATE_IP3_I2S0))) )
	        {
			if(!(g_power_domain_lock_token & I2S0_DOMAIN_LOCK_TOKEN))
		
				poweroff = 1;
		}
#endif	
		break;
	
	default :
		printk( "[SYSCON][Err] S5PC110_Power_Gating - power_domain: %d \n", power_domain );
		break;
    	}

	return poweroff;
}


/*
 * s5p_pmic_gating()
 *
 * To do turn on/off LDOs of pmic for power gating 
 * 
*/
int s5p_pmic_gating(unsigned int power_domain, unsigned int on_off)
{
    switch( power_domain )
    {
	case S5PC110_POWER_DOMAIN_MFC: //MFC off
		if (on_off) {
			// power on
		} else {
			// power off		
		}
		break;

	case S5PC110_POWER_DOMAIN_G3D: //G3D off
		if (on_off) {
			// power on
		} else {
			// power off		
		}
		break;

	case S5PC110_POWER_DOMAIN_LCD: //LCD: ldo7, 17
		if (on_off) {
			// power on
			max8998_ldo_enable_direct(MAX8998_LDO7);
			max8998_ldo_enable_direct(MAX8998_LDO17);
		} else {
			// power off
			max8998_ldo_disable_direct(MAX8998_LDO7);	
			max8998_ldo_disable_direct(MAX8998_LDO17);
		}
		break;

	case S5PC110_POWER_DOMAIN_TV: //TV off
		if (on_off) {
			// power on
			//max8998_ldo_enable_direct(MAX8998_LDO8);
		} else {
			// power off		
			//max8998_ldo_disable_direct(MAX8998_LDO8);
		}
		break;

	case S5PC110_POWER_DOMAIN_CAM: //CAM: ldo 11,12,13,14,15, 16
		if (on_off) {
			// power on
			/*max8998_ldo_enable_direct(MAX8998_LDO11);
			max8998_ldo_enable_direct(MAX8998_LDO12);
			max8998_ldo_enable_direct(MAX8998_LDO13);
			max8998_ldo_enable_direct(MAX8998_LDO14);
			max8998_ldo_enable_direct(MAX8998_LDO15);
			max8998_ldo_enable_direct(MAX8998_LDO16);*/																		
		} else {
			// power off
			/*max8998_ldo_disable_direct(MAX8998_LDO7);
			max8998_ldo_disable_direct(MAX8998_LDO12);
			max8998_ldo_disable_direct(MAX8998_LDO13);
			max8998_ldo_disable_direct(MAX8998_LDO14);
			max8998_ldo_disable_direct(MAX8998_LDO15);
			max8998_ldo_disable_direct(MAX8998_LDO16);*/																	
		}
		break;

	case S5PC110_POWER_DOMAIN_AUDIO: //AUDIO off
#if 0 // for audio pwr gating is not supported on chip
		if (on_off) {
			// power on
		} else {
			// power off		
		}
#endif
		break;
		
	default :
		printk( "[SYSCON][Err] S5PC110_PMIC_Gating - power_domain: %d durning %s\n", power_domain, on_off ? "Turn on" : "Turn off" );
		break;
    	}
    	
    	return 1;
}


/*
 * s5p_power_gating()
 *
 * To do power gating
 * 
*/
extern int tvblk_turnon;
int s5p_power_gating(unsigned int power_domain, unsigned int on_off)
{
	unsigned int tmp, val;
	int retvalue = 0;
	u32 con;

	if (power_domain > S5PC110_POWER_DOMAIN_UNCANGIBLE_MASK) return;

	spin_lock_irq(&power_gating_lock);
	
	//mutex_lock(&power_lock);
	if(on_off == DOMAIN_ACTIVE_MODE) {
	
		if(s5p_domain_off_check(power_domain)){
			tmp = readl(S5P_NORMAL_CFG);
			if(!(tmp & power_domain)) // enable only once
			{
			
				if (power_domain == S5PC110_POWER_DOMAIN_TV) {
					con = __raw_readl(S5P_CLKGATE_IP1);
					con |= 0x0f00; // enable VP, Mixer, TVEnc, HDMI
					__raw_writel(con, S5P_CLKGATE_IP1);
					tvblk_turnon=1;
									
					//con = readl(S5P_CLKGATE_BLOCK);
					//con |= S5P_CLKGATE_BLOCK_TV;
					//writel(con, S5P_CLKGATE_BLOCK);
				}	
						
				/*Check if we have to enable the ldo's*/
				if(dvs_initilized)
					s5p_pmic_gating(power_domain, 1);
				
				tmp = tmp | (power_domain);
			writel(tmp , S5P_NORMAL_CFG);
			while(!(readl(S5P_BLK_PWR_STAT) & (power_domain)));
				DBG("Requested domain-active mode:  %x \n",power_domain);
				
				
				if (tvblk_turnon) {
					con = __raw_readl(S5P_CLKGATE_IP1);
					con =  (con & ~(0x0f00)); // disable others in VP, Mixer, TVEnc, HDMI
					__raw_writel(con, S5P_CLKGATE_IP1);		
					tvblk_turnon = 0;
				}				
								
			}		
			retvalue = 1;			
		}		
	}
	else if(on_off == DOMAIN_LP_MODE) {
		
		if(s5p_domain_off_check(power_domain)){

			 tmp = readl(S5P_NORMAL_CFG);
			if((tmp & power_domain)) // disable only once
			{
				tmp = tmp & ~(power_domain);
			writel(tmp , S5P_NORMAL_CFG);
    			while((readl(S5P_BLK_PWR_STAT) & (power_domain)));
				DBG("Requested domain-LP mode:  %x \n",power_domain);
				/*Check if we have to disable the ldo's*/
				if(dvs_initilized)
					s5p_pmic_gating(power_domain, 0);
			}
			retvalue = 1;
		}
	}	
	//mutex_unlock(&power_lock);
	
	spin_unlock_irq(&power_gating_lock);
	
	return retvalue;
}
EXPORT_SYMBOL(s5p_power_gating);



/*
 * s5p_init_domain_power()
 *
 * Initailize power domain at booting 
 * 
*/
static void s5p_init_domain_power(void)
{
#if 0
	s5p_power_gating(S5PC110_POWER_DOMAIN_TV,  DOMAIN_LP_MODE);
	s5p_power_gating(S5PC110_POWER_DOMAIN_MFC, DOMAIN_LP_MODE);
	s5p_power_gating(S5PC110_POWER_DOMAIN_AUDIO, DOMAIN_LP_MODE);
	s5p_power_gating(S5PC110_POWER_DOMAIN_G3D, DOMAIN_LP_MODE);
#endif
}

#endif //CONFIG_PM_PWR_GATING


/* helper functions to save and restore register state */

void s5pc11x_pm_do_save(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		ptr->val = __raw_readl(ptr->reg);
		//DBG("saved %p value %08lx\n", ptr->reg, ptr->val);
	}
}

/* s5pc11x_pm_do_restore
 *
 * restore the system from the given list of saved registers
 *
 * Note, we do not use DBG() in here, as the system may not have
 * restore the UARTs state yet
*/

void s5pc11x_pm_do_restore(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		//printk(KERN_DEBUG "restore %p (restore %08lx, was %08x)\n",
		       //ptr->reg, ptr->val, __raw_readl(ptr->reg));

		__raw_writel(ptr->val, ptr->reg);
	}
}

#ifdef S5PC11X_ALIVEGPIO_STORE
void s5pc11x_pm_do_restore_alive(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		__raw_writel(ptr->val, ptr->reg);
		__raw_readl(ptr->reg);

		//printk(" value = %x adress=%x data =%x\n",ptr->val,ptr->reg,__raw_readl(ptr->reg));
	}
}
#endif

/* s5pc11x_pm_do_restore_core
 *
 * similar to s36410_pm_do_restore_core
 *
 * WARNING: Do not put any debug in here that may effect memory or use
 * peripherals, as things may be changing!
*/

/* s5pc11x_pm_do_save_phy
 *
 * save register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

void s5pc11x_pm_do_save_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		ptr->val = readl(target_reg + (ptr->reg));
	}
}

/* s5pc11x_pm_do_restore_phy
 *
 * restore register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

void s5pc11x_pm_do_restore_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		writel(ptr->val, (target_reg + ptr->reg));
	}
}

void (*pm_cpu_prep)(void);
void (*pm_cpu_sleep)(void);

#define any_allowed(mask, allow) (((mask) & (allow)) != (allow))


#define eint_offset(irq)                (irq)
#define eint_irq_to_bit(irq)            (1 << (eint_offset(irq) & 0x7))
#define eint_conf_reg(irq)              ((eint_offset(irq)) >> 3)
#define eint_filt_reg(irq)              ((eint_offset(irq)) >> 2)
#define eint_mask_reg(irq)              ((eint_offset(irq)) >> 3)
#define eint_pend_reg(irq)              ((eint_offset(irq)) >> 3)

// intr_mode 0x2=>falling edge, 0x3=>rising dege, 0x4=>Both edge
static void s5pc11x_pm_set_eint(unsigned int irq, unsigned int intr_mode)
{
	int offs = (irq);
	int shift;
	u32 ctrl, mask, tmp;
	//u32 newvalue = 0x2; // Falling edge

	shift = (offs & 0x7) * 4;
	if((0 <= offs) && (offs < 8)){
		tmp = readl(S5PC11X_GPH0CON);
		tmp |= (0xf << shift);
		writel(tmp , S5PC11X_GPH0CON);
#ifdef S5PC11X_ALIVEGPIO_STORE
		readl(S5PC11X_GPH0CON); // FIX for EVT0 bug
#endif
		/*pull up disable*/
	}
	else if((8 <= offs) && (offs < 16)){
		tmp = readl(S5PC11X_GPH1CON);
		tmp |= (0xf << shift);
		writel(tmp , S5PC11X_GPH1CON);
#ifdef S5PC11X_ALIVEGPIO_STORE
		readl(S5PC11X_GPH1CON); // FIX for EVT0 bug
#endif
	}
	else if((16 <= offs) && (offs < 24)){
		tmp = readl(S5PC11X_GPH2CON);
		tmp |= (0xf << shift);
		writel(tmp , S5PC11X_GPH2CON);
#ifdef S5PC11X_ALIVEGPIO_STORE
		readl(S5PC11X_GPH2CON); // FIX for EVT0 bug
#endif
	}
	else if((24 <= offs) && (offs < 32)){
		tmp = readl(S5PC11X_GPH3CON);
		tmp |= (0xf << shift);
		writel(tmp , S5PC11X_GPH3CON);
#ifdef S5PC11X_ALIVEGPIO_STORE
		readl(S5PC11X_GPH3CON); // FIX for EVT0 bug
#endif
	}
	else{
		printk(KERN_ERR "No such irq number %d", offs);
		return;
	}

	/*special handling for keypad eint*/
	if( (24 <= irq) && (irq <= 27))
	{// disable the pull up
		tmp = readl(S5PC11X_GPH3PUD);
		tmp &= ~(0x3 << ((offs & 0x7) * 2));	
		writel(tmp, S5PC11X_GPH3PUD);
#ifdef S5PC11X_ALIVEGPIO_STORE
		readl(S5PC11X_GPH3PUD); // FIX for EVT0 bug
#endif
		DBG("S5PC11X_GPH3PUD = %x\n",readl(S5PC11X_GPH3PUD));
	}
	

	/*Set irq type*/
	mask = 0x7 << shift;
	ctrl = readl(S5PC11X_EINTCON(eint_conf_reg(irq)));
	ctrl &= ~mask;
	//ctrl |= newvalue << shift;
	ctrl |= intr_mode << shift;

	writel(ctrl, S5PC11X_EINTCON(eint_conf_reg(irq)));
#ifdef S5PC11X_ALIVEGPIO_STORE
	readl(S5PC11X_EINTCON(eint_conf_reg(irq)));
#endif
	/*clear mask*/
	mask = readl(S5PC11X_EINTMASK(eint_mask_reg(irq)));
	mask &= ~(eint_irq_to_bit(irq));
	writel(mask, S5PC11X_EINTMASK(eint_mask_reg(irq)));
#ifdef S5PC11X_ALIVEGPIO_STORE
	readl(S5PC11X_EINTMASK(eint_mask_reg(irq)));
#endif

	/*clear pending*/
	mask = readl(S5PC11X_EINTPEND(eint_pend_reg(irq)));
	mask &= (eint_irq_to_bit(irq));
	writel(mask, S5PC11X_EINTPEND(eint_pend_reg(irq)));
#ifdef S5PC11X_ALIVEGPIO_STORE	
	readl(S5PC11X_EINTPEND(eint_pend_reg(irq)));
#endif
	
	/*Enable wake up mask*/
	tmp = readl(S5P_EINT_WAKEUP_MASK);
	tmp &= ~(1 << (irq));
	writel(tmp , S5P_EINT_WAKEUP_MASK);

	DBG("S5PC11X_EINTCON = %x\n",readl(S5PC11X_EINTCON(eint_conf_reg(irq))));
	DBG("S5PC11X_EINTMASK = %x\n",readl(S5PC11X_EINTMASK(eint_mask_reg(irq))));
	DBG("S5PC11X_EINTPEND = %x\n",readl(S5PC11X_EINTPEND(eint_pend_reg(irq))));
	
	return;
}


static void s5pc11x_pm_clear_eint(unsigned int irq)
{
	u32 mask;
	/*clear pending*/
	mask = readl(S5PC11X_EINTPEND(eint_pend_reg(irq)));
	mask &= (eint_irq_to_bit(irq));
	writel(mask, S5PC11X_EINTPEND(eint_pend_reg(irq)));
#ifdef S5PC11X_ALIVEGPIO_STORE	
	readl(S5PC11X_EINTPEND(eint_pend_reg(irq)));
#endif
}

extern unsigned int HWREV;
extern int hw_version_check();
extern short gp2a_get_proximity_enable(void);
#define BOOT_ARM_CLK	800000 //800MHz
/* s5pc11x_pm_enter
 *
 * central control for sleep/resume process
*/

extern unsigned int is_calling_or_playing;
#define IS_VOICE_CALL_2G		(0x1 << 4)
#define IS_VOICE_CALL_3G		(0x1 << 5)
#define IS_DATA_CALL		(0x1 << 6)

static int s5pc11x_pm_enter(suspend_state_t state)
{
	unsigned long regs_save[16];
	unsigned int tmp;


#ifdef CONFIG_HAS_WAKELOCK
	//wake_unlock(&pm_wake_lock);
#endif

	/* ensure the debug is initialised (if enabled) */
	DBG("s5pc11x_pm_enter(%d)\n", state);

	if (pm_cpu_prep == NULL || pm_cpu_sleep == NULL) {
		printk(KERN_ERR PFX "error: no cpu sleep functions set\n");
		return -EINVAL;
	}

#ifdef CONFIG_CPU_FREQ
	s5pc110_pm_target(BOOT_ARM_CLK);
#endif

	/* store the physical address of the register recovery block */
	s5pc110_sleep_save_phys = virt_to_phys(regs_save);

	DBG("s5pc11x_sleep_save_phys=0x%08lx\n", s5pc110_sleep_save_phys);

	s5pc11x_pm_do_save(gpio_save, ARRAY_SIZE(gpio_save));
#ifdef S5PC11X_ALIVEGPIO_STORE
	s5pc11x_pm_do_save(gpio_save_alive, ARRAY_SIZE(gpio_save_alive));
#endif
	s5pc11x_pm_do_save(irq_save, ARRAY_SIZE(irq_save));
	s5pc11x_pm_do_save(core_save, ARRAY_SIZE(core_save));
	s5pc11x_pm_do_save(sromc_save, ARRAY_SIZE(sromc_save));
	s5pc11x_pm_do_save(uart_save, ARRAY_SIZE(uart_save));


	/* ensure INF_REG0  has the resume address */
	__raw_writel(virt_to_phys(s5pc110_cpu_resume), S5P_INFORM0);

	/* call cpu specific preperation */
	pm_cpu_prep();

	/* flush cache back to ram */
	flush_cache_all();

#if 0		// To preserve 24MHz clock.
	/* USB & OSC Clock pad Enable */
	tmp = __raw_readl(S5P_SLEEP_CFG);
	//tmp |= (S5P_SLEEP_CFG_OSC_EN | S5P_SLEEP_CFG_USBOSC_EN);
	tmp &= ~(S5P_SLEEP_CFG_OSC_EN | S5P_SLEEP_CFG_USBOSC_EN);
	__raw_writel(tmp , S5P_SLEEP_CFG);
#endif
	__raw_writel(0xffffffff , S5P_EINT_WAKEUP_MASK);

	/* Power mode Config setting */
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	tmp |= S5P_CFG_WFI_SLEEP;
	__raw_writel(tmp,S5P_PWR_CFG);

	if (!hw_version_check()) {
	/* Set wakeup mask regsiter */
	__raw_writel(0xFFED, S5P_WAKEUP_MASK); 
	} else {

		if((is_calling_or_playing & IS_VOICE_CALL_2G) || (is_calling_or_playing & IS_VOICE_CALL_3G) || (is_calling_or_playing & IS_DATA_CALL)){
			__raw_writel(0xFFDD, S5P_WAKEUP_MASK); //0xFFDD:key, RTC_ALARM	
		}else{
		__raw_writel(0xFFFD, S5P_WAKEUP_MASK); //0xFFDD:key, RTC_ALARM	
	}
	}

	__raw_writel(0xffffffff, S5PC110_VIC0REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC1REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC2REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC3REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC0REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC1REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC2REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC3REG(VIC_INT_SOFT_CLEAR));

	/* SYSC INT Disable */
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= (S5P_OTHER_SYSC_INTOFF);
	__raw_writel(tmp,S5P_OTHERS);

	 /* Clear WAKEUP_STAT register for next wakeup */
        tmp = __raw_readl(S5P_WAKEUP_STAT);
        __raw_writel(tmp, S5P_WAKEUP_STAT);

	/* Wake up source setting */
        //s5pc11x_pm_configure_extint();

	// key pad direction control for evt0
	//s5pc11x_set_keypad_sleep_gpio();
	
	/*Set EINT 22 as wake up source*/
	s5pc11x_pm_set_eint(11, 0x2);
	s5pc11x_pm_set_eint(22, 0x2);
	s5pc11x_pm_set_eint(15, 0x4);
	s5pc11x_pm_set_eint(21, 0x4);
	s5pc11x_pm_set_eint(7,  0x02);		//PMIC	
	s5pc11x_pm_set_eint(6, 0x4); //det_3.5

	s5pc11x_pm_set_eint(28, 0x4);	// T_FLASH_DETECT
//[hdlnc_bp_ytkwon : 20100326
	#ifdef CONFIG_KEPLER_AUDIO_A1026
		if(HWREV!=0x08)
		{
    			if(get_headset_status() & SEC_HEADSET_4_POLE_DEVICE)
			{
				s5pc11x_pm_set_eint(30, 0x4); //sendend
				s5pc11x_pm_set_eint(18, 0x4); //sendend 2.5
			}
   			else
   			{
       			s5pc11x_pm_clear_eint(30);
	   			s5pc11x_pm_clear_eint(18);
   			}
		}
	#else
		if(HWREV==0x0a ||HWREV==0x0c)
		{
   			if(get_headset_status() & SEC_HEADSET_4_POLE_DEVICE)
			{
				s5pc11x_pm_set_eint(30, 0x4); //sendend
			}
			else
   			{
     			s5pc11x_pm_clear_eint(30);
       		}
			
		}
		else
		{
   			if(get_headset_status() & SEC_HEADSET_4_POLE_DEVICE)
			{
				s5pc11x_pm_set_eint(30, 0x4); //sendend
				s5pc11x_pm_set_eint(18, 0x4); //sendend 2.5
			}
   			else
   			{
	   			s5pc11x_pm_clear_eint(30);
       			s5pc11x_pm_clear_eint(18);
  			}
		}
	#endif
//]hdlnc_bp_ytkwon : 20100326
		
	if(gp2a_get_proximity_enable())
	{
	    s5pc11x_pm_set_eint(2, 0x4);//proximity
	}
	s5pc11x_pm_set_eint(20, 0x3);//WiFi
	s5pc11x_pm_set_eint(23, 0x2);//microusb

#if defined CONFIG_T959_VER_B0
	s5pc11x_pm_set_eint(29, 0x4);
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#elif defined CONFIG_KEPLER_VER_B0
#elif defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)
	s5pc11x_pm_set_eint(27, 0x2);
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#else	
	//gpio key
	if(HWREV >= 0xB)
	{
		s5pc11x_pm_set_eint(27, 0x4);
		s5pc11x_pm_set_eint(29, 0x4);
	}
#endif
	
	if (!hw_version_check()) {
	/*Set keypad as EINT for EVT0 wake up workaround*/
	s5pc11x_pm_set_eint(24, 0x2);
	s5pc11x_pm_set_eint(25, 0x2);
	s5pc11x_pm_set_eint(26, 0x2);
	s5pc11x_pm_set_eint(27, 0x2);

	/*Column pull down enabled*/
	tmp = readl(S5PC11X_GPH2PUD);
	tmp &= ~(0xFF);
	tmp |= 0x55;	
	writel(tmp, S5PC11X_GPH2PUD);
	}

	s3c_config_sleep_gpio();

	s3c_gpio_slp_cfgpin(S5PC11X_MP03(3),  S3C_GPIO_SLP_OUT0);
	s3c_gpio_slp_setpull_updown(S5PC11X_MP03(3),  S3C_GPIO_SLP_OUT0);
#if 0
        tmp = __raw_readl(S5P_OTHERS);
        tmp &= ~(3 << 8);
        tmp |= (3 << 8);
        __raw_writel(tmp, S5P_OTHERS);

        __raw_writel(0,S5P_MIE_CONTROL);
        __raw_writel(0,S5P_HDMI_CONTROL);
        __raw_writel(0,S5P_USB_PHY_CONTROL);
        __raw_writel(0,S5P_DAC_CONTROL);
        __raw_writel(0,S5P_MIPI_PHY_CONTROL);
        __raw_writel(0,S5P_ADC_CONTROL);
        __raw_writel(0,S5P_PSHOLD_CONTROL);
#endif

#if (!(defined CONFIG_ARIES_VER_B0) && !(defined CONFIG_ARIES_VER_B4) && !(defined CONFIG_ARIES_VER_B5))
// Enable PS_HOLD pin to avoid reset failure */
        __raw_writel((0x5 << 12 | 0x1<<9 | 0x1<<8 | 0x1<<0),S5P_PSHOLD_CONTROL);
#endif

	/* s5pc11x_cpu_save will also act as our return point from when
	 * we resume as it saves its own register state, so use the return
	 * code to differentiate return from save and return from sleep */

	if (s5pc110_cpu_save(regs_save) == 0) {
		flush_cache_all();
		if (!hw_version_check()) {		
		/* This function for Chip bug on EVT0 */
		tmp = __raw_readl(S5P_EINT_WAKEUP_MASK + 4); //PWR_MODE
		tmp |= (1 << 2);
		__raw_writel(tmp , S5P_EINT_WAKEUP_MASK + 4);
		// end mod
		}
		pm_cpu_sleep();
	}

	/* restore the cpu state */
	cpu_init();

	s5pc11x_pm_do_restore(gpio_save, ARRAY_SIZE(gpio_save));
#ifdef S5PC11X_ALIVEGPIO_STORE
	s5pc11x_pm_do_restore_alive(gpio_save_alive, ARRAY_SIZE(gpio_save_alive));
#endif
	s5pc11x_pm_do_restore(irq_save, ARRAY_SIZE(irq_save));
	__raw_writel(0x0, S3C24XX_VA_UART2+S3C2410_UCON);
	__raw_writel(0xf, S3C24XX_VA_UART2+S5P_UINTP);
	__raw_writel(0xf, S3C24XX_VA_UART2+S5P_UINTSP);
	__raw_writel(0xf, S3C24XX_VA_UART2+S5P_UINTM);

	/*		Temporary workaround to protect lockup by UART	- 20100316	*/
	    __raw_writel(0x0, S3C24XX_VA_UART3+S3C2410_UCON);
	    __raw_writel(0xf, S3C24XX_VA_UART3+S5P_UINTM);
	    __raw_writel(0xf, S3C24XX_VA_UART3+S5P_UINTSP);
	    __raw_writel(0xf, S3C24XX_VA_UART3+S5P_UINTP);
	    __raw_writel(0x0, S3C24XX_VA_UART2+S3C2410_UCON);
	    __raw_writel(0xf, S3C24XX_VA_UART2+S5P_UINTM);
	    __raw_writel(0xf, S3C24XX_VA_UART2+S5P_UINTSP);
	    __raw_writel(0xf, S3C24XX_VA_UART2+S5P_UINTP);
	    __raw_writel(0x0, S3C24XX_VA_UART1+S3C2410_UCON);
	    __raw_writel(0xf, S3C24XX_VA_UART1+S5P_UINTM);
	    __raw_writel(0xf, S3C24XX_VA_UART1+S5P_UINTSP);
	    __raw_writel(0xf, S3C24XX_VA_UART1+S5P_UINTP);
	    __raw_writel(0x0, S3C24XX_VA_UART0+S3C2410_UCON);
	    __raw_writel(0xf, S3C24XX_VA_UART0+S5P_UINTM);
	    __raw_writel(0xf, S3C24XX_VA_UART0+S5P_UINTSP);
	    __raw_writel(0xf, S3C24XX_VA_UART0+S5P_UINTP);


	s5pc11x_pm_do_restore(uart_save, ARRAY_SIZE(uart_save));
	s5pc11x_pm_do_restore(core_save, ARRAY_SIZE(core_save));
	s5pc11x_pm_do_restore(sromc_save, ARRAY_SIZE(sromc_save));

	/*enable gpio, uart, mmc*/
        tmp = __raw_readl(S5P_OTHERS);
#if ((defined CONFIG_ARIES_VER_B0) || (defined CONFIG_ARIES_VER_B4) || (defined CONFIG_ARIES_VER_B5))
        tmp |= (1<<31) | (1<<28) | (1<<29);
#else
        tmp |= (1<<31) | (0x1<<30) | (1<<28) | (1<<29);
#endif
        __raw_writel(tmp, S5P_OTHERS);

	/* EINT22 Pending clear */ 
	//s5pc11x_pm_clear_eint(22); //<= do action in s3c-keypad.c
//	s5pc11x_pm_clear_eint(21);
	if (!hw_version_check()) {
	// for evt 0 keypad wakeup workaround
		s5pc11x_pm_clear_eint(24);
		s5pc11x_pm_clear_eint(25);
		s5pc11x_pm_clear_eint(26);
		s5pc11x_pm_clear_eint(27);
	        s5pc11x_pm_clear_eint(21);
	} else {
		 /* Clear WAKEUP_STAT register for next wakeup */
		tmp = __raw_readl(S5P_WAKEUP_STAT);
		__raw_writel(tmp, S5P_WAKEUP_STAT);	

		printk("wakeup source is 0x%x  \n", tmp);
		printk(" EXT_INT_0_PEND       %x \n", __raw_readl(S5PC11X_EINTPEND(0)));
		printk(" EXT_INT_1_PEND       %x \n", __raw_readl(S5PC11X_EINTPEND(1)));
		printk(" EXT_INT_2_PEND       %x \n", __raw_readl(S5PC11X_EINTPEND(2)));
		printk(" EXT_INT_3_PEND       %x \n", __raw_readl(S5PC11X_EINTPEND(3)));
	}

	DBG("\npost sleep, preparing to return 2\n");

	s5pc11x_pm_check_restore();

#ifdef CONFIG_HAS_WAKELOCK
        //wake_lock_timeout(&pm_wake_lock, 5 * HZ);
#endif

	/* ok, let's return from sleep */
	DBG("S5PC110 PM Resume (post-restore)\n");

	return 0;
}


static struct platform_suspend_ops s5pc11x_pm_ops = {
	.enter		= s5pc11x_pm_enter,
	.valid		= suspend_valid_only_mem,
};

/* s5pc11x_pm_init
 *
 * Attach the power management functions. This should be called
 * from the board specific initialisation if the board supports
 * it.
*/

int __init s5pc11x_pm_init(void)
{
	printk("s5pc11x Power Management, (c) 2008 Samsung Electronics\n");

#ifdef CONFIG_HAS_WAKELOCK
        wake_lock_init(&pm_wake_lock, WAKE_LOCK_SUSPEND, "pm_wake_lock");
#endif
	weint_base = ioremap(S5P_APM_BASE, 0x350);
        if(!weint_base) {
                printk("Unable to allocate memory\n");
                return -1;
        }

#if 0
	u32 tmp,ret;
	/*Set EINT22 as wake up*/

	set_irq_type(IRQ_EINT(22), IRQ_TYPE_EDGE_FALLING);
        ret = setup_irq(IRQ_EINT(22), &s3c_gpio_irq);
        if (ret) {
                printk("request_irq failed (EINT16-31) !!!\n");
		return;
        }

#endif
#ifdef CONFIG_PM_PWR_GATING
	//s5p_init_domain_power();
#endif
	// ABB register setup
	__raw_writel(0x00800000, S5P_ABB_VALUE ); //0xe010c300


	/* set the irq configuration for wake */
	suspend_set_ops(&s5pc11x_pm_ops);
	return 0;
}
