/* linux/arch/arm/plat-s5pc11x/s5pc100-clock.c
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
#include <linux/delay.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/cpu-freq.h>

#include <plat/regs-clock.h>
#include <plat/regs-audss.h>
#include <plat/regs-power.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>


#define DBG(fmt...) 
//#define DBG printk



/* For S5PC100 EVT0 workaround
 * When we modify DIVarm value to change ARM speed D0_BUS parent clock is also changed
 * If we prevent from unwanted changing of bus clock, we should modify DIVd0_bus value also.
 */
#define PREVENT_BUS_CLOCK_CHANGE
static unsigned long s5pc11x_doutapll_roundrate(struct clk *clk,
                                              unsigned long rate);


#define CLK_DIV_CHANGE_BY_STEP 0


#define MAX_DVFS_LEVEL  7
extern unsigned int s5pc11x_cpufreq_index;


/*APLL_FOUT, MPLL_FOUT, ARMCLK, HCLK_DSYS*/
static const u32 s5p_sysout_clk_tab_1GHZ[][4] = {
	// APLL:1000,ARMCLK:1000,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	{1000* MHZ, 667 *MHZ, 1000 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:800,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	{800* MHZ, 667 *MHZ, 800 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:400,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	{800* MHZ, 667 *MHZ, 400 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:267,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
//266	{800* MHZ, 667 *MHZ, 267 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:200,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	{800* MHZ, 667 *MHZ, 200 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:100,HCLK_MSYS:100,MPLL:667,HCLK_DSYS:83,HCLK_PSYS:66,PCLK_MSYS:50,PCLK_DSYS:66,PCLK_PSYS:66
	{800* MHZ, 667 *MHZ, 100 *MHZ, 133 *MHZ},
};


#define DIV_TAB_MAX_FIELD	12

/*div0 ratio table*/
/*apll, a2m, HCLK_MSYS, PCLK_MSYS, HCLK_DSYS, PCLK_DSYS, HCLK_PSYS, PCLK_PSYS, MFC_DIV, G3D_DIV, MSYS source(2D, 3D, MFC)(0->apll,1->mpll), DMC0 div*/
static const u32 s5p_sys_clk_div0_tab_1GHZ[][DIV_TAB_MAX_FIELD] = {
        {0, 4, 4, 1, 3, 1, 4, 1, 3, 3, 0, 3},
        {0, 3, 3, 1, 3, 1, 4, 1, 3, 3, 0, 3},
        {1, 3, 1, 1, 3, 1, 4, 1, 3, 3, 0, 3},
        {3, 3, 0, 1, 3, 1, 4, 1, 3, 3, 0, 3},
        {7, 7, 0, 0, 7, 0, 9, 0, 3, 3, 1, 4},
};

/*pms value table*/
/*APLL(m, p, s), MPLL(m, p, s)*/
static const u32 s5p_sys_clk_mps_tab_1GHZ[][6] = {
        {250, 6, 1, 667, 12, 1},
        {200, 6, 1, 667, 12, 1},
        {200, 6, 1, 667, 12, 1},
        {200, 6, 1, 667, 12, 1},
        {200, 6, 1, 667, 12, 1},
};


/*APLL_FOUT, MPLL_FOUT, ARMCLK, HCLK_DSYS*/
static const u32 s5p_sysout_clk_tab_800MHZ[][4] = {
	// APLL:800,ARMCLK:800,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	{800* MHZ, 667 *MHZ, 800 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:400,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	{800* MHZ, 667 *MHZ, 400 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:267,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
//	{800* MHZ, 667 *MHZ, 267 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:200,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	{800* MHZ, 667 *MHZ, 200 *MHZ, 166 *MHZ},
	// APLL:800,ARMCLK:100,HCLK_MSYS:100,MPLL:667,HCLK_DSYS:83,HCLK_PSYS:66,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	{800* MHZ, 667 *MHZ, 100 *MHZ, 166 *MHZ},
};


/*div0 ratio table*/
/*apll, a2m, HCLK_MSYS, PCLK_MSYS, HCLK_DSYS, PCLK_DSYS, HCLK_PSYS, PCLK_PSYS, MFC_DIV, G3D_DIV,MSYS source, DMC0 div*/
static const u32 s5p_sys_clk_div0_tab_800MHZ[][DIV_TAB_MAX_FIELD] = {
        {0, 3, 3, 1, 3, 1, 4, 1, 3, 3, 0, 3},
        {1, 3, 1, 1, 3, 1, 4, 1, 3, 3, 0, 3},
        {3, 3, 0, 1, 3, 1, 4, 1, 3, 3, 0, 3},
        {7, 7, 0, 0, 7, 0, 9, 0, 3, 3, 0, 3},
};


/*pms value table*/
/*APLL(m, p, s), MPLL(m, p, s)*/
static const u32 s5p_sys_clk_mps_tab_800MHZ[][6] = {
        {200, 6, 1, 667, 12, 1},
        {200, 6, 1, 667, 12, 1},
        {200, 6, 1, 667, 12, 1},
        {200, 6, 1, 667, 12, 1},
};


static const u32 (*s5p_sysout_clk_tab_all[2])[4] = {
        s5p_sysout_clk_tab_1GHZ,
        s5p_sysout_clk_tab_800MHZ,
};

static const u32 (*s5p_sys_clk_div0_tab_all[2])[DIV_TAB_MAX_FIELD] = {
        s5p_sys_clk_div0_tab_1GHZ,
        s5p_sys_clk_div0_tab_800MHZ,
};

static const u32 (*s5p_sys_clk_mps_tab_all[2])[6] = {
        s5p_sys_clk_mps_tab_1GHZ,
        s5p_sys_clk_mps_tab_800MHZ,
};


extern void ChangeClkDiv0(unsigned int val);

/* fin_apll, fin_mpll and fin_epll are all the same clock, which we call
 * ext_xtal_mux for want of an actual name from the manual.
*/
static unsigned long s5pc11x_roundrate_clksrc(struct clk *clk, unsigned long rate);

struct clk clk_ext_xtal_mux = {
	.name		= "ext_xtal",
	.id		= -1,
};

#define clk_fin_apll	clk_ext_xtal_mux
#define clk_fin_mpll	clk_ext_xtal_mux
#define clk_fin_epll	clk_ext_xtal_mux
#define clk_fin_vpll	clk_ext_xtal_mux

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

struct clk clk_srclk = {
	.name		= "srclk",
	.id		= -1,
};

struct clk clk_fout_apll = {
	.name		= "fout_apll",
	.id		= -1,
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


static unsigned long s5pc11x_clk_doutapll_get_rate(struct clk *clk)
{
  	unsigned long rate = clk_get_rate(clk->parent);

	rate /= (((__raw_readl(S5P_CLK_DIV0) & S5P_CLKDIV0_APLL_MASK) >> S5P_CLKDIV0_APLL_SHIFT) + 1);

	return rate;
}

#ifdef CONFIG_CPU_FREQ
extern unsigned int dvfs_change_direction;
extern unsigned int prevIndex;
extern unsigned int S5PC11X_FREQ_TAB;


static u32 s5p_cpu_clk_tab_size(void)
{
	if(S5PC11X_FREQ_TAB == 0)
		return ARRAY_SIZE(s5p_sysout_clk_tab_1GHZ);
	else
		return ARRAY_SIZE(s5p_sys_clk_mps_tab_800MHZ);
}



int s5pc11x_clk_set_apll(unsigned int target_freq,
                                unsigned int index )
{
	u32 val, reg;
	unsigned int mask;	
	const u32 (*s5p_sys_clk_div0_tab)[DIV_TAB_MAX_FIELD];
	const u32 (*s5p_sys_clk_mps_tab)[6];

	s5p_sys_clk_div0_tab = s5p_sys_clk_div0_tab_all[S5PC11X_FREQ_TAB];
	s5p_sys_clk_mps_tab = s5p_sys_clk_mps_tab_all[S5PC11X_FREQ_TAB];

	/*change the apll*/
	
	//////////////////////////////////////////////////
	/* APLL should be changed in this level
	 * APLL -> MPLL(for stable transition) -> APLL
	 * Some clock source's clock API  are not prepared. Do not use clock API
	 * in below code.
	 */
	__raw_writel(0x40e, S5P_VA_DMC1 + 0x30);
	//__raw_writel(0x10233206, S5P_VA_DMC1 + 0x34);
	//__raw_writel(0x0E100222, S5P_VA_DMC1 + 0x3C);		
	
	/* Change APLL to MPLL in MFC_MUX and G3D MUX and G2D */
	reg = __raw_readl(S5P_CLK_DIV2);
	DBG("before apll transition DIV2=%x\n",reg);
	reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK  | S5P_CLKDIV2_G2D_MASK);
	reg |= (0x3<<S5P_CLKDIV2_G3D_SHIFT) |
		(0x3<<S5P_CLKDIV2_MFC_SHIFT) | (0x3<<S5P_CLKDIV2_G2D_SHIFT);
	__raw_writel(reg, S5P_CLK_DIV2);
	DBG("during apll transition DIV2=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT0);
	} while (reg & ((1<<16)|(1<<17)));
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT1);
	} while (reg & (1<<20));
		
	reg = __raw_readl(S5P_CLK_SRC2);
	DBG("before apll transition SRC2=%x\n",reg);
	reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK | S5P_CLKSRC2_G2D_MASK);
	reg |= (1<<S5P_CLKSRC2_G3D_SHIFT) | (1<<S5P_CLKSRC2_MFC_SHIFT) | (1<<S5P_CLKSRC2_G2D_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC2);
	DBG("during apll transition SRC2=%x\n",reg);	
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT1);
	} while (reg & ((1<<7)|(1<<3)|(1<<27)));
	
	/* SCLKAPLL -> SCLKMPLL */
	reg = __raw_readl(S5P_CLK_SRC0);
	DBG("before apll transition SRC0=%x\n",reg);	
	reg &= ~(S5P_CLKSRC0_MUX200_MASK);
	reg |= (0x1 << S5P_CLKSRC0_MUX200_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC0);
	DBG("durint apll transition SRC0=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT0);
	} while (reg & (0x1<<18));		
	//////////////////////////////////////////
	
	/* Set apll_out=fin */
	val = __raw_readl(S5P_CLK_SRC0);
	val = val & (~(0x1));
	__raw_writel(val, S5P_CLK_SRC0);
	
	/*stop apll*/
	val  = __raw_readl(S5P_APLL_CON);
        val = val & (~(0x1 << 31));
        __raw_writel(val, S5P_APLL_CON);

	/* Lock time = 300us*24Mhz = 7200(0x1c20) */
	__raw_writel(0x1c20, S5P_APLL_LOCK);
		
	/*set apll divider value*/	
	val = __raw_readl(S5P_CLK_DIV0);
	val = val & (~(0xffff));
	val = val | (s5p_sys_clk_div0_tab[index][0] << 0)| 
			(s5p_sys_clk_div0_tab[index][1] << 4)| 
			(s5p_sys_clk_div0_tab[index][2] << 8)
			| (s5p_sys_clk_div0_tab[index][3] << 12); 
	__raw_writel(val, S5P_CLK_DIV0);

	// check for div status
	mask = S5P_CLK_DIV_STAT0_DIV_APLL |S5P_CLK_DIV_STAT0_DIV_A2M |S5P_CLK_DIV_STAT0_DIV_HCLK_MSYS |S5P_CLK_DIV_STAT0_DIV_PCLK_MSYS;
	do {
		val = __raw_readl(S5P_CLK_DIV_STAT0) & mask;
	} while (val);
	DBG("\n DIV0 = %x\n",__raw_readl(S5P_CLK_DIV0));

	/*set apll_con*/
	val = (0 << 31) | (0 << 28) | (s5p_sys_clk_mps_tab[index][0] << 16) | 
			(s5p_sys_clk_mps_tab[index][1] << 8) | 
			(s5p_sys_clk_mps_tab[index][2] << 0);
	__raw_writel(val, S5P_APLL_CON);
	__raw_writel(val | (1 << 31), S5P_APLL_CON);
	while(!(__raw_readl(S5P_APLL_CON) & (1 << 29)));

	/* Set apll_out=fout */
	val = __raw_readl(S5P_CLK_SRC0);
	val = val | (0x1 << 0);
	__raw_writel(val, S5P_CLK_SRC0);
	
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT0);
	} while (reg & (1<<2));
			
	////////////////////////////////////
	/* Change MPLL to APLL in MFC_MUX and G3D MUX */
	reg = __raw_readl(S5P_CLK_SRC2);
	reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK | S5P_CLKSRC2_G2D_MASK);
	reg |= (0<<S5P_CLKSRC2_G3D_SHIFT) | (0<<S5P_CLKSRC2_MFC_SHIFT) | (0<<S5P_CLKSRC2_G2D_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC2);
	DBG("after apll transition SRC2=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT1);
	} while (reg & ((1<<7)|(1<<3)|(1<<27)));
			
	reg = __raw_readl(S5P_CLK_DIV2);
	reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK | S5P_CLKDIV2_G2D_MASK);
	reg |= (0x0<<S5P_CLKDIV2_G3D_SHIFT)|(0x0<<S5P_CLKDIV2_MFC_SHIFT)|(0x0<<S5P_CLKDIV2_G2D_SHIFT);
	__raw_writel(reg, S5P_CLK_DIV2);
	DBG("after apll transition DIV2=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT0);
	} while (reg & ((1<<16)|(1<<17)));
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT1);
	} while (reg & (1<<20));

	/* Change MPLL to APLL in MSYS_MUX and HPM_MUX */
	reg = __raw_readl(S5P_CLK_SRC0);
	reg &= ~(S5P_CLKSRC0_MUX200_MASK);
	reg |= (0x0 << S5P_CLKSRC0_MUX200_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC0);
	DBG("after apll transition SRC0=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT0);
	} while (reg & (0x1<<18));

	__raw_writel(0x618, S5P_VA_DMC1 + 0x30);
	//__raw_writel(0x12130005, S5P_VA_DMC1 + 0x34);
	//__raw_writel(0x0E190222, S5P_VA_DMC1 + 0x3C);		
////////////////////////////////////

	clk_fout_apll.rate = target_freq ;

	//udelay(30);

	DBG("S5P_APLL_CON = %x, S5P_CLK_DIV0=%x\n",__raw_readl(S5P_APLL_CON),__raw_readl(S5P_CLK_DIV0));

	return 0;
}



int s5pc11x_clk_set_rate(struct clk *clk, unsigned int target_freq,
                                unsigned int index )
{
	int cur_freq;
	unsigned int mask;
	u32 val, reg;
	u32 size;
	const u32 (*s5p_sysout_clk_tab)[4];
	const u32 (*s5p_sys_clk_div0_tab)[DIV_TAB_MAX_FIELD];

	s5p_sysout_clk_tab = s5p_sysout_clk_tab_all[S5PC11X_FREQ_TAB];
	s5p_sys_clk_div0_tab = s5p_sys_clk_div0_tab_all[S5PC11X_FREQ_TAB];
	size = s5p_cpu_clk_tab_size();

	if(index >= size)
	{
		printk("=DVFS ERR index(%d) > size(%d)\n", index, size);
		return 1;
	}


	/* validate target frequency */ 
	if(s5p_sysout_clk_tab[index][2] != target_freq)
	{
		DBG("=DVFS ERR target_freq (%d) != cpu_tab_freq (%d)\n", s5p_sysout_clk_tab[index][2], target_freq);
		return 0;
	}

	cur_freq = (int)s5pc11x_clk_doutapll_get_rate(clk);

	/*check if change in DMC0 divider*/
	if(s5p_sys_clk_div0_tab[prevIndex][11] != s5p_sys_clk_div0_tab[index][11])
	{
		val = __raw_readl(S5P_CLK_DIV6);
		val &= ~(S5P_CLKDIV6_ONEDRAM_MASK);
		val |= (s5p_sys_clk_div0_tab[index][11] << S5P_CLKDIV6_ONEDRAM_SHIFT);
		__raw_writel(val, S5P_CLK_DIV6);
		do {
			val = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (val & ((1<<15)));
	}



#if 0//not using 2D, 3D, MFC MPLL
	/*check if change in MSYS source for 2D, 3D, MFC*/
	if(s5p_sys_clk_div0_tab[prevIndex][10] != s5p_sys_clk_div0_tab[index][10])
	{
		if(s5p_sys_clk_div0_tab[index][10] == 1)
		{// Make the source as MPLL
			/*Changing the DIV2 register is not required as proper values are already there*/	
			reg = __raw_readl(S5P_CLK_SRC2);
			DBG("before apll transition SRC2=%x\n",reg);
			reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK | S5P_CLKSRC2_G2D_MASK);
			reg |= (1<<S5P_CLKSRC2_G3D_SHIFT) | (1<<S5P_CLKSRC2_MFC_SHIFT) | (1<<S5P_CLKSRC2_G2D_SHIFT);
			__raw_writel(reg, S5P_CLK_SRC2);
			DBG("during apll transition SRC2=%x\n",reg);	
			do {
				reg = __raw_readl(S5P_CLK_MUX_STAT1);
			} while (reg & ((1<<7)|(1<<3)|(1<<27)));
		}
		else
		{// Make the source as APLL
			reg = __raw_readl(S5P_CLK_SRC2);
			reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK | S5P_CLKSRC2_G2D_MASK);
			reg |= (0<<S5P_CLKSRC2_G3D_SHIFT) | (0<<S5P_CLKSRC2_MFC_SHIFT) | (0<<S5P_CLKSRC2_G2D_SHIFT);
			__raw_writel(reg, S5P_CLK_SRC2);
			DBG("after apll transition SRC2=%x\n",reg);
			do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
			} while (reg & ((1<<7)|(1<<3)|(1<<27)));
			
			/*Changing the DIV2 register is not required as proper values are already there*/	
		}
	}
//#else // use 100MHZ for 3D,2D, MFC
/*check if change in MSYS source for 2D, 3D, MFC*/
	if(s5p_sys_clk_div0_tab[prevIndex][10] != s5p_sys_clk_div0_tab[index][10])
	{
		/*Changing the DIV2 register */	
		reg = __raw_readl(S5P_CLK_DIV2);
		DBG("before apll transition DIV2=%x\n",reg);
		reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK  | S5P_CLKDIV2_G2D_MASK);
		reg |= (s5p_sys_clk_div0_tab[index][8]<<S5P_CLKDIV2_G3D_SHIFT) |
		(s5p_sys_clk_div0_tab[index][9]<<S5P_CLKDIV2_MFC_SHIFT) | 
		(s5p_sys_clk_div0_tab[index][9]<<S5P_CLKDIV2_G2D_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV2);
		DBG("during apll transition DIV2=%x\n",reg);
		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT0);
		} while (reg & ((1<<16)|(1<<17)));
		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (reg & (1<<20));
	}

	
#endif


	/* check if change in apll */
	if(s5p_sysout_clk_tab[prevIndex][0] != s5p_sysout_clk_tab[index][0])
	{
		DBG("changing apll\n");
		s5pc11x_clk_set_apll(target_freq, index);
		return 0;	
	}
	DBG("apll target frequency = %d, index=%d\n",s5p_sysout_clk_tab[index][2],index);

	/*return if current frequency is same as target frequency*/
	if(cur_freq == s5p_sysout_clk_tab[index][2])
		return 0;

	// change clock divider
	mask = (~(S5P_CLKDIV0_APLL_MASK)) & (~(S5P_CLKDIV0_A2M_MASK)) & 
			(~(S5P_CLKDIV0_HCLK200_MASK)) & (~(S5P_CLKDIV0_PCLK100_MASK));
	val = __raw_readl(S5P_CLK_DIV0) & mask;
	val |= (s5p_sys_clk_div0_tab[index][0]) << S5P_CLKDIV0_APLL_SHIFT;
	val |= (s5p_sys_clk_div0_tab[index][1]) << S5P_CLKDIV0_A2M_SHIFT;
	val |= (s5p_sys_clk_div0_tab[index][2]) << S5P_CLKDIV0_HCLK200_SHIFT;
	val |= (s5p_sys_clk_div0_tab[index][3]) << S5P_CLKDIV0_PCLK100_SHIFT;
	__raw_writel(val, S5P_CLK_DIV0);

//	udelay(30);
	mask = S5P_CLK_DIV_STAT0_DIV_APLL |S5P_CLK_DIV_STAT0_DIV_A2M |S5P_CLK_DIV_STAT0_DIV_HCLK_MSYS |S5P_CLK_DIV_STAT0_DIV_PCLK_MSYS;
	do {
		val = __raw_readl(S5P_CLK_DIV_STAT0) & mask;
	} while (val);
	DBG("\n DIV0 = %x\n",__raw_readl(S5P_CLK_DIV0));

	return 0;
}



/*Change MPll rate*/
int s5pc11x_clk_set_mpll(unsigned int index )
{
	int cur_freq;
	u32 val, mask;
	u32 size;
	u32 mpll_target_freq = 0;
	u32 fvco = 0, vsel = 0;
	struct clk *xtal_clk;
        u32 xtal;

	const u32 (*s5p_sysout_clk_tab)[4];
	const u32 (*s5p_sys_clk_div0_tab)[DIV_TAB_MAX_FIELD];
	const u32 (*s5p_sys_clk_mps_tab)[6];

        s5p_sysout_clk_tab = s5p_sysout_clk_tab_all[S5PC11X_FREQ_TAB];
	s5p_sys_clk_div0_tab = s5p_sys_clk_div0_tab_all[S5PC11X_FREQ_TAB];
	s5p_sys_clk_mps_tab = s5p_sys_clk_mps_tab_all[S5PC11X_FREQ_TAB];
	printk("mpll changed.\n");
	size = s5p_cpu_clk_tab_size();

	if(index >= size)
	{
		printk("=DVFS ERR index(%d) > size(%d)\n", index, size);
		return 1;
	}


	mpll_target_freq = s5p_sysout_clk_tab[index][1];
	
	cur_freq = clk_fout_mpll.rate;

	DBG("Current mpll frequency = %d\n",cur_freq);
	DBG("target mpll frequency = %d\n",mpll_target_freq);


	/* current frquency is same as target frequency */
	if(cur_freq == mpll_target_freq)
	{
		return 0;
	}

	/*change the mpll*/

	/*Change A2M divider*/
	//val = __raw_readl(S5P_CLK_DIV0);
	//val = val & (~S5P_CLKDIV0_A2M_MASK);		
	//val = val | (s5p_sys_clk_div0_tab[index][1] << S5P_CLKDIV0_A2M_SHIFT);

#if 0
	/*Change the mux for PSYS, DSYS domain to use apll*/
	val = __raw_readl(S5P_CLK_SRC0);
	val = val | (1 << S5P_CLKSRC0_MUX166_SHIFT) | (1 << S5P_CLKSRC0_MUX133_SHIFT);
	__raw_writel(val, S5P_CLK_SRC0);
#endif	
	/* Set mpll_out=fin */ //workaround for evt0
	val = __raw_readl(S5P_CLK_SRC0);
	val = val & (~S5P_CLKSRC0_MPLL_MASK);
	__raw_writel(val, S5P_CLK_SRC0);
	
	/*stop mpll*/
	val  = __raw_readl(S5P_MPLL_CON);
        val = val & (~(0x1 << 31));
        __raw_writel(val, S5P_MPLL_CON);

#if 1
	/*set mpll divider value*/	
	val = __raw_readl(S5P_CLK_DIV0);
	val = val & (~(0xffff<<16));
	val = val | (s5p_sys_clk_div0_tab[index][4] << 16)| 
			(s5p_sys_clk_div0_tab[index][5] << 20)| 
			(s5p_sys_clk_div0_tab[index][6] << 24)
			| (s5p_sys_clk_div0_tab[index][7] << 28); 
	__raw_writel(val, S5P_CLK_DIV0);
#endif

	/*set mpll_con*/

	xtal_clk = clk_get(NULL, "xtal");
        BUG_ON(IS_ERR(xtal_clk));

        xtal = clk_get_rate(xtal_clk);
        clk_put(xtal_clk);
	DBG("xtal = %d\n",xtal);


	fvco = ((xtal/s5p_sys_clk_mps_tab[index][4])*s5p_sys_clk_mps_tab[index][3])/1000000;
	DBG("fvco = %d\n",fvco);

	if(fvco <= 1400)
		vsel = 0;
	else
		vsel = 1;	

	val = (0 << 31) | (0 << 28) | (vsel << 27) | (s5p_sys_clk_mps_tab[index][3] << 16) | 
			(s5p_sys_clk_mps_tab[index][4] << 8) | 
			(s5p_sys_clk_mps_tab[index][5] << 0);
	__raw_writel(val, S5P_MPLL_CON);
	__raw_writel(val | (1 << 31), S5P_MPLL_CON);
	while(!(__raw_readl(S5P_MPLL_CON) & (1 << 29)));

	/* Set mpll_out=fout */ //workaround for evt0
	val = __raw_readl(S5P_CLK_SRC0);
	val = val | (0x1 << S5P_CLKSRC0_MPLL_SHIFT);
	__raw_writel(val, S5P_CLK_SRC0);


#if 0
	/*Change the mux for PSYS, DSYS domain to use mpll again*/
	val = __raw_readl(S5P_CLK_SRC0);
	val = val & ~((1 << S5P_CLKSRC0_MUX166_SHIFT) | (1 << S5P_CLKSRC0_MUX133_SHIFT));
	__raw_writel(val, S5P_CLK_SRC0);
#endif

	clk_fout_mpll.rate = mpll_target_freq ;

	//udelay(30); // some delay ??
	mask = S5P_CLK_DIV_STAT0_DIV_PCLK_PSYS |S5P_CLK_DIV_STAT0_DIV_HCLK_PSYS |S5P_CLK_DIV_STAT0_DIV_PCLK_DSYS |S5P_CLK_DIV_STAT0_DIV_HCLK_DSYS;
	do {
		val = __raw_readl(S5P_CLK_DIV_STAT0) & mask;
	} while (val);
	
	DBG("S5P_MPLL_CON = %x, S5P_CLK_DIV0=%x, S5P_CLK_SRC0=%x, \n",__raw_readl(S5P_MPLL_CON),__raw_readl(S5P_CLK_DIV0),__raw_readl(S5P_CLK_SRC0));

	return 0;
}



int s5pc11x_clk_dsys_psys_change(int index) {
	unsigned int div_hclk_dsys, div_hclk_psys, div_pclk_dsys, div_pclk_psys, val, mask;
	u32 size;
	
	const u32 (*s5p_sysout_clk_tab)[4];
	const u32 (*s5p_sys_clk_div0_tab)[DIV_TAB_MAX_FIELD];

        s5p_sysout_clk_tab = s5p_sysout_clk_tab_all[S5PC11X_FREQ_TAB];
	s5p_sys_clk_div0_tab = s5p_sys_clk_div0_tab_all[S5PC11X_FREQ_TAB];

	size = s5p_cpu_clk_tab_size();

	if(index >= size)
	{
		printk("index(%d) > size(%d)\n", index, size);
		return 1;
	}

	/* check if change in mpll */
        if(s5p_sysout_clk_tab[prevIndex][1] != s5p_sysout_clk_tab[index][1])
        {
                DBG("changing mpll\n");
                s5pc11x_clk_set_mpll(index);
                return 0;
        }


	val = __raw_readl(S5P_CLK_DIV0);

	div_hclk_dsys = (val & S5P_CLKDIV0_HCLK166_MASK) >> S5P_CLKDIV0_HCLK166_SHIFT;
	div_hclk_psys = (val & S5P_CLKDIV0_HCLK133_MASK) >> S5P_CLKDIV0_HCLK133_SHIFT;
	div_pclk_dsys = (val & S5P_CLKDIV0_PCLK83_MASK) >> S5P_CLKDIV0_PCLK83_SHIFT;
	div_pclk_psys = (val & S5P_CLKDIV0_PCLK66_MASK) >> S5P_CLKDIV0_PCLK66_SHIFT;
	if ((div_hclk_dsys == s5p_sys_clk_div0_tab[index][4]) && 
			(div_hclk_psys == s5p_sys_clk_div0_tab[index][6]) &&
			(div_pclk_dsys == s5p_sys_clk_div0_tab[index][5]) &&
			(div_pclk_psys == s5p_sys_clk_div0_tab[index][7]) ) {
			DBG("No change in psys, dsys domain\n");
		return 0;
	} else {
		mask = (~(S5P_CLKDIV0_HCLK166_MASK)) & (~(S5P_CLKDIV0_HCLK133_MASK)) & 
			(~(S5P_CLKDIV0_PCLK83_MASK)) & (~(S5P_CLKDIV0_PCLK66_MASK)) ;
		val = val & mask;
		val |= (s5p_sys_clk_div0_tab[index][4]) << S5P_CLKDIV0_HCLK166_SHIFT;
		val |= (s5p_sys_clk_div0_tab[index][6]) << S5P_CLKDIV0_HCLK133_SHIFT;
		val |= (s5p_sys_clk_div0_tab[index][5]) << S5P_CLKDIV0_PCLK83_SHIFT;
		val |= (s5p_sys_clk_div0_tab[index][7]) << S5P_CLKDIV0_PCLK66_SHIFT;
		__raw_writel(val, S5P_CLK_DIV0);


		DBG("DSYS/PSYS DIV0 = %x\n",__raw_readl(S5P_CLK_DIV0));
		//udelay(30);
		mask = S5P_CLK_DIV_STAT0_DIV_PCLK_PSYS |S5P_CLK_DIV_STAT0_DIV_HCLK_PSYS |S5P_CLK_DIV_STAT0_DIV_PCLK_DSYS |S5P_CLK_DIV_STAT0_DIV_HCLK_DSYS;
		do {
			val = __raw_readl(S5P_CLK_DIV_STAT0) & mask;
		} while (val);

		return 0;
	}	
}
#endif //CONFIG_CPU_FREQ


int s5pc11x_clk_doutapll_set_rate(struct clk *clk, unsigned long rate)
{

#ifdef CONFIG_CPU_FREQ
        int index, ret;
        index = s5pc11x_cpufreq_index;
        ret = s5pc11x_clk_set_rate(clk, rate, index);
        return 0;
#else

	struct clk *temp_clk = clk;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(temp_clk, rate);
	div = clk_get_rate(temp_clk->parent) / rate;

	val = __raw_readl(S5P_CLK_DIV0);
	val &=~ S5P_CLKDIV0_APLL_MASK;
	val |= (div - 1) << S5P_CLKDIV0_APLL_SHIFT;
	__raw_writel(val, S5P_CLK_DIV0);

	return 0;
#endif //CONFIG_CPU_FREQ
}

struct clk clk_dout_apll = {
	.name = "dout_apll",
	.id = -1,
	.parent = &clk_mout_apll.clk,
	.get_rate = s5pc11x_clk_doutapll_get_rate,
	.set_rate = s5pc11x_clk_doutapll_set_rate,
	.round_rate = s5pc11x_doutapll_roundrate,
};


static unsigned long s5pc11x_doutapll_roundrate(struct clk *clk,
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

static inline struct clksrc_clk *to_clksrc(struct clk *clk)
{
        return container_of(clk, struct clksrc_clk, clk);
}

static unsigned long s5pc11x_getrate_clksrc(struct clk *clk)
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

static int s5pc11x_setrate_clksrc(struct clk *clk, unsigned long rate)
{
        struct clksrc_clk *sclk = to_clksrc(clk);
        void __iomem *reg = sclk->reg_divider;
        unsigned int div;
        u32 val;

        rate = clk_round_rate(clk, rate);
        if (rate) {
                div = clk_get_rate(clk->parent) / rate;

                val = __raw_readl(reg);
                val &= ~sclk->mask;
                val |= (div - 1) << sclk->divider_shift;
                __raw_writel(val, reg);
        }

        return 0;
}

static int s5pc11x_setparent_clksrc(struct clk *clk, struct clk *parent)
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
                clk->parent = parent;

                clksrc &= ~sclk->mask;
                clksrc |= src_nr << sclk->shift;

                __raw_writel(clksrc, sclk->reg_source);
                return 0;
        }

        return -EINVAL;
}

static unsigned long s5pc11x_roundrate_clksrc(struct clk *clk,
                                              unsigned long rate)
{
        unsigned long parent_rate = clk_get_rate(clk->parent);
        int div;

        if (rate >= parent_rate)
                rate = parent_rate;
        else {
                div = parent_rate / rate;
                if(parent_rate % rate)
                        div++;

                if (div == 0)
                        div = 1;
                if (div > 16)
                        div = 16;

                rate = parent_rate / div;
        }

        return rate;
}

static int fout_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	unsigned int epll_con = __raw_readl(S5P_EPLL_CON) & ~ ctrlbit;

	if(enable)
	   __raw_writel(epll_con | ctrlbit, S5P_EPLL_CON);
	//else
	//   __raw_writel(epll_con, S5P_EPLL_CON);

	return 0;
}

static unsigned long fout_get_rate(struct clk *clk)
{
	return clk->rate;
}

static int fout_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int epll_con;

	epll_con = __raw_readl(S5P_EPLL_CON);

	epll_con &= ~S5P_EPLLVAL(0x1, 0x1ff, 0x3f, 0x7); /* Clear V, M, P & S */

	switch (rate) {
	case 48000000:
			epll_con |= S5P_EPLLVAL(0, 96, 6, 3);
			break;
	case 96000000:
			epll_con |= S5P_EPLLVAL(0, 96, 6, 2);
			break;
	case 144000000:
			epll_con |= S5P_EPLLVAL(1, 144, 6, 2);
			break;
	case 192000000:
			epll_con |= S5P_EPLLVAL(0, 96, 6, 1);
			break;
	case 288000000:
			epll_con |= S5P_EPLLVAL(1, 144, 6, 1);
			break;
	case 32750000:
	case 32768000:
			epll_con |= S5P_EPLLVAL(1, 131, 6, 4);
			break;
	case 45000000:
	case 45158000:
			epll_con |= S5P_EPLLVAL(0, 271, 18, 3);
			break;
	case 49125000:
	case 49152000:
			epll_con |= S5P_EPLLVAL(0, 131, 8, 3);
			break;
	case 67737600:
	case 67738000:
			epll_con |= S5P_EPLLVAL(1, 271, 12, 3);
			break;
	case 73800000:
	case 73728000:
			epll_con |= S5P_EPLLVAL(1, 295, 12, 3);
			break;
	case 36000000:
			epll_con |= S5P_EPLLVAL(0, 72, 6, 3); /* 36M = 0.75*48M */
			break;
	case 60000000:
			epll_con |= S5P_EPLLVAL(0, 120, 6, 3); /* 60M = 1.25*48M */
			break;
	case 72000000:
			epll_con |= S5P_EPLLVAL(0, 144, 6, 3); /* 72M = 1.5*48M */
			break;
	case 80000000:
			epll_con |= S5P_EPLLVAL(1, 160, 6, 3);
			break;
	case 84000000:
			epll_con |= S5P_EPLLVAL(0, 84, 6, 2);
			break;
	case 50000000:
			epll_con |= S5P_EPLLVAL(0, 100, 6, 3);
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
		.set_parent     = s5pc11x_setparent_clksrc,
	},
	.shift		= S5P_CLKSRC0_EPLL_SHIFT,
	.mask		= S5P_CLKSRC0_EPLL_MASK,
	.sources	= &clk_src_epll,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_vpll_list[] = {
	[0] = &clk_27m,
	[1] = &clk_srclk,
};

static struct clk_sources clk_src_vpll = {
	.sources	= clk_src_vpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_vpll_list),
};

struct clksrc_clk clk_mout_vpll = {
	.clk	= {
		.name		= "mout_vpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_VPLL_SHIFT,
	.mask		= S5P_CLKSRC0_VPLL_MASK,
	.sources	= &clk_src_vpll,
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

static unsigned long s5pc11x_clk_doutmpll_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	return rate;
}

struct clk clk_dout_ck166 = {
	.name		= "dout_ck166",
	.id		= -1,
	.parent		= &clk_mout_mpll.clk,
	.get_rate	= s5pc11x_clk_doutmpll_get_rate,
};

static unsigned long s5pc11x_clk_sclk_hdmi_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	return rate;
}

struct clk clk_sclk_hdmi = {
	.name		= "sclk_hdmi",
	.id		= -1,
	.parent		= &clk_mout_vpll.clk,
	.get_rate	= s5pc11x_clk_sclk_hdmi_get_rate,
};


static int s5pc11x_clk_out_set_rate(struct clk *clk, unsigned long rate)
{
	u32 val = 0, div = 0, rate_div = 1; 
	int err = -EINVAL;
	if(rate && clk->parent)
	{
		if(clk->parent == &clk_fout_apll)
			rate_div = 4;
		if(clk->parent == &clk_fout_mpll)
			rate_div = 2;
			
		div = clk_get_rate(clk->parent) / rate/ rate_div;
		val = __raw_readl(S5P_CLK_OUT);
		val &= (~(0xF << 20));
		val |= (div - 1) << 20;
		__raw_writel(val, S5P_CLK_OUT);
		err = 0;
	}
	return err;
}

static int s5pc11x_clk_out_set_parent(struct clk *clk, struct clk *parent)
{
	u32 val = 0; 
	int err = 0;
	clk->parent = parent;
	val = __raw_readl(S5P_CLK_OUT);

	if(parent == &clk_fout_apll)// rate is APLL/4
	{
		val = val & (~(0x1F << 12));
		val |= (0x0 << 12);
	}
	else if(parent == &clk_fout_mpll)// rate is MPLL/2
	{
		val = val & (~(0x1F << 12));
		val |= (0x1 << 12);
	}
	else if(parent == &clk_fout_epll)
	{
		val = val & (~(0x1F << 12));
		val |= (0x2 << 12);
	}
	else if(parent == &clk_mout_vpll.clk)
	{
		val = val & (~(0x1F << 12));
		val |= (0x3 << 12);
	}
	else
	{
		err = -EINVAL;
	}

	__raw_writel(val, S5P_CLK_OUT);
	return err;
}

struct clk xclk_out = {
	.name		= "clk_out",
	.id		= -1,
	.set_rate	= s5pc11x_clk_out_set_rate,
	.set_parent	= s5pc11x_clk_out_set_parent,
};


static struct clk *clkset_spi_list[] = {
	&clk_srclk,	/*XXTI*/
	&clk_srclk,	/*XusbXTI*/
	&clk_sclk_hdmi,
	&clk_srclk,
	&clk_srclk,
	&clk_srclk,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk
};

static struct clk_sources clkset_spi = {
	.sources	= clkset_spi_list,
	.nr_sources	= ARRAY_SIZE(clkset_spi_list),
};

static struct clk *clkset_uart_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	NULL,
	NULL,
};

static struct clk_sources clkset_uart = {
	.sources	= clkset_uart_list,
	.nr_sources	= ARRAY_SIZE(clkset_uart_list),
};

static struct clk *clkset_mmc0_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,	
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk *clkset_mmc1_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,	
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk *clkset_mmc2_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,	
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk *clkset_mmc3_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,	
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk_sources clkset_mmc0 = {
	.sources	= clkset_mmc0_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc0_list),
};

static struct clk_sources clkset_mmc1 = {
	.sources	= clkset_mmc1_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc1_list),
};

static struct clk_sources clkset_mmc2 = {
	.sources	= clkset_mmc2_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc2_list),
};

static struct clk_sources clkset_mmc3 = {
	.sources	= clkset_mmc3_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc3_list),
};

static struct clk *clkset_lcd_list[] = {
	NULL,
	&clk_srclk,	/*XusbXTI*/
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_lcd = {
	.sources	= clkset_lcd_list,
	.nr_sources	= ARRAY_SIZE(clkset_lcd_list),
};

static struct clk *clkset_cam0_list[] = {
	NULL,
	&clk_srclk,	/*XusbXTI*/
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_cam1_list[] = {
	NULL,
	&clk_srclk,	/*XusbXTI*/
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_cam0 = {
	.sources	= clkset_cam0_list,
	.nr_sources	= ARRAY_SIZE(clkset_cam0_list),
};

static struct clk_sources clkset_cam1 = {
	.sources	= clkset_cam1_list,
	.nr_sources	= ARRAY_SIZE(clkset_cam1_list),
};

static struct clk *clkset_fimc0_list[] = {
	NULL,
	&clk_srclk,	/*XusbXTI*/
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_fimc1_list[] = {
	NULL,
	&clk_srclk,	/*XusbXTI*/
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_fimc2_list[] = {
	NULL,
	&clk_srclk,	/*XusbXTI*/
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_fimc0 = {
	.sources	= clkset_fimc0_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc0_list),
};

static struct clk_sources clkset_fimc1 = {
	.sources	= clkset_fimc1_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc1_list),
};

static struct clk_sources clkset_fimc2 = {
	.sources	= clkset_fimc2_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc2_list),
};

static struct clk *clkset_csis_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_csis = {
	.sources	= clkset_csis_list,
	.nr_sources	= ARRAY_SIZE(clkset_csis_list),
};

static struct clk *clkset_pwi_list[] = {
	&clk_srclk,
	&clk_mout_epll.clk,
	&clk_mout_mpll.clk,
	NULL,
};

static struct clk_sources clkset_pwi = {
	.sources	= clkset_pwi_list,
	.nr_sources	= ARRAY_SIZE(clkset_pwi_list),
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
#if 0
static inline struct clksrc_clk *to_clksrc(struct clk *clk)
{
	return container_of(clk, struct clksrc_clk, clk);
}

static unsigned long s5pc11x_getrate_clksrc(struct clk *clk)
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

static int s5pc11x_setrate_clksrc(struct clk *clk, unsigned long rate)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	void __iomem *reg = sclk->reg_divider;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(clk, rate);
	if (rate) {
		div = clk_get_rate(clk->parent) / rate;

		val = __raw_readl(reg);
		val &= ~sclk->mask;
		val |= (div - 1) << sclk->divider_shift;
		__raw_writel(val, reg);
	}

	return 0;
}

static int s5pc11x_setparent_clksrc(struct clk *clk, struct clk *parent)
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
		clk->parent = parent;

		clksrc &= ~sclk->mask;
		clksrc |= src_nr << sclk->shift;

		__raw_writel(clksrc, sclk->reg_source);
		return 0;
	}

	return -EINVAL;
}

static unsigned long s5pc11x_roundrate_clksrc(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate >= parent_rate)
		rate = parent_rate;
	else {
		div = parent_rate / rate;
		if(parent_rate % rate)
			div++;

		if (div == 0)
			div = 1;
		if (div > 16)
			div = 16;

		rate = parent_rate / div;
	}

	return rate;
}
#endif

static struct clksrc_clk clk_mmc0 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 0,
		.ctrlbit 	= S5P_CLKGATE_IP2_HSMMC0,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_MMC0,
	},
	.shift		= S5P_CLKSRC4_MMC0_SHIFT,
	.mask		= S5P_CLKSRC4_MMC0_MASK,
	.sources	= &clkset_mmc0,
	.divider_shift	= S5P_CLKDIV4_MMC0_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc1 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 1,
		.ctrlbit    	= S5P_CLKGATE_IP2_HSMMC1,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_MMC1,
	},
	.shift		= S5P_CLKSRC4_MMC1_SHIFT,
	.mask		= S5P_CLKSRC4_MMC1_MASK,
	.sources	= &clkset_mmc1,
	.divider_shift	= S5P_CLKDIV4_MMC1_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc2 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 2,
		.ctrlbit    	= S5P_CLKGATE_IP2_HSMMC2,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_MMC2,
	},
	.shift		= S5P_CLKSRC4_MMC2_SHIFT,
	.mask		= S5P_CLKSRC4_MMC2_MASK,
	.sources	= &clkset_mmc2,
	.divider_shift	= S5P_CLKDIV4_MMC2_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc3 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 3,
		.ctrlbit    	= S5P_CLKGATE_IP2_HSMMC3,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_MMC3,
	},
	.shift		= S5P_CLKSRC4_MMC3_SHIFT,
	.mask		= S5P_CLKSRC4_MMC3_MASK,
	.sources	= &clkset_mmc3,
	.divider_shift	= S5P_CLKDIV4_MMC3_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_uart_uclk0 = {
	.clk	= {
		.name		= "uclk1",
		.id		= 0,
		.ctrlbit 	= S5P_CLKGATE_IP3_UART0,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_UART0,
	},
	.shift		= S5P_CLKSRC4_UART0_SHIFT,
	.mask		= S5P_CLKSRC4_UART0_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV4_UART0_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_uart_uclk1 = {
	.clk	= {
		.name		= "uclk1",
		.id		= 1,
		.ctrlbit   	= S5P_CLKGATE_IP3_UART1,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_UART1,
	},
	.shift		= S5P_CLKSRC4_UART1_SHIFT,
	.mask		= S5P_CLKSRC4_UART1_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV4_UART1_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_uart_uclk2 = {
	.clk	= {
		.name		= "uclk1",
		.id		= 2,
		.ctrlbit    	= S5P_CLKGATE_IP3_UART2,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_UART2,
	},
	.shift		= S5P_CLKSRC4_UART2_SHIFT,
	.mask		= S5P_CLKSRC4_UART2_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV4_UART2_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_uart_uclk3 = {
	.clk	= {
		.name		= "uclk1",
		.id		= 3,
		.ctrlbit    	= S5P_CLKGATE_IP3_UART3,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_UART3,
	},
	.shift		= S5P_CLKSRC4_UART3_SHIFT,
	.mask		= S5P_CLKSRC4_UART3_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV4_UART3_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_spi0 = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI0,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_SPI0,
	},
	.shift		= S5P_CLKSRC5_SPI0_SHIFT,
	.mask		= S5P_CLKSRC5_SPI0_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV5_SPI0_SHIFT,
	.reg_divider	= S5P_CLK_DIV5,
	.reg_source	= S5P_CLK_SRC5,
};

static struct clksrc_clk clk_spi1 = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI1,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_SPI1,
	},
	.shift		= S5P_CLKSRC5_SPI1_SHIFT,
	.mask		= S5P_CLKSRC5_SPI1_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV5_SPI1_SHIFT,
	.reg_divider	= S5P_CLK_DIV5,
	.reg_source	= S5P_CLK_SRC5,
};

static struct clksrc_clk clk_spi2 = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI2,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_SPI2,
	},
	.shift		= S5P_CLKSRC5_SPI2_SHIFT,
	.mask		= S5P_CLKSRC5_SPI2_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV5_SPI2_SHIFT,
	.reg_divider	= S5P_CLK_DIV5,
	.reg_source	= S5P_CLK_SRC5,
};

static struct clksrc_clk clk_lcd = {
	.clk	= {
		.name		= "sclk_lcd",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP1_FIMD,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_FIMD,
	},
	.shift		= S5P_CLKSRC1_FIMD_SHIFT,
	.mask		= S5P_CLKSRC1_FIMD_MASK,
	.sources	= &clkset_lcd,
	.divider_shift	= S5P_CLKDIV1_FIMD_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_cam0 = {
	.clk	= {
		.name		= "sclk_cam0",
		.id		= -1,
		.ctrlbit        = 0,
		.enable		= NULL,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_CAM0,
	},
	.shift		= S5P_CLKSRC1_CAM0_SHIFT,
	.mask		= S5P_CLKSRC1_CAM0_MASK,
	.sources	= &clkset_cam0,
	.divider_shift	= S5P_CLKDIV1_CAM0_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_cam1 = {
	.clk	= {
		.name		= "sclk_cam1",
		.id		= -1,
		.ctrlbit        = 0,
		.enable		= NULL,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_CAM1,
	},
	.shift		= S5P_CLKSRC1_CAM1_SHIFT,
	.mask		= S5P_CLKSRC1_CAM1_MASK,
	.sources	= &clkset_cam1,
	.divider_shift	= S5P_CLKDIV1_CAM1_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_fimc0 = {
	.clk	= {
		.name		= "sclk_fimc",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC0,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.powerDomain	= S5PC110_POWER_DOMAIN_CAM,
		.srcMaskReg     = S5P_CLK_SRC_MASK1,
		.srcMaskBit	= (S5P_CLKSRC_MASK1_FIMC0_LCLK|S5P_CLKSRC_MASK0_CAM0|S5P_CLKSRC_MASK0_CAM1),
	},
	.shift		= S5P_CLKSRC3_FIMC0_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC0_LCLK_MASK,
	.sources	= &clkset_fimc0,
	.divider_shift	= S5P_CLKDIV3_FIMC0_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk clk_fimc1 = {
	.clk	= {
		.name		= "sclk_fimc",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC1,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.powerDomain	= S5PC110_POWER_DOMAIN_CAM,
		.srcMaskReg     = S5P_CLK_SRC_MASK1,
		.srcMaskBit	= S5P_CLKSRC_MASK1_FIMC1_LCLK,
	},
	.shift		= S5P_CLKSRC3_FIMC1_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC1_LCLK_MASK,
	.sources	= &clkset_fimc1,
	.divider_shift	= S5P_CLKDIV3_FIMC1_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk clk_fimc2 = {
	.clk	= {
		.name		= "sclk_fimc",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC2,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.powerDomain	= S5PC110_POWER_DOMAIN_CAM,
		.srcMaskReg     = S5P_CLK_SRC_MASK1,
		.srcMaskBit	= S5P_CLKSRC_MASK1_FIMC2_LCLK,
	},
	.shift		= S5P_CLKSRC3_FIMC2_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC2_LCLK_MASK,
	.sources	= &clkset_fimc2,
	.divider_shift	= S5P_CLKDIV3_FIMC2_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk clk_csis = {
	.clk	= {
		.name		= "mipi-csis",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP0_CSIS,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.powerDomain	= S5PC110_POWER_DOMAIN_CAM,
	},
	.shift		= S5P_CLKSRC1_CSIS_SHIFT,
	.mask		= S5P_CLKSRC1_CSIS_MASK,
	.sources	= &clkset_csis,
	.divider_shift	= S5P_CLKDIV1_CSIS_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
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

static struct clk *clkset_mdnie_pwmclk_sel_list[] = {
	NULL,
	&clk_srclk,	/*XusbXTI*/
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_mdnie_pwmclk_sel = {
	.sources	= clkset_mdnie_pwmclk_sel_list,
	.nr_sources	= ARRAY_SIZE(clkset_mdnie_pwmclk_sel_list),
};

static struct clksrc_clk clk_mdnie_pwmclk_sel = {
	.clk	= {
		.name		= "mdnie_pwmclk_sel",
		.id		= -1,
		//.ctrlbit      ,
		//.enable	,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK1,
		.srcMaskBit	= S5P_CLKSRC_MASK1_MDNIE_PWM,
	},
	.shift		= S5P_CLKSRC3_MDINE_PWMCLK_SHIFT,
	.mask		= S5P_CLKSRC3_MDINE_PWMCLK_MASK,
	.sources	= &clkset_mdnie_pwmclk_sel,
	.divider_shift	= S5P_CLKDIV3_MDINE_PWM_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};


static struct clk *clkset_mdnie_sel_list[] = {
	NULL,
	&clk_srclk,	/*XusbXTI*/
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_mdnie_sel = {
	.sources	= clkset_mdnie_sel_list,
	.nr_sources	= ARRAY_SIZE(clkset_mdnie_sel_list),
};

static struct clksrc_clk clk_mdnie_sel = {
	.clk	= {
		.name		= "mdnie_sel",
		.id		= -1,
		.ctrlbit	= S5P_CLKGATE_IP1_MIE,
		.enable	= s5pc11x_clk_ip1_ctrl	,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK1,
		.srcMaskBit	= S5P_CLKSRC_MASK1_MDNIE,
		.powerDomain = S5PC110_POWER_DOMAIN_LCD,
	},
	.shift		= S5P_CLKSRC3_MDNIE_SHIFT,
	.mask		= S5P_CLKSRC3_MDNIE_MASK,
	.sources	= &clkset_mdnie_sel,
	.divider_shift	= S5P_CLKDIV3_MDINE_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clk *clkset_audio0_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_audio0 = {
	.sources	= clkset_audio0_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio0_list),
};

static struct clksrc_clk clk_audio0 = {
	.clk	= {
		.name		= "sclk_audio0",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP3_I2S0,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_AUDIO0,
	},
	.shift		= S5P_CLKSRC6_AUDIO0_SHIFT,
	.mask		= S5P_CLKSRC6_AUDIO0_MASK,
	.sources	= &clkset_audio0,
	.divider_shift	= S5P_CLKDIV6_AUDIO0_SHIFT,
	.reg_divider	= S5P_CLK_DIV6,
	.reg_source	= S5P_CLK_SRC6,
};

static struct clk *clkset_audio1_list[] = {
	&clk_iis_cd1,
	&clk_pcm_cd1,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_audio1 = {
	.sources	= clkset_audio1_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio1_list),
};

static struct clksrc_clk clk_audio1 = {
	.clk	= {
		.name		= "sclk_audio1",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP3_I2S1 | S5P_CLKGATE_IP3_PCM1,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_AUDIO1,
	},
	.shift		= S5P_CLKSRC6_AUDIO1_SHIFT,
	.mask		= S5P_CLKSRC6_AUDIO1_MASK,
	.sources	= &clkset_audio1,
	.divider_shift	= S5P_CLKDIV6_AUDIO1_SHIFT,
	.reg_divider	= S5P_CLK_DIV6,
	.reg_source	= S5P_CLK_SRC6,
};


static struct clk *clkset_audio2_list[] = {
	NULL,
	&clk_pcm_cd0,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_audio2 = {
	.sources	= clkset_audio2_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio2_list),
};

static struct clksrc_clk clk_audio2 = {
	.clk	= {
		.name		= "sclk_audio2",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP3_I2S2,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
		.srcMaskReg     = S5P_CLK_SRC_MASK0,
		.srcMaskBit	= S5P_CLKSRC_MASK0_AUDIO2,
	},
	.shift		= S5P_CLKSRC6_AUDIO2_SHIFT,
	.mask		= S5P_CLKSRC6_AUDIO2_MASK,
	.sources	= &clkset_audio2,
	.divider_shift	= S5P_CLKDIV6_AUDIO2_SHIFT,
	.reg_divider	= S5P_CLK_DIV6,
	.reg_source	= S5P_CLK_SRC6,
};

static struct clk *clkset_i2smain_list[] = {
	NULL, /* XXTI */
	&clk_fout_epll,
};

static struct clk_sources clkset_i2smain_clk = {
	.sources	= clkset_i2smain_list,
	.nr_sources	= ARRAY_SIZE(clkset_i2smain_list),
};

static struct clksrc_clk clk_i2smain = {
	.clk	= {
		.name		= "i2smain_clk", /* C110 calls it Main CLK */
		.id		= -1,
		//.powerDomain 	= S5PC110_POWER_DOMAIN_AUDIO,
		.set_parent	= s5pc11x_setparent_clksrc,
	},
	.shift		= S5P_AUDSS_CLKSRC_MAIN_SHIFT,
	.mask		= S5P_AUDSS_CLKSRC_MAIN_MASK,
	.sources	= &clkset_i2smain_clk,
	.reg_source	= S5P_CLKSRC_AUDSS,
};

static struct clk *clkset_audss_hclk_list[] = {
	&clk_i2smain.clk,
	&clk_iis_cd0,
};

static struct clk_sources clkset_audss_hclk = {
	.sources	= clkset_audss_hclk_list,
	.nr_sources	= ARRAY_SIZE(clkset_audss_hclk_list),
};

static struct clksrc_clk clk_audss_hclk = {
	.clk	= {
		.name		= "audss_hclk", /* C110 calls it BUSCLK */
		.id		= -1,
		//.powerDomain    = S5PC110_POWER_DOMAIN_AUDIO,
		.ctrlbit        = S5P_AUDSS_CLKGATE_HCLKI2S,
		.enable		= s5pc11x_audss_clkctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_AUDSS_CLKSRC_BUSCLK_SHIFT,
	.mask		= S5P_AUDSS_CLKSRC_BUSCLK_MASK,
	.sources	= &clkset_audss_hclk,
	.divider_shift	= S5P_AUDSS_CLKDIV_BUSCLK_SHIFT,
	.reg_divider	= S5P_CLKDIV_AUDSS,
	.reg_source	= S5P_CLKSRC_AUDSS,
};

static struct clk *clkset_i2sclk_list[] = {
	&clk_i2smain.clk,
	&clk_iis_cd0,
	&clk_audio0.clk,
};

static struct clk_sources clkset_i2sclk = {
	.sources	= clkset_i2sclk_list,
	.nr_sources	= ARRAY_SIZE(clkset_i2sclk_list),
};

static struct clksrc_clk clk_i2sclk = {
	.clk	= {
		.name		= "i2sclk",
		.id		= -1,
		//.powerDomain    = S5PC110_POWER_DOMAIN_AUDIO,
		.ctrlbit        = S5P_AUDSS_CLKGATE_CLKI2S,
		.enable		= s5pc11x_audss_clkctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_AUDSS_CLKSRC_I2SCLK_SHIFT,
	.mask		= S5P_AUDSS_CLKSRC_I2SCLK_MASK,
	.sources	= &clkset_i2sclk,
	.divider_shift	= S5P_AUDSS_CLKDIV_I2SCLK_SHIFT,
	.reg_divider	= S5P_CLKDIV_AUDSS,
	.reg_source	= S5P_CLKSRC_AUDSS,
};


/* Clock initialisation code */

static struct clksrc_clk *init_parents[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
	&clk_mout_vpll,
	&clk_mmc0,
	&clk_mmc1,
	&clk_mmc2,
	&clk_mmc3,	
	&clk_uart_uclk0,
	&clk_uart_uclk1,
	&clk_uart_uclk2,
	&clk_uart_uclk3,
	&clk_spi0,
	&clk_spi1,
	&clk_spi2,
	&clk_audio0,
	&clk_audio1,
	&clk_audio2,
	&clk_i2sclk,
	&clk_audss_hclk,
	&clk_i2smain,
	&clk_lcd,
	&clk_cam0,
	&clk_cam1,
	&clk_fimc0,
	&clk_fimc1,
	&clk_fimc2,
	&clk_mdnie_pwmclk_sel,
	&clk_mdnie_sel,
	&clk_csis,
};

static void __init_or_cpufreq s5pc11x_set_clksrc(struct clksrc_clk *clk)
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

#ifdef CONFIG_CPU_FREQ_LOG
void print_clocks(void) 
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long armclk;
	unsigned long hclk200;
	unsigned long hclk166;
	unsigned long hclk133;
	unsigned long pclk100;
	unsigned long pclk83;
	unsigned long pclk66;
	unsigned long apll;
	unsigned long mpll;
	unsigned long vpll;
	unsigned long epll;
	//unsigned int ptr;
	u32 clkdiv0, clkdiv1;

	clkdiv0 = __raw_readl(S5P_CLK_DIV0);
	clkdiv1 = __raw_readl(S5P_CLK_DIV1);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	apll = s5pc11x_get_pll(xtal, __raw_readl(S5P_APLL_CON), S5PC11X_PLL_APLL);
	mpll = s5pc11x_get_pll(xtal, __raw_readl(S5P_MPLL_CON), S5PC11X_PLL_MPLL);
	epll = s5pc11x_get_pll(xtal, __raw_readl(S5P_EPLL_CON), S5PC11X_PLL_EPLL);
	vpll = s5pc11x_get_pll(xtal, __raw_readl(S5P_VPLL_CON), S5PC11X_PLL_VPLL);

	armclk = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_APLL);
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX200_SHIFT)) {
		hclk200 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK200);
	} else {
		hclk200 = armclk / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK200);
	}
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX166_SHIFT)) {
		hclk166 = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
		hclk166 = hclk166 / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
	} else {
		hclk166 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
	}
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX133_SHIFT)) {
		hclk133 = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
		hclk133 = hclk133 / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
	} else {
		hclk133 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
	}

	pclk100 = hclk200 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK100);
	pclk83 = hclk166 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK83);
	pclk66 = hclk133 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK66);

	printk("S5PC110: ARMCLK=%ld, HCLKM=%ld, HCLKD=%ld, HCLKP=%ld, PCLKM=%ld, PCLKD=%ld, PCLKP=%ld\n",
	       armclk, hclk200, hclk166, hclk133, pclk100, pclk83, pclk66);

}
#endif

void __init_or_cpufreq s5pc110_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long armclk;
	unsigned long hclk200;
	unsigned long hclk166;
	unsigned long hclk133;
	unsigned long pclk100;
	unsigned long pclk83;
	unsigned long pclk66;
	unsigned long apll;
	unsigned long mpll;
	unsigned long vpll;
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

	apll = s5pc11x_get_pll(xtal, __raw_readl(S5P_APLL_CON), S5PC11X_PLL_APLL);
	mpll = s5pc11x_get_pll(xtal, __raw_readl(S5P_MPLL_CON), S5PC11X_PLL_MPLL);
	epll = s5pc11x_get_pll(xtal, __raw_readl(S5P_EPLL_CON), S5PC11X_PLL_EPLL);
	vpll = s5pc11x_get_pll(xtal, __raw_readl(S5P_VPLL_CON), S5PC11X_PLL_VPLL);

	printk(KERN_INFO "S5PC100: PLL settings, A=%ld, M=%ld, E=%ld, H=%ld\n",
	       apll, mpll, epll, vpll);

	armclk = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_APLL);
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX200_SHIFT)) {
		hclk200 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK200);
	} else {
		hclk200 = armclk / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK200);
	}
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX166_SHIFT)) {
		hclk166 = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
		hclk166 = hclk166 / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
	} else {
		hclk166 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
	}
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX133_SHIFT)) {
		hclk133 = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
		hclk133 = hclk133 / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
	} else {
		hclk133 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
	}
		
	pclk100 = hclk200 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK100);
	pclk83 = hclk166 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK83);
	pclk66 = hclk133 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK66);


	printk(KERN_INFO "S5PC110: ARMCLK=%ld, HCLKM=%ld, HCLKD=%ld, HCLKP=%ld, PCLKM=%ld, PCLKD=%ld, PCLKP=%ld\n",
	       armclk, hclk200, hclk166, hclk133, pclk100, pclk83, pclk66);
	
	clk_fout_apll.rate = apll;
	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;
	clk_mout_vpll.clk.rate = vpll;

	clk_f.rate = armclk;
	clk_h200.rate = hclk200;
	clk_p100.rate = pclk100;
	clk_h166.rate = hclk166;
	clk_p83.rate = pclk83;
	clk_h133.rate = hclk133;
	clk_p66.rate = pclk66;
	clk_h.rate = hclk133;
	clk_p.rate = pclk66;

	clk_srclk.rate = xtal; //24MHz

	clk_set_parent(&clk_mmc0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc2.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc3.clk, &clk_mout_mpll.clk);

	clk_set_parent(&clk_spi0.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_spi1.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_spi2.clk, &clk_mout_epll.clk);
	
	clk_set_parent(&clk_uart_uclk0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_uart_uclk1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_uart_uclk2.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_uart_uclk3.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_lcd.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mdnie_sel.clk, &clk_mout_mpll.clk);


	for (ptr = 0; ptr < ARRAY_SIZE(init_parents); ptr++)
		s5pc11x_set_clksrc(init_parents[ptr]);

	clk_set_rate(&clk_mmc0.clk, 52*MHZ);	
	clk_set_rate(&clk_mmc1.clk, 50*MHZ);
	clk_set_rate(&clk_mmc2.clk, 50*MHZ);
	clk_set_rate(&clk_mmc3.clk, 50*MHZ);
	clk_set_rate(&clk_lcd.clk, 167*MHZ);
	clk_set_rate(&clk_mdnie_sel.clk, 167*MHZ);

	//printk(KERN_INFO "*\nS5PC110: SRC0=%x, SRC1=%x, SRC2=%x, SRC3=%x, \nSRC4=%x, SRC5=%x, SRC6=%x\n",__raw_readl(S5P_CLK_SRC0),__raw_readl(S5P_CLK_SRC1),__raw_readl(S5P_CLK_SRC2),__raw_readl(S5P_CLK_SRC3),__raw_readl(S5P_CLK_SRC4),__raw_readl(S5P_CLK_SRC5),__raw_readl(S5P_CLK_SRC6));	
	//printk(KERN_INFO "*\nS5PC110: DIV0=%x, DIV1=%x, DIV2=%x, DIV3=%x\n",__raw_readl(S5P_CLK_DIV0),__raw_readl(S5P_CLK_DIV1),__raw_readl(S5P_CLK_DIV2),__raw_readl(S5P_CLK_DIV3));		
}

static struct clk *clks[] __initdata = {
	&clk_ext_xtal_mux,
	&clk_mout_epll.clk,
	&clk_fout_epll,
	&clk_mout_mpll.clk,
	&clk_dout_ck166,
	&clk_mout_vpll.clk,
	&clk_sclk_hdmi,
	&clk_srclk,
	&clk_mmc0.clk,
	&clk_mmc1.clk,
	&clk_mmc2.clk,
	&clk_mmc3.clk,	
	&clk_uart_uclk0.clk,
	&clk_uart_uclk1.clk,
	&clk_uart_uclk2.clk,
	&clk_uart_uclk3.clk,
	&clk_spi0.clk,
	&clk_spi1.clk,
	&clk_spi2.clk,
	&clk_iis_cd0,
	&clk_iis_cd1,
	&clk_audio0.clk,
	&clk_audio1.clk,
	&clk_audio2.clk,
	&clk_i2sclk.clk,
	&clk_audss_hclk.clk,
	&clk_i2smain.clk,
	&clk_lcd.clk,
	&clk_dout_apll,
	&clk_cam0.clk,
	&clk_cam1.clk,
	&clk_fimc0.clk,
	&clk_fimc1.clk,
	&clk_fimc2.clk,
	&clk_mdnie_pwmclk_sel.clk,
	&clk_mdnie_sel.clk,	
	&clk_csis.clk,
	&xclk_out,
};

void __init s5pc110_register_clocks(void)
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
