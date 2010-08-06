/* linux/arch/arm/plat-s5pc1xx/s5pc100-clock.c
 *
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/sysdev.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/cpu-freq.h>

#include <plat/regs-clock.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>

/* For S5PC100 EVT0 workaround
 * When we modify DIVarm value to change ARM speed D0_BUS parent clock is also changed
 * If we prevent from unwanted changing of bus clock, we should modify DIVd0_bus value also.
 */
#define PREVENT_BUS_CLOCK_CHANGE

extern void ChangeClkDiv0(unsigned int val);

struct clk clk_ext_xtal_mux = {
	.name		= "ext_xtal",
	.id		= -1,
	.rate		= XTAL_FREQ,
};

#define clk_fin_apll	clk_ext_xtal_mux
#define clk_fin_mpll	clk_ext_xtal_mux
#define clk_fin_epll	clk_ext_xtal_mux
#define clk_fin_hpll	clk_ext_xtal_mux

#define clk_fout_mpll	clk_mpll

struct clk_sources {
	unsigned int	nr_sources;
	struct clk	**sources;
};

struct clksrc_clk {
	struct clk		clk;
	unsigned int		mask;
	unsigned int		shift;

	struct clk_sources	*sources;

	unsigned int		divider_shift;
	void __iomem		*reg_divider;
	void __iomem		*reg_source;
};

/* The peripheral clocks are all controlled via clocksource followed
 * by an optional divider and gate stage. We currently roll this into
 * one clock which hides the intermediate clock from the mux.
 *
 * Note, the JPEG clock can only be an even divider...
 *
 * The scaler and LCD clocks depend on the S3C64XX version, and also
 * have a common parent divisor so are not included here.
 */

static inline struct clksrc_clk *to_clksrc(struct clk *clk)
{
	return container_of(clk, struct clksrc_clk, clk);
}

static unsigned long s5pc1xx_getrate_clksrc(struct clk *clk)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	unsigned long rate = clk_get_rate(clk->parent);
	u32 clkdiv = __raw_readl(sclk->reg_divider);

	clkdiv >>= sclk->divider_shift;
	clkdiv &= 0xf;
	clkdiv++;

	rate /= clkdiv;
	return rate;
}

static int s5pc1xx_setrate_clksrc(struct clk *clk, unsigned long rate)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	void __iomem *reg = sclk->reg_divider;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(clk, rate);
	div = clk_get_rate(clk->parent) / rate;

	val = __raw_readl(reg);
	val &= ~sclk->mask;
	val |= (div - 1) << sclk->shift;
	__raw_writel(val, reg);

	return 0;
}

static int s5pc1xx_setparent_clksrc(struct clk *clk, struct clk *parent)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	struct clk_sources *srcs = sclk->sources;
	u32 clksrc = __raw_readl(sclk->reg_source);
	int src_nr = -1;
	int ptr;

	for (ptr = 0; ptr < srcs->nr_sources; ptr++)
		if (srcs->sources[ptr] == parent) {
			src_nr = ptr;
			break;
		}

	if (src_nr >= 0) {
		clksrc &= ~sclk->mask;
		clksrc |= src_nr << sclk->shift;

		__raw_writel(clksrc, sclk->reg_source);
		return 0;
	}

	return -EINVAL;
}

static unsigned long s5pc1xx_roundrate_clksrc(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate > parent_rate)
		rate = parent_rate;
	else {
		div = rate / parent_rate;

		if (div == 0)
			div = 1;
		if (div > 16)
			div = 16;

		rate = parent_rate / div;
	}

	return rate;
}

struct clk clk_srclk = {
	.name		= "srclk",
	.id		= -1,
};

static unsigned long s5pc1xx_clk_foutapll_get_rate(struct clk *clk)
{
  	unsigned long rate = clk_get_rate(clk->parent);

	return s5pc1xx_get_pll(rate, __raw_readl(S5P_APLL_CON));
}

int s5pc1xx_clk_foutapll_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *temp_clk = clk;
	unsigned long xtal = clk_get_rate(clk->parent);
	__raw_writel(rate, S5P_APLL_CON);
	
	temp_clk->rate = s5pc1xx_get_pll(xtal, __raw_readl(S5P_APLL_CON));
	
	return 0;
}


struct clk clk_fout_apll = {
	.name		= "fout_apll",
	.id		= -1,
	.parent		= &clk_ext_xtal_mux,
	.get_rate	= s5pc1xx_clk_foutapll_get_rate,
	.set_rate	= s5pc1xx_clk_foutapll_set_rate,
};

static struct clk *clk_src_apll_list[] = {
	[0] = &clk_fin_apll,
	[1] = &clk_fout_apll,
};

static struct clk_sources clk_src_apll = {
	.sources	= clk_src_apll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_apll_list),
};

struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,		
	},
	.shift		= S5P_CLKSRC0_APLL_SHIFT,
	.mask		= S5P_CLKSRC0_APLL_MASK,
	.sources	= &clk_src_apll,
	.reg_source	= S5P_CLK_SRC0,
};


static unsigned long s5pc1xx_clk_doutapll_get_rate(struct clk *clk)
{
  	unsigned long rate = clk_get_rate(clk->parent);

	rate /= (((__raw_readl(S5P_CLK_DIV0) & S5P_CLKDIV0_APLL_MASK) >> S5P_CLKDIV0_APLL_SHIFT) + 1);

	return rate;
}

int s5pc1xx_clk_doutapll_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *temp_clk = clk;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(temp_clk, rate);
	div = clk_get_rate(temp_clk->parent) / rate;

	val = __raw_readl(S5P_CLK_DIV0);
	val &=~ S5P_CLKDIV0_APLL_MASK;
	val |= (div - 1) << S5P_CLKDIV0_APLL_SHIFT;
	__raw_writel(val, S5P_CLK_DIV0);

	temp_clk->rate = rate;

	return 0;
}

struct clk clk_dout_apll = {
	.name = "dout_apll",
	.id = -1,
	.parent = &clk_mout_apll.clk,
	.get_rate = s5pc1xx_clk_doutapll_get_rate,
	.set_rate = s5pc1xx_clk_doutapll_set_rate,
};

static unsigned long s5pc1xx_clk_doutarm_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	rate /= (((__raw_readl(S5P_CLK_DIV0) & S5P_CLKDIV0_ARM_MASK) >> S5P_CLKDIV0_ARM_SHIFT) + 1);

	return rate;
}

static unsigned long s5pc1xx_doutarm_roundrate(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate > parent_rate)
		rate = parent_rate;
	else {
		div = parent_rate / rate;

		div ++;
		
		rate = parent_rate / div;
	}

	return rate;
}

int s5pc1xx_clk_doutarm_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *temp_clk = clk;
	unsigned int div_arm;
	unsigned int val;
#ifdef PREVENT_BUS_CLOCK_CHANGE
	unsigned int d0_bus_ratio, arm_ratio_old, ratio;
	val = __raw_readl(S5P_CLK_DIV0);
	d0_bus_ratio = (val & S5P_CLKDIV0_D0_MASK) >> S5P_CLKDIV0_D0_SHIFT;
	arm_ratio_old = (val & S5P_CLKDIV0_ARM_MASK) >> S5P_CLKDIV0_ARM_SHIFT;
	ratio = (arm_ratio_old + 1) * (d0_bus_ratio + 1);
#endif
	temp_clk->rate = rate;

	div_arm = clk_get_rate(temp_clk->parent) / rate;
	
#ifndef PREVENT_BUS_CLOCK_CHANGE
	val = __raw_readl(S5P_CLK_DIV0);
	val &=~ S5P_CLKDIV0_ARM_MASK;
	val |= (div_arm - 1) << S5P_CLKDIV0_ARM_SHIFT;
#else	
	d0_bus_ratio = (ratio / div_arm) -1;
	val &=~ (S5P_CLKDIV0_ARM_MASK | S5P_CLKDIV0_D0_MASK);
	val |= (div_arm - 1) << S5P_CLKDIV0_ARM_SHIFT;
	val |= d0_bus_ratio << S5P_CLKDIV0_D0_SHIFT;
	//printk(KERN_INFO "d0_bus_ratio : %08d ,arm_ratio: %08d\n",d0_bus_ratio, (div_arm-1));
	
#endif

#ifdef PREVENT_BUS_CLOCK_CHANGE

	/* Clock Down */
	if(arm_ratio_old < (div_arm - 1)) {
		val = __raw_readl(S5P_CLK_DIV0);
		val &=~ S5P_CLKDIV0_ARM_MASK;
		val |= (div_arm - 1) << S5P_CLKDIV0_ARM_SHIFT;
		__raw_writel(val, S5P_CLK_DIV0);

		val = __raw_readl(S5P_CLK_DIV0);
		val &=~ S5P_CLKDIV0_D0_MASK;
		val |= d0_bus_ratio << S5P_CLKDIV0_D0_SHIFT;
		__raw_writel(val, S5P_CLK_DIV0);
		
	} else {
		val = __raw_readl(S5P_CLK_DIV0);
		val &=~ S5P_CLKDIV0_D0_MASK;
		val |= d0_bus_ratio << S5P_CLKDIV0_D0_SHIFT;
		__raw_writel(val, S5P_CLK_DIV0);

		val = __raw_readl(S5P_CLK_DIV0);
		val &=~ S5P_CLKDIV0_ARM_MASK;
		val |= (div_arm - 1) << S5P_CLKDIV0_ARM_SHIFT;
		__raw_writel(val, S5P_CLK_DIV0);
	}

#else
	__raw_writel(val, S5P_CLK_DIV0);
#endif
	return 0;
}

struct clk clk_dout_arm = {
	.name = "dout_arm",
	.id = -1,
	.parent = &clk_dout_apll,
	.get_rate = s5pc1xx_clk_doutarm_get_rate,
	.set_rate = s5pc1xx_clk_doutarm_set_rate,
	.round_rate	= s5pc1xx_doutarm_roundrate,
};

static int fout_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	unsigned int epll_con = __raw_readl(S5P_EPLL_CON) & ~ ctrlbit;

	if(enable)
	   __raw_writel(epll_con | ctrlbit, S5P_EPLL_CON);
	else
	   __raw_writel(epll_con, S5P_EPLL_CON);

	return 0;
}

static unsigned long fout_get_rate(struct clk *clk)
{
	return clk->rate;
}

static int fout_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int epll_con;

	if(clk->rate == rate)	/* Return if nothing changed */
		return 0;

	epll_con = __raw_readl(S5P_EPLL_CON);

	epll_con &= ~S5P_EPLLVAL(0xff, 0x3f, 0x7); /* Clear M, P & S */

	switch (rate) {
	case 48000000:
			epll_con |= S5P_EPLLVAL(96, 3, 3);
			break;
	case 96000000:
			epll_con |= S5P_EPLLVAL(96, 3, 2);
			break;
	case 144000000:
			epll_con |= S5P_EPLLVAL(144, 3, 2);
			break;
	case 192000000:
			epll_con |= S5P_EPLLVAL(96, 3, 1);
			break;
	case 32750000:
	case 32768000:
			epll_con |= S5P_EPLLVAL(131, 3, 4);
			break;
	case 45000000:
	case 45158000:
			epll_con |= S5P_EPLLVAL(90, 3, 3);
			break;
	case 49125000:
	case 49152000:
			epll_con |= S5P_EPLLVAL(131, 4, 3);
			break;
	case 67737600:
	case 67738000:
			epll_con |= S5P_EPLLVAL(226, 5, 3);
			break;
	case 73800000:
	case 73728000:
			epll_con |= S5P_EPLLVAL(246, 5, 3);
			break;
	case 36000000:
			epll_con |= S5P_EPLLVAL(72, 3, 3); /* 36M = 0.75*48M */
			break;
	case 60000000:
			epll_con |= S5P_EPLLVAL(120, 3, 3); /* 60M = 1.25*48M */
			break;
	case 72000000:
			epll_con |= S5P_EPLLVAL(144, 3, 3); /* 72M = 1.5*48M */
			break;
	case 84000000:
			epll_con |= S5P_EPLLVAL(168, 3, 3); /* 84M = 1.75*48M */
			break;
	default:
			printk(KERN_ERR "Invalid Clock Freq!\n");
			return -EINVAL;
	}

	__raw_writel(epll_con, S5P_EPLL_CON);

	clk->rate = rate;

	return 0;
}

struct clk clk_fout_epll = {
	.name		= "fout_epll",
	.id		= -1,
	.ctrlbit	= S5P_EPLL_EN,
	.enable		= fout_enable,
	.get_rate	= fout_get_rate,
	.set_rate	= fout_set_rate,
};

static struct clk *clk_src_epll_list[] = {
	[0] = &clk_fin_epll,
	[1] = &clk_fout_epll,
};

static struct clk_sources clk_src_epll = {
	.sources	= clk_src_epll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_epll_list),
};

struct clksrc_clk clk_mout_epll = {
	.clk	= {
		.name		= "mout_epll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_EPLL_SHIFT,
	.mask		= S5P_CLKSRC0_EPLL_MASK,
	.sources	= &clk_src_epll,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_hpll_list[] = {
	[0] = &clk_27m,
	[1] = &clk_srclk,
};

static struct clk_sources clk_src_hpll = {
	.sources	= clk_src_hpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_hpll_list),
};

struct clksrc_clk clk_mout_hpll = {
	.clk	= {
		.name		= "mout_hpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_HPLL_SHIFT,
	.mask		= S5P_CLKSRC0_HPLL_MASK,
	.sources	= &clk_src_hpll,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_mpll_list[] = {
	[0] = &clk_fin_mpll,
	[1] = &clk_fout_mpll,
};

static struct clk_sources clk_src_mpll = {
	.sources	= clk_src_mpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_mpll_list),
};

struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name		= "mout_mpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MPLL_SHIFT,
	.mask		= S5P_CLKSRC0_MPLL_MASK,
	.sources	= &clk_src_mpll,
	.reg_source	= S5P_CLK_SRC0,
};

static unsigned long s5pc1xx_clk_doutmpll_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	/* printk(KERN_DEBUG "%s: parent is %ld\n", __func__, rate); */

	rate /= (((__raw_readl(S5P_CLK_DIV1) & S5P_CLKDIV1_MPLL_MASK) >> S5P_CLKDIV1_MPLL_SHIFT) + 1);

	return rate;
}

struct clk clk_dout_mpll = {
	.name		= "dout_mpll",
	.id		= -1,
	.parent		= &clk_mout_mpll.clk,
	.get_rate	= s5pc1xx_clk_doutmpll_get_rate,
};

static unsigned long s5pc1xx_clk_doutmpll2_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	/* printk(KERN_DEBUG "%s: parent is %ld\n", __func__, rate); */

	rate /= (((__raw_readl(S5P_CLK_DIV1) & S5P_CLKDIV1_MPLL2_MASK) >> S5P_CLKDIV1_MPLL2_SHIFT) + 1);

	return rate;
}

struct clk clk_dout_mpll2 = {
	.name		= "dout_mpll2",
	.id		= -1,
	.parent		= &clk_mout_mpll.clk,
	.get_rate	= s5pc1xx_clk_doutmpll2_get_rate,
};

static unsigned long s5pc1xx_clk_sclk_hdmi_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	/* printk(KERN_DEBUG "%s: parent is %ld\n", __func__, rate); */

	rate /= (((__raw_readl(S5P_CLK_DIV3) & S5P_CLKDIV3_HDMI_MASK) >> S5P_CLKDIV3_HDMI_SHIFT) + 1);

	return rate;
}

struct clk clk_sclk_hdmi = {
	.name		= "sclk_hdmi",
	.id		= -1,
	.parent		= &clk_mout_hpll.clk,
	.get_rate	= s5pc1xx_clk_sclk_hdmi_get_rate,
};

static struct clk *clkset_spi_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll2,
	&clk_fin_epll,
	&clk_mout_hpll.clk,
};

static struct clk_sources clkset_spi = {
	.sources	= clkset_spi_list,
	.nr_sources	= ARRAY_SIZE(clkset_spi_list),
};

static struct clk *clkset_uart_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	NULL,
	NULL,
};

static struct clk_sources clkset_uart = {
	.sources	= clkset_uart_list,
	.nr_sources	= ARRAY_SIZE(clkset_uart_list),
};

static struct clk *clkset_irda_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_mout_hpll.clk,
	&clk_48m,
};

static struct clk_sources clkset_irda = {
	.sources	= clkset_irda_list,
	.nr_sources	= ARRAY_SIZE(clkset_irda_list),
};

static struct clk *clkset_uhost_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_mout_hpll.clk,
	&clk_48m,
};

static struct clk_sources clkset_uhost = {
	.sources	= clkset_uhost_list,
	.nr_sources	= ARRAY_SIZE(clkset_uhost_list),
};

static struct clk *clkset_mmc0_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_fin_epll,
	NULL,
};

static struct clk_sources clkset_mmc0 = {
	.sources	= clkset_mmc0_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc0_list),
};

static struct clk *clkset_mmc1_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_fin_epll,
	&clk_mout_hpll.clk,
};

static struct clk_sources clkset_mmc1 = {
	.sources	= clkset_mmc1_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc1_list),
};

static struct clk *clkset_mmc2_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_fin_epll,
	&clk_mout_hpll.clk,
};

static struct clk_sources clkset_mmc2 = {
	.sources	= clkset_mmc2_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc2_list),
};

static struct clk *clkset_lcd_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_mout_hpll.clk,
	&clk_54m,
};

static struct clk_sources clkset_lcd = {
	.sources	= clkset_lcd_list,
	.nr_sources	= ARRAY_SIZE(clkset_lcd_list),
};

static struct clk *clkset_fimc_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_mout_hpll.clk,
	&clk_54m,
};

static struct clk_sources clkset_fimc = {
	.sources	= clkset_fimc_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc_list),
};

static struct clk *clkset_mixer_list[] = {
	&clk_27m,
	&clk_54m,
	&clk_sclk_hdmi,
	NULL,
};

static struct clk_sources clkset_mixer = {
	.sources	= clkset_mixer_list,
	.nr_sources	= ARRAY_SIZE(clkset_mixer_list),
};

static struct clk *clkset_pwi_list[] = {
	&clk_srclk,
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	NULL,
};

static struct clk_sources clkset_pwi = {
	.sources	= clkset_pwi_list,
	.nr_sources	= ARRAY_SIZE(clkset_pwi_list),
};

static struct clksrc_clk clk_mmc0 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_SCLK0_MMC0,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_MMC0_SHIFT,
	.mask		= S5P_CLKSRC2_MMC0_MASK,
	.sources	= &clkset_mmc0,
	.divider_shift	= S5P_CLKDIV3_MMC0_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clksrc_clk clk_mmc1 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_MMC1,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_MMC1_SHIFT,
	.mask		= S5P_CLKSRC2_MMC1_MASK,
	.sources	= &clkset_mmc1,
	.divider_shift	= S5P_CLKDIV3_MMC1_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clksrc_clk clk_mmc2 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_SCLK0_MMC2,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_MMC2_SHIFT,
	.mask		= S5P_CLKSRC2_MMC2_MASK,
	.sources	= &clkset_mmc2,
	.divider_shift	= S5P_CLKDIV3_MMC2_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clksrc_clk clk_usbhost = {
	.clk	= {
		.name		= "usb-host-bus",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_USBHOST,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_UHOST_SHIFT,
	.mask		= S5P_CLKSRC1_UHOST_MASK,
	.sources	= &clkset_uhost,
	.divider_shift	= S5P_CLKDIV2_UHOST_SHIFT,
	.reg_divider	= S5P_CLK_DIV2,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_uart_uclk1 = {
	.clk	= {
		.name		= "uclk1",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_UART,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_UART_SHIFT,
	.mask		= S5P_CLKSRC1_UART_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV2_UART_SHIFT,
	.reg_divider	= S5P_CLK_DIV2,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_spi0 = {
	.clk	= {
		.name		= "spi_epll",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_SCLK0_SPI0,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_SPI0_SHIFT,
	.mask		= S5P_CLKSRC1_SPI0_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV2_SPI0_SHIFT,
	.reg_divider	= S5P_CLK_DIV2,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_spi1 = {
	.clk	= {
		.name		= "spi_epll",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_SPI1,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_SPI1_SHIFT,
	.mask		= S5P_CLKSRC1_SPI1_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV2_SPI1_SHIFT,
	.reg_divider	= S5P_CLK_DIV2,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_spi2 = {
	.clk	= {
		.name		= "spi_epll",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_SCLK0_SPI2,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_SPI2_SHIFT,
	.mask		= S5P_CLKSRC1_SPI2_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV2_SPI2_SHIFT,
	.reg_divider	= S5P_CLK_DIV2,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_irda = {
	.clk	= {
		.name		= "sclk_irda",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_IRDA,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_IRDA_SHIFT,
	.mask		= S5P_CLKSRC1_IRDA_MASK,
	.sources	= &clkset_irda,
	.divider_shift	= S5P_CLKDIV2_IRDA_SHIFT,
	.reg_divider	= S5P_CLK_DIV2,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_pwi = {
	.clk	= {
		.name		= "sclk_pwi",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_PWI,
		.enable		= s5pc1xx_sclk0_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_PWI_SHIFT,
	.mask		= S5P_CLKSRC3_PWI_MASK,
	.sources	= &clkset_pwi,
	.divider_shift	= S5P_CLKDIV4_PWI_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk clk_lcd = {
	.clk	= {
		.name		= "sclk_lcd",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK1_LCD,
		.enable		= s5pc1xx_sclk1_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_LCD_SHIFT,
	.mask		= S5P_CLKSRC2_LCD_MASK,
	.sources	= &clkset_lcd,
	.divider_shift	= S5P_CLKDIV3_LCD_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clksrc_clk clk_fimc0 = {
	.clk	= {
		.name		= "sclk_fimc",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_SCLK1_FIMC0,
		.enable		= s5pc1xx_sclk1_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_FIMC0_SHIFT,
	.mask		= S5P_CLKSRC2_FIMC0_MASK,
	.sources	= &clkset_fimc,
	.divider_shift	= S5P_CLKDIV3_FIMC0_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clksrc_clk clk_fimc1 = {
	.clk	= {
		.name		= "sclk_fimc",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_SCLK1_FIMC1,
		.enable		= s5pc1xx_sclk1_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_FIMC1_SHIFT,
	.mask		= S5P_CLKSRC2_FIMC1_MASK,
	.sources	= &clkset_fimc,
	.divider_shift	= S5P_CLKDIV3_FIMC1_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clksrc_clk clk_fimc2 = {
	.clk	= {
		.name		= "sclk_fimc",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_SCLK1_FIMC2,
		.enable		= s5pc1xx_sclk1_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_FIMC2_SHIFT,
	.mask		= S5P_CLKSRC2_FIMC2_MASK,
	.sources	= &clkset_fimc,
	.divider_shift	= S5P_CLKDIV3_FIMC2_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clksrc_clk clk_mixer = {
	.clk	= {
		.name		= "sclk_mixer",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK1_MIXER,
		.enable		= s5pc1xx_sclk1_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_MIXER_SHIFT,
	.mask		= S5P_CLKSRC2_MIXER_MASK,
	.sources	= &clkset_mixer,
	.divider_shift	= S5P_CLKDIV3_HDMI_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC2,
};

struct clk clk_iis_cd0 = {
	.name		= "iis_cdclk0",
	.id		= -1,
};

struct clk clk_iis_cd1 = {
	.name		= "iiscd_cdclk1",
	.id		= -1,
};

struct clk clk_iis_cd2 = {
	.name		= "iiscd_cdclk2",
	.id		= -1,
};

struct clk clk_pcm_cd0 = {
	.name		= "pcmcd_cdclk0",
	.id		= -1,
};

struct clk clk_pcm_cd1 = {
	.name		= "pcmcd_cdclk1",
	.id		= -1,
};

static struct clk *clkset_audio0_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_dout_mpll,
	[2] = &clk_fin_epll,
	[3] = &clk_iis_cd0,
	[4] = &clk_pcm_cd0,
	[5] = &clk_mout_hpll.clk,
};

static struct clk_sources clkset_audio0 = {
	.sources	= clkset_audio0_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio0_list),
};

static struct clksrc_clk clk_audio0 = {
	.clk	= {
		.name		= "sclk_audio0",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK1_AUDIO0,
		.enable		= s5pc1xx_sclk1_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_AUDIO0_SHIFT,
	.mask		= S5P_CLKSRC3_AUDIO0_MASK,
	.sources	= &clkset_audio0,
	.divider_shift	= S5P_CLKDIV4_AUDIO0_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clk *clkset_audio1_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_dout_mpll,
	[2] = &clk_fin_epll,
	[3] = &clk_iis_cd1,
	[4] = &clk_pcm_cd1,
	[5] = &clk_mout_hpll.clk,
};

static struct clk_sources clkset_audio1 = {
	.sources	= clkset_audio1_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio1_list),
};

static struct clksrc_clk clk_audio1 = {
	.clk	= {
		.name		= "sclk_audio1",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK1_AUDIO1,
		.enable		= s5pc1xx_sclk1_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_AUDIO1_SHIFT,
	.mask		= S5P_CLKSRC3_AUDIO1_MASK,
	.sources	= &clkset_audio1,
	.divider_shift	= S5P_CLKDIV4_AUDIO1_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clk *clkset_audio2_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_dout_mpll,
	[2] = &clk_fin_epll,
	[3] = &clk_iis_cd2,
	[4] = &clk_mout_hpll.clk,
};

static struct clk_sources clkset_audio2 = {
	.sources	= clkset_audio2_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio2_list),
};

static struct clksrc_clk clk_audio2 = {
	.clk	= {
		.name		= "sclk_audio2",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK1_AUDIO2,
		.enable		= s5pc1xx_sclk1_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_AUDIO2_SHIFT,
	.mask		= S5P_CLKSRC3_AUDIO2_MASK,
	.sources	= &clkset_audio2,
	.divider_shift	= S5P_CLKDIV4_AUDIO2_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clk *clkset_i2sclkd2_list[] = {
	[0] = &clk_fout_epll,
	[1] = &clk_iis_cd0,
	[2] = &clk_audio0.clk,
};

static struct clk_sources clkset_i2sclkd2 = {
	.sources	= clkset_i2sclkd2_list,
	.nr_sources	= ARRAY_SIZE(clkset_i2sclkd2_list),
};

static struct clksrc_clk clk_i2sclkd2 = {
	.clk	= {
		.name		= "i2sclkd2",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_D20_I2SD2,
		.enable		= s5pc1xx_clk_d20_ctrl,
		.set_parent	= s5pc1xx_setparent_clksrc,
		.get_rate	= s5pc1xx_getrate_clksrc,
		.set_rate	= s5pc1xx_setrate_clksrc,
		.round_rate	= s5pc1xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_I2SD2_SHIFT,
	.mask		= S5P_CLKSRC3_I2SD2_MASK,
	.sources	= &clkset_i2sclkd2,
	.divider_shift	= S5P_CLKDIV4_I2SD2_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC3,
};

/* Clock initialisation code */

static struct clksrc_clk *init_parents[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
	&clk_mout_hpll,
	&clk_mmc0,
	&clk_mmc1,
	&clk_mmc2,
	&clk_usbhost,
	&clk_uart_uclk1,
	&clk_spi0,
	&clk_spi1,
	&clk_spi2,
	&clk_audio0,
	&clk_audio1,
	&clk_audio2,
	&clk_irda,
	&clk_pwi,
	&clk_lcd,
	&clk_fimc0,
	&clk_fimc1,
	&clk_fimc2,
	&clk_mixer,
};

static void __init_or_cpufreq s5pc1xx_set_clksrc(struct clksrc_clk *clk)
{
	struct clk_sources *srcs = clk->sources;
	u32 clksrc = __raw_readl(clk->reg_source);

	clksrc &= clk->mask;
	clksrc >>= clk->shift;

	if (clksrc > srcs->nr_sources || !srcs->sources[clksrc]) {
		printk(KERN_ERR "%s: bad source %d\n",
		       clk->clk.name, clksrc);
		return;
	}

	clk->clk.parent = srcs->sources[clksrc];

	printk(KERN_INFO "%s: source is %s (%d), rate is %ld\n",
	       clk->clk.name, clk->clk.parent->name, clksrc,
	       clk_get_rate(&clk->clk));
}

#define GET_DIV(clk, field) ((((clk) & field##_MASK) >> field##_SHIFT) + 1)

void __init_or_cpufreq s5pc100_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long armclk;
	unsigned long hclkd0;
	unsigned long hclk;
	unsigned long pclkd0;
	unsigned long pclk;
	unsigned long apll;
	unsigned long mpll;
	unsigned long hpll;
	unsigned long epll;
	unsigned int ptr;
	u32 clkdiv0, clkdiv1;

	printk(KERN_DEBUG "%s: registering clocks\n", __func__);

	clkdiv0 = __raw_readl(S5P_CLK_DIV0);
	clkdiv1 = __raw_readl(S5P_CLK_DIV1);

	printk(KERN_DEBUG "%s: clkdiv0 = %08x, clkdiv1 = %08x\n", __func__, clkdiv0, clkdiv1);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	printk(KERN_DEBUG "%s: xtal is %ld\n", __func__, xtal);

	apll = s5pc1xx_get_pll(xtal, __raw_readl(S5P_APLL_CON));
	mpll = s5pc1xx_get_pll(xtal, __raw_readl(S5P_MPLL_CON));
	epll = s5pc1xx_get_pll(xtal, __raw_readl(S5P_EPLL_CON));
	hpll = s5pc1xx_get_pll(xtal, __raw_readl(S5P_HPLL_CON));

	printk(KERN_INFO "S5PC100: PLL settings, A=%ld, M=%ld, E=%ld, H=%ld\n",
	       apll, mpll, epll, hpll);

	armclk = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_APLL);
	armclk = armclk / GET_DIV(clkdiv0, S5P_CLKDIV0_ARM);
	hclkd0 = armclk / GET_DIV(clkdiv0, S5P_CLKDIV0_D0);
	pclkd0 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLKD0);
	hclk = mpll / GET_DIV(clkdiv1, S5P_CLKDIV1_D1);
	pclk = hclk / GET_DIV(clkdiv1, S5P_CLKDIV1_PCLKD1);

	printk(KERN_INFO "S5PC100: ARMCLK=%ld, HCLKD0=%ld, PCLKD0=%ld, HCLK=%ld, PCLK=%ld\n",
	       armclk, hclkd0, pclkd0, hclk, pclk);

	clk_fout_apll.rate = apll;
	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;
	clk_mout_hpll.clk.rate = hpll;

	clk_f.rate = armclk;
	clk_hd0.rate = hclkd0;
	clk_pd0.rate = pclkd0;
	clk_h.rate = hclk;
	clk_p.rate = pclk;

	for (ptr = 0; ptr < ARRAY_SIZE(init_parents); ptr++)
		s5pc1xx_set_clksrc(init_parents[ptr]);
}

static struct clk *clks[] __initdata = {
	&clk_ext_xtal_mux,
	&clk_iis_cd0,
	&clk_iis_cd1,
	&clk_iis_cd2,
	&clk_pcm_cd0,
	&clk_pcm_cd1,
	&clk_mout_epll.clk,
	&clk_fout_epll,
	&clk_mout_mpll.clk,
	&clk_dout_mpll,
	&clk_dout_mpll2,
	&clk_mout_hpll.clk,
	&clk_sclk_hdmi,
	&clk_srclk,
	&clk_mmc0.clk,
	&clk_mmc1.clk,
	&clk_mmc2.clk,
	&clk_usbhost.clk,
	&clk_uart_uclk1.clk,
	&clk_spi0.clk,
	&clk_spi1.clk,
	&clk_spi2.clk,
	&clk_audio0.clk,
	&clk_audio1.clk,
	&clk_audio2.clk,
	&clk_irda.clk,
	&clk_pwi.clk,
	&clk_lcd.clk,
	&clk_fimc0.clk,
	&clk_fimc1.clk,
	&clk_fimc2.clk,
	&clk_mixer.clk,
	&clk_dout_apll,
	&clk_dout_arm,
	&clk_fout_apll,
};

void __init s5pc100_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	for (ptr = 0; ptr < ARRAY_SIZE(clks); ptr++) {
		clkp = clks[ptr];
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}
}
