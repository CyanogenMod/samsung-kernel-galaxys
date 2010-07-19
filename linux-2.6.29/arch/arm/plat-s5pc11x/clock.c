/* linux/arch/arm/plat-s5pc11x/clock.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5PC1XX Base clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/regs-power.h>
#include <plat/regs-audss.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>

struct clk clk_27m = {
	.name		= "clk_27m",
	.id		= -1,
	.rate		= 27000000,
};

#ifdef CONFIG_PM_PWR_GATING
extern int s5p_power_gating(unsigned int power_domain, unsigned int on_off);
extern int s5p_domain_off_check(unsigned int power_domain);
int tvblk_turnon;
#endif
static int inline s5pc11x_clk_gate(void __iomem *reg,
				struct clk *clk,
				int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	u32 con, mask;

	con = __raw_readl(reg);


	if (enable) {

		/*Disable the src mask before enabling the gating*/
		if(clk->srcMaskReg)
		{
			mask = __raw_readl(clk->srcMaskReg);
			mask |= (clk->srcMaskBit);
			__raw_writel(mask, clk->srcMaskReg);
			//printk("\nenable mask for %s\n",clk->name);
		}
#ifdef CONFIG_PM_PWR_GATING
		if(clk->powerDomain)
		{
			/*if ((clk->powerDomain == S5PC110_POWER_DOMAIN_TV) && s5p_domain_off_check(S5PC110_POWER_DOMAIN_TV)) {
				con |= 0x0f00; // enable VP, Mixer, TVEnc, HDMI
				tvblk_turnon=1;
			}*/	
			s5p_power_gating(clk->powerDomain,  DOMAIN_ACTIVE_MODE);
		}
#endif
		con |= ctrlbit;
		__raw_writel(con, reg);
#ifdef CONFIG_PM_PWR_GATING		
		/*if (tvblk_turnon) {
			con =  (con & ~(0x0f00)) | ctrlbit; // disable others in VP, Mixer, TVEnc, HDMI
			__raw_writel(con, reg);		
			tvblk_turnon = 0;
		}*/
#endif		
	} else {
		con &= ~ctrlbit;
		__raw_writel(con, reg);
#ifdef CONFIG_PM_PWR_GATING
		if(clk->powerDomain)
		{
			s5p_power_gating(clk->powerDomain,  DOMAIN_LP_MODE);
		}
#endif
		/*Enable the src mask before enabling the gating*/
		if(clk->srcMaskReg)
		{
			mask = __raw_readl(clk->srcMaskReg);
			mask &= (~(clk->srcMaskBit));
			__raw_writel(mask, clk->srcMaskReg);
			//printk("\ndisable mask for %s\n",clk->name);
		}
	}
	return 0;
}


int s5pc11x_clk_ip0_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP0, clk, enable);
}

int s5pc11x_clk_ip1_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP1, clk, enable);
}

int s5pc11x_clk_ip2_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP2, clk, enable);
}

int s5pc11x_clk_ip3_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP3, clk, enable);
}

int s5pc11x_clk_ip4_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP4, clk, enable);
}

int s5pc11x_clk_block_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_BLOCK, clk, enable);
}

int s5pc11x_audss_clkctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_AUDSS, clk, enable);
}

static struct clk init_clocks_disable[] = {
/*Disable: IP0*/
	{
		.name		= "csis",
		.id		= -1,
		.parent		= &clk_p83,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_CSIS,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_CSIS,
	}, {
		.name		= "ipc",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_IPC,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
	}, {
		.name		= "rotator",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_ROTATOR,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
	}, {
		.name		= "jpeg",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_JPEG,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
	}, {
		.name		= "fimc2",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_FIMC2,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
		.srcMaskReg 	= S5P_CLK_SRC_MASK1,
		.srcMaskBit 	= S5P_CLKSRC_MASK1_FIMC2_LCLK,
	}, {
		.name		= "fimc1",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_FIMC1,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
		.srcMaskReg 	= S5P_CLK_SRC_MASK1,
		.srcMaskBit 	= S5P_CLKSRC_MASK1_FIMC1_LCLK,
	}, {
		.name		= "fimc0",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_FIMC0,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
		.srcMaskReg 	= S5P_CLK_SRC_MASK1,
		.srcMaskBit 	= S5P_CLKSRC_MASK1_FIMC0_LCLK,
	}, {	
		.name		= "mfc",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_MFC,
		.powerDomain = S5PC110_POWER_DOMAIN_MFC,
	}, {
		.name		= "g2d",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_G2D,
		.powerDomain = S5PC110_POWER_DOMAIN_LCD,
	}, {
#if 0
		.name		= "g3d",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_G3D,
		.powerDomain = S5PC110_POWER_DOMAIN_G3D,
	}, {
		.name		= "imem",
		.id		= -1,
		.parent		= &clk_p100,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_IMEM,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
		.name		= "pdma1",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_PDMA1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "pdma0",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_PDMA0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "mdma",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_MDMA,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
/*Disable: IP1*/
		.name		= "nfcon",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_NFCON,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "sromc",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_SROMC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "cfcon",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_CFCON,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#if 0
		.name		= "nandxl",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_NANDXL,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
		.name		= "usbhost",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_USBHOST,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name           = "otg",
		.id             = -1,
		.parent         = &clk_h133,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_USBOTG,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "hdmi",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_HDMI,
		.powerDomain = S5PC110_POWER_DOMAIN_TV,
		//.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		//.srcMaskBit 	= S5P_CLKSRC_MASK0_HDMI,
	}, {
		.name		= "tvenc",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_TVENC,
		.powerDomain = S5PC110_POWER_DOMAIN_TV,
		//.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		//.srcMaskBit 	= S5P_CLKSRC_MASK0_DAC,
	}, {
		.name		= "mixer",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_MIXER,
		.powerDomain = S5PC110_POWER_DOMAIN_TV,
		//.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		//.srcMaskBit 	= S5P_CLKSRC_MASK0_MIXER,
	}, {
		.name		= "vp",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_VP,
		.powerDomain = S5PC110_POWER_DOMAIN_TV,
	}, {
		.name		= "dsim",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_DSIM,
		.powerDomain = S5PC110_POWER_DOMAIN_LCD,
	}, {
#ifndef CONFIG_FB_S3C_MDNIE

		.name		= "mdnie",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_MIE,
		.powerDomain = S5PC110_POWER_DOMAIN_LCD,
	}, {
#endif	
#if 0
		.name		= "lcd",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_FIMD,
		.powerDomain = S5PC110_POWER_DOMAIN_LCD,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_FIMD,
	}, {
#endif
/*Disable: IP2*/
		.name		= "tzic3",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_TZIC3,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzic2",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_TZIC2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzic1",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_TZIC1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzic0",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_TZIC0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tsi",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_TSI,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "hsmmc",
		.id		= 3,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC3,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_MMC3,
	}, {
		.name		= "hsmmc",
		.id		= 2,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_MMC2,
	}, {
		.name		= "hsmmc",
		.id		= 1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_MMC1,
	}, {
		.name		= "hsmmc",
		.id		= 0,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_MMC0,
    }, {
		.name		= "secjtag",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_SECJTAG,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
    }, {
		.name		= "hostif",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HOSTIF,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
    }, {
		.name		= "modem",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_MODEM,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
    }, {
#if 0//jtag enable        
		.name		= "coresight",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_CORESIGHT,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
        }, {
#endif        
		.name		= "sdm",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_SDM,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
    }, {

		.name		= "secss",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_SECSS,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
    }, {
/*Disable: IP3*/
#if 1
		.name		= "pcm",
		.id		= 2,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "pcm",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM1 | S5P_CLKGATE_IP3_I2S1 ,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "pcm",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
#if 0
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_TSADC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif	
#if 0
		.name		= "timers",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PWM,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
#if 0 //watchdog enable
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_WDT,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif	
		.name		= "keypad",
		.id		= -1,
		.parent		 = &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_KEYIF,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "uart",
		.id		= 3,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART3,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#if 0
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "systimer",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SYSTIMER,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#if 0
		.name		= "rtc",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_RTC,
		.powerDomain = S5PC110_POWER_DOMAIN_RTC,
	}, {
#endif
		.name           = "spi",
		.id             = 2,
		.parent         = &clk_p66,
		.enable         = s5pc11x_clk_ip3_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_SPI2,
    }, {
		.name           = "spi",
		.id             = 1,
		.parent         = &clk_p66,
		.enable         = s5pc11x_clk_ip3_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_SPI1,
    }, {
		.name           = "spi",
		.id             = 0,
		.parent         = &clk_p66,
		.enable         = s5pc11x_clk_ip3_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_SPI0,
    }, {
		.name		= "i2c",
		.id			= 3,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C_HDMI_PHY,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "i2c",
		.id			= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "i2c",
		.id			= 1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C_HDMI_DDC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "i2c",
		.id			= 0,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
//	}, {
//		.name		= "i2s_v50",
//		.id		= 0,
//		.parent		= &clk_p,
//		.enable		= s5pc11x_clk_ip3_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP3_I2S0 | S5P_CLKGATE_IP3_PCM0, /* I2S0 is v5.0 */
////		.powerDomain = S5PC110_POWER_DOMAIN_AUDIO,
//		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
//		.srcMaskBit 	= S5P_CLKSRC_MASK0_AUDIO0,
	}, {
		.name		= "i2s_v32",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S1 | S5P_CLKGATE_IP3_PCM1, /* I2S1 is v3.2 */
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_AUDIO1,
	}, {
		.name		= "i2s_v32",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S2, /* I2S2 is v3.2 */
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_AUDIO2,
	}, {
		.name		= "ac97",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_AC97,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {	
		.name		= "spdif",
		.id		= -1,
		.parent		= NULL,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SPDIF | S5P_CLKGATE_IP3_PCM0 | S5P_CLKGATE_IP3_PCM1 | S5P_CLKGATE_IP3_I2S0 | S5P_CLKGATE_IP3_I2S1 | S5P_CLKGATE_IP3_I2S2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_SPDIF,
/*Disable: IP4*/
	}, {
		.name		= "tzpc",
		.id		= 0,
		.parent		= &clk_p100,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_TZPC0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzpc",
		.id		= 1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_TZPC1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzpc",
		.id		= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_TZPC2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzpc",
		.id		= 3,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_TZPC3,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "seckey",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_SECKEY,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "iem_apc",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_IEM_APC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "iem_iec",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_IEM_IEC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "chip_id",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_CHIP_ID,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}
};

static struct clk init_clocks[] = {
/*Enable: IP0*/
	{	
//		.name		= "csis",
//		.id		= -1,
//		.parent		= &clk_p83,
//		.enable		= s5pc11x_clk_ip0_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP0_CSIS,
//		.powerDomain = ,
//	}, {
//		.name		= "ipc",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip0_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP0_IPC,
//		.powerDomain = ,
//	}, {
//		.name		= "rotator",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip0_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP0_ROTATOR,
//		.powerDomain = ,
//	}, {n
//		.name		= "jpeg",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip0_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP0_JPEG,
//		.powerDomain = ,
//	}, {
#if 0 // temprary
		.name		= "fimc2",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_FIMC2,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
	}, {
		.name		= "fimc1",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_FIMC1,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
	}, {
		.name		= "fimc0",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_FIMC0,
		.powerDomain = S5PC110_POWER_DOMAIN_CAM,
	}, {
		.name		= "g2d",
		.id			= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_G2D,
		.powerDomain = S5PC110_POWER_DOMAIN_LCD,
	}, {
#endif
		.name		= "g3d",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_G3D,
		.powerDomain = S5PC110_POWER_DOMAIN_G3D,
	}, {
		.name		= "imem",
		.id		= -1,
		.parent		= &clk_p100,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_IMEM,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
//		.name		= "pdma1",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip0_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP0_PDMA1,
//		.powerDomain = ,
//	}, {
//		.name		= "pdma0",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip0_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP0_PDMA0,
//		.powerDomain = ,
//	}, {
//		.name		= "mdma",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip0_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP0_MDMA,
//		.powerDomain = ,
//	}, {
		.name		= "dmc1",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_DMC1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "dmc0",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_DMC0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {

/*Enable: IP1*/
//		.name		= "nfcon",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP1_NFCON,
//		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
//	}, {
#if 0
		.name		= "sromc",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_SROMC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif	
#if 0
		.name		= "cfcon",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_CFCON,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
		.name		= "nandxl",
		.id		= -1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_NANDXL,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
//		.name		= "usbhost",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP1_USBHOST,
//		.powerDomain = 
//	}, {
//		.name		= "usbotg",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP1_USBOTG,
//		.powerDomain = 
//	}, {
//		.name		= "hdmi",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP1_HDMI,
//		.powerDomain = 
//	}, {
//		.name		= "tvenc",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP1_TVENC,
//		.powerDomain = 
//	}, {
//		.name		= "mixer",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP1_MIXER,
//		.powerDomain = 
//	}, {
//		.name		= "vp",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP1_VP,
//		.powerDomain = 
//	}, {
//		.name		= "dsim",
//		.id		= -1,
//		.parent		= &clk_h166,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP1_DSIM,
//		.powerDomain = 
//	}, {
#ifdef CONFIG_FB_S3C_MDNIE

		.name		= "mdnie",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_MIE,
		.powerDomain = S5PC110_POWER_DOMAIN_LCD,
	}, {
#endif
		.name		= "lcd",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_FIMD,
		.powerDomain = S5PC110_POWER_DOMAIN_LCD,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_FIMD,
	}, {
/*Enable: IP2*/
//		.name		= "tzic3",
//		.id		= -1,
//		.parent		= &clk_h200,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_TZIC3,
//		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
//	}, {
//		.name		= "tzic2",
//		.id		= -1,
//		.parent		= &clk_h200,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_TZIC2,
//		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
//	}, {
//		.name		= "tzic1",
//		.id		= -1,
//		.parent		= &clk_h200,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_TZIC1,
//		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
//	}, {
//		.name		= "tzic0",
//		.id		= -1,
//		.parent		= &clk_h200,
//		.enable		= s5pc11x_clk_ip1_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_TZIC0,
//		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
//	}, {
		.name		= "vic3",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_VIC3,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "vic2",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_VIC2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "vic1",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_VIC1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "vic0",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_VIC0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
//		.name		= "tsi",
//		.id		= -1,
//		.parent		= &clk_p66,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_TSI,
//		.powerDomain = 
//	}, {
//		.name		= "hsmmc",
//		.id		= 3,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC3,
//		.powerDomain = 
//	}, {
//		.name		= "hsmmc",
//		.id		= 2,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC2,
//		.powerDomain = 
//	}, {
//		.name		= "hsmmc",
//		.id		= 1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC1,
//		.powerDomain = 
//	}, {
//		.name		= "hsmmc",
//		.id		= 0,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC0,
//		.powerDomain = 
//        }, {
//		.name		= "secjtag",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_SECJTAG,
//		.powerDomain = 
//        }, {
//		.name		= "hostif",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_HOSTIF,
//		.powerDomain = 
//        }, {
//		.name		= "modem",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_MODEM,
//		.powerDomain = 
//      }, {
#if 1// enble jtag
		.name		= "coresight",
		.id		= -1,
		.parent		= &clk_h200,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_CORESIGHT,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
        }, {
#endif
//		.name		= "sdm",
//		.id		= -1,
//		.parent		= &clk_h200,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_SDM,
//		.powerDomain = ,
//      }, {
//		.name		= "secss",
//		.id		= -1,
//		.parent		= &clk_h133,
//		.enable		= s5pc11x_clk_ip2_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP2_SECSS,
//		.powerDomain = ,
//        }, {

/*Enable: IP3*/
#if 0
		.name		= "pcm",
		.id		= 2,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "pcm",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "pcm",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
		.name		= "syscon",
		.id		= 0,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SYSCON,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "gpio",
		.id		= 0,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_GPIO,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#if 1
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_TSADC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
		.name		= "timers",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PWM,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#if 1 //enable watchdog
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_WDT,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif	
#if 0
		.name		= "keypad",
		.id		= -1,
		.parent		 = &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_KEYIF,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "uart",
		.id		= 3,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART3,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_UART3,
	}, {
#endif	
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_UART2,
	}, {
#if 0
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_UART1,
	}, {
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_UART0,
	}, {
#endif
		.name		= "systimer",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SYSTIMER,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "rtc",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_RTC,
		.powerDomain = S5PC110_POWER_DOMAIN_RTC,
	}, {
#if 0	
		.name           = "spi",
		.id             = 2,
		.parent         = &clk_p66,
		.enable         = s5pc11x_clk_ip3_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
        }, {
		.name           = "spi",
		.id             = 1,
		.parent         = &clk_p66,
		.enable         = s5pc11x_clk_ip3_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
        }, {
		.name           = "spi",
		.id             = 0,
		.parent         = &clk_p66,
		.enable         = s5pc11x_clk_ip3_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
        }, {
		.name		= "i2c",
		.id			= 3,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C_HDMI_PHY,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "i2c",
		.id			= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "i2c",
		.id			= 1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C_HDMI_DDC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "i2c",
		.id			= 0,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif	
		.name		= "i2s_v50",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S0 | S5P_CLKGATE_IP3_PCM0, /* I2S0 is v5.0 */
		.powerDomain 	= S5PC110_POWER_DOMAIN_AUDIO,
		.srcMaskReg 	= S5P_CLK_SRC_MASK0,
		.srcMaskBit 	= S5P_CLKSRC_MASK0_AUDIO0,	
#if 0	
	}, {
		.name		= "i2s_v32",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S1 | S5P_CLKGATE_IP3_PCM1, /* I2S1 is v3.2 */
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "i2s_v32",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S2, /* I2S2 is v3.2 */
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "ac97",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_AC97,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,	
	}, {	
		.name		= "spdif",
		.id		= -1,
		.parent		= NULL,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SPDIF | S5P_CLKGATE_IP3_PCM0 | S5P_CLKGATE_IP3_PCM1 | S5P_CLKGATE_IP3_I2S0 | S5P_CLKGATE_IP3_I2S1 | S5P_CLKGATE_IP3_I2S2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
/*Enable: IP4*/
#if 0
		.name		= "tzpc",
		.id		= 0,
		.parent		= &clk_p100,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_TZPC0,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzpc",
		.id		= 1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_TZPC1,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzpc",
		.id		= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_TZPC2,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "tzpc",
		.id		= 3,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_TZPC3,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
#endif
//		.name		= "seckey",
//		.id		= -1,
//		.parent		= &clk_p66,
//		.enable		= s5pc11x_clk_ip4_ctrl,
//		.ctrlbit	= S5P_CLKGATE_IP4_SECKEY,
//		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
//	}, {
#if 0
		.name		= "iem_apc",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_IEM_APC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "iem_iec",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_IEM_IEC,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
	}, {
		.name		= "chip_id",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip4_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP4_CHIP_ID,
		.powerDomain = S5PC110_POWER_DOMAIN_TOP,
#endif
	}
};

static struct clk *clks[] __initdata = {
	&clk_ext,
	&clk_epll,
	&clk_27m,
};

void __init s5pc11x_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));

	clkp = init_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {

		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}

		(clkp->enable)(clkp, 0);
	}
}
