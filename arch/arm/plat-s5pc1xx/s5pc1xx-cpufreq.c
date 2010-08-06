/*
 *  linux/arch/arm/plat-s5pc1xx/s5pc1xx-cpufreq.c
 *
 *  CPU frequency scaling for S5PC1XX
 *
 *  Copyright (C) 2008 Samsung Electronics
 *
 *  Based on cpu-sa1110.c, Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

//#include <mach/hardware.h>
#include <asm/system.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/cpu-freq.h>
#include <plat/pll.h>

#define USE_IEM

/* This CPU-FREQ driver supports 834Mhz as a Maximum operation speed
*  Bootloader must set 834Mhz to support current DVFS scenario like below table
*  If not, this driver dosen't work properly. 
*  Commented by jc.lee@samsung.com
*/

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

bool iem_enabled = FALSE;

/* frequency */
static struct cpufreq_frequency_table s5pc100_freq_table[] = {
	{L0, 834*1000},
	{L1, 666*1000},
	{L2, 333*1000},
	{L3, 166*1000},
	{L4, 83*1000},
	{0, CPUFREQ_TABLE_END},
};

enum iem_perf_level {
	IEM_L1,
	IEM_L2,
	IEM_L3,
	IEM_L4,
	IEM_L5,
	IEM_L6,
	IEM_L7,
	IEM_L8,		
};

static const u32 corevdd_value[8] = {
	0x10, 0x13, 0x17, 0x1f, 0x37, 0x3f, 0x77, 0x7f
};

static const u32 target_perf_level[8] = {
	0x08, 0x18, 0x28, 0x38, 0x48, 0x60, 0x70, 0x80
};

static const unsigned long pms_val_per_lvl[8][3] = {
	{445, 4, 0},	// IEM_L1
	{445, 4, 0},
	{400, 4, 1},
	{333, 3, 1},	// IEM_L4 : PLL 666 Mhz (333*12/3/1=666)
	{333, 3, 1},
	{333, 3, 1},
	{417, 3, 1},	// IEM_L7 : PLL 833 Mhz (417*12/3/2=834)
	{333, 3, 1}
};

static const unsigned long clk_div_per_lvl[8][4] = {
	/* APLL_DIV, HPM_DIV, ARM_DIV, HCLKD0_DIV */
	{2, 4, 1, 4},
	{2, 4, 1, 4},
	{2, 4, 1, 3},
	{1, 4, 1, 4},	// IEM_L4 : arm=666/1/1=666, d0=166	
	{1, 4, 5, 1},
	{1, 4, 5, 5},
	{1, 4, 1, 5},	// IEM_L7 : arm=833/1/1=833, d0=166
	{1, 4, 1, 4},	
};

void enable_open_loop(void)
{
	__raw_writel(0x1, S5P_APC_CONTROL);	// APC_VDD_UD - changeable voltage

}

void set_open_loop_default_value(void)
{
	__raw_writel(0x03, S5P_APC_VDDCHK);
	__raw_writel(0x01, S5P_APC_VDDCHKD);

	__raw_writel( corevdd_value[0], S5P_APC_PL1_COREVDD);
	__raw_writel( corevdd_value[1], S5P_APC_PL2_COREVDD);
	__raw_writel( corevdd_value[2], S5P_APC_PL3_COREVDD);
	__raw_writel( corevdd_value[3], S5P_APC_PL4_COREVDD);
	__raw_writel( corevdd_value[4], S5P_APC_PL5_COREVDD);
	__raw_writel( corevdd_value[5], S5P_APC_PL6_COREVDD);
	__raw_writel( corevdd_value[6], S5P_APC_PL7_COREVDD);
	__raw_writel( corevdd_value[7], S5P_APC_PL8_COREVDD);
	
}

void set_iem_ctrl(bool enable, enum iem_perf_level lvl)
{
	unsigned long target_lvl, iem_target_lvl, en_bit;

	target_lvl = lvl+1;
	
	en_bit = (enable) ? TRUE : FALSE;

	iem_target_lvl = (0xff>>(8-target_lvl));
	__raw_writel((iem_target_lvl<<16)|(en_bit<<0), S5P_IEM_CONTROL);
}

void set_dcg_idx_and_map(void)
{
	__raw_writel(0xD2489240, S5P_DCGIDX_MAP0);
	__raw_writel(0xDB6491B6, S5P_DCGIDX_MAP1);
	__raw_writel(0xFFFDB6B6, S5P_DCGIDX_MAP2);
	__raw_writel(0x3B2B1B0B, S5P_DCGPERF_MAP0);
	__raw_writel(0x8073634B, S5P_DCGPERF_MAP1);
	
}

// [Set Apll control IEM performance level]
// Fout (Hz) = (m * Fin) / (p * 2s)
// where  m = (MDIV + 8),  p = (PDIV + 2),  s = SDIV
// M[9:0], P[5:0], S[2:0] value for ARM PLL at IEM performance level-8
void set_apllcon_iem_pref_level(u32 p_lv, u32 m,
				u32 p, u32 s)
{
	__raw_writel( (m<<16) | (p<<8) | (s<<0), S5P_APLL_CON_L(p_lv));
}

void set_clk_div_iem_perf_level(u32 p_lv, u32 apll, u32 hpmclk,
				u32 armclk, u32 hclkd0)
{
	__raw_writel( ((apll-1)<<12)|((hpmclk-1)<<8)|((armclk-1)<<4)|(hclkd0-1), S5P_CLKDIV_IEM_L(p_lv));
	
}

void set_sample_iem_clk(void)
{
	int iter;

	for (iter=0; iter<8; iter++) {
		set_apllcon_iem_pref_level(iter+1, pms_val_per_lvl[iter][0], pms_val_per_lvl[iter][1], 
			pms_val_per_lvl[iter][2]);
		
		set_clk_div_iem_perf_level(iter+1, clk_div_per_lvl[iter][0], clk_div_per_lvl[iter][1], 
			clk_div_per_lvl[iter][2], clk_div_per_lvl[iter][3]);
	}

	__raw_writel(0xFAC688, S5P_DVCIDX_MAP);		// IECCFGDVCIDXMAP[23:0]
	__raw_writel(0xCB5E8, S5P_FREQ_CPU);		// 833000 (KHz)
	__raw_writel(0x28870, S5P_FREQ_DPM);		// 166000 (KHz)
}

void enable_iec(void)
{
	u32 regval;

	regval = __raw_readl(S5P_IECDPCCR);
	regval &= ~(1<<0);
	regval |= (1<<0); 	//IEC enable

	__raw_writel(regval, S5P_IECDPCCR);
}

void disable_iec(void)
{
	u32 regval;

	regval = __raw_readl(S5P_IECDPCCR);
	regval &= ~(1<<0); 	//IEC disable

	__raw_writel(regval, S5P_IECDPCCR);
}


void set_target_perf(enum iem_perf_level lvl)
{
	__raw_writel( target_perf_level[lvl], S5P_IECDPCTGTPERF);
}

void set_max_perf_map_idx(enum iem_perf_level lvl)
{
	unsigned long reg;
	
	if (lvl > 7) {
		printk(KERN_ERR "Index value is over than limits 0~7");
		return;
	}
	reg = __raw_readl(S5P_IECDPCCR);
	reg &= ~(7<<5);
	reg |= (lvl << 5);	// Max Performance Mapping Index value
	__raw_writel(reg, S5P_IECDPCCR);
	
}

void iem_enter(void)
{
	set_max_perf_map_idx(IEM_L8);
	enable_open_loop();
	
	set_iem_ctrl(TRUE, IEM_L8); // Enable IEM operation, previous Target perf level=8
	enable_iec();
	iem_enabled = TRUE;
	
}

void iem_exit(void)
{
	disable_iec();
	set_iem_ctrl(FALSE, IEM_L8); // Disable IEM operation, previous Target perf level=8
	iem_enabled = FALSE;
}

void iem_init(void)
{
	set_dcg_idx_and_map();
	set_sample_iem_clk();
}

/* TODO: Add support for SDRAM timing changes */

int s5pc100_verify_speed(struct cpufreq_policy *policy)
{
#ifndef USE_FREQ_TABLE
	struct clk *mpu_clk;
#endif

	if (policy->cpu)
		return -EINVAL;
#ifdef USE_FREQ_TABLE
	return cpufreq_frequency_table_verify(policy, s5pc100_freq_table);
#else
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
				     policy->cpuinfo.max_freq);
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	policy->min = clk_round_rate(mpu_clk, policy->min * KHZ_T) / KHZ_T;
	policy->max = clk_round_rate(mpu_clk, policy->max * KHZ_T) / KHZ_T;

	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
				     policy->cpuinfo.max_freq);

	clk_put(mpu_clk);

	return 0;
#endif
}

unsigned int s5pc100_getspeed(unsigned int cpu)
{
	struct clk * mpu_clk;
	unsigned long rate;

	if (cpu)
		return 0;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return 0;
	rate = clk_get_rate(mpu_clk) / KHZ_T;

	clk_put(mpu_clk);

	return rate;
}

static int s5pc100_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	struct clk * mpu_clk;
	struct clk * apll_clk;
	struct cpufreq_freqs freqs;
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index,reg;


	u32 apll, clk_div0;


	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);
	
	apll_clk = clk_get(NULL, "fout_apll");
	if (IS_ERR(apll_clk)) {
		clk_put(mpu_clk);
		return PTR_ERR(apll_clk);
	}
	
	freqs.old = s5pc100_getspeed(0);
#ifdef USE_FREQ_TABLE
	if (cpufreq_frequency_table_target(policy, s5pc100_freq_table, target_freq, relation, &index)) {
		ret = -EINVAL;
		goto out;
	}

	arm_clk = s5pc100_freq_table[index].frequency;

	freqs.new = arm_clk;
#else
	freqs.new = clk_round_rate(mpu_clk, target_freq * KHZ_T) / KHZ_T;
#endif
	freqs.cpu = 0;

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
#ifdef USE_DVS
	if(freqs.new < freqs.old){
		/* frequency scaling */
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0)
			printk("frequency scaling error\n");
		/* voltage scaling */
		set_power(index);
	}else{
		/* voltage scaling */
		set_power(index);

		/* frequency scaling */
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0)
			printk("frequency scaling error\n");
	}


#else
	if(freqs.new > freqs.old) {
		
		/* Frequency up */	
		set_power(index);
		
		switch(index) {

		case L0:
			if(!iem_enabled)	
				iem_enter();
			
			set_target_perf(IEM_L7);
			udelay(100);
			break;
		
		case L1:
		case L2:
			reg = __raw_readl(S5P_CLK_DIV0);
			reg &=~((0x7<<12)|(0x7<<8));
			reg |= ((clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));
			__raw_writel(reg, S5P_CLK_DIV0);
			
			reg &=~(0x7<<4);
			reg |= (clkdiv0_val[index][0]<<4);
			__raw_writel(reg, S5P_CLK_DIV0);
			break;

		case L3:
			reg = __raw_readl(S5P_CLK_DIV0);
			reg &=~((0x7<<12)|(0x7<<8));
			reg |= ((clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));
			__raw_writel(reg, S5P_CLK_DIV0);
			
			reg &=~(0x7<<4);
			reg |= (clkdiv0_val[index][0]<<4);
			__raw_writel(reg, S5P_CLK_DIV0);

			reg = __raw_readl(S5P_CLK_DIV1);
			reg &=~(0x7<<16);
			reg |= (0x1<<16);
			__raw_writel(reg, S5P_CLK_DIV1);

			reg = __raw_readl(S5P_CLK_DIV1);
			reg &=~(0x7<<12);
			reg |= (0x1<<12);
			__raw_writel(reg, S5P_CLK_DIV1);
			break;
		case L4:
		default:
			break;
		}
		
	} else if(freqs.new < freqs.old) {
		
		switch(index) {
		case L1:
		#if 0	
			if(!iem_enabled)	
				iem_enter();			
			set_target_perf(IEM_L4);
			udelay(100);
		#else
			iem_exit();
		#endif
			break;
			
		case L2:
		case L3:
			reg = __raw_readl(S5P_CLK_DIV0);
			reg &=~(0x7<<4);
			reg |= (clkdiv0_val[index][0]<<4);
			__raw_writel(reg, S5P_CLK_DIV0);
		
			reg &=~((0x7<<12)|(0x7<<8));
			reg |= ((clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));			
			__raw_writel(reg, S5P_CLK_DIV0);
			break;
			
		case L4:
			reg = __raw_readl(S5P_CLK_DIV0);
			reg &=~(0x7<<4);
			reg |= (clkdiv0_val[index][0]<<4);
			__raw_writel(reg, S5P_CLK_DIV0);
		
			reg &=~((0x7<<12)|(0x7<<8));
			reg |= ((clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));
			__raw_writel(reg, S5P_CLK_DIV0);

			reg = __raw_readl(S5P_CLK_DIV1);
			reg &=~(0x7<<12);
			reg |= (0x3<<12);
			__raw_writel(reg, S5P_CLK_DIV1);

			reg = __raw_readl(S5P_CLK_DIV1);
			reg &=~(0x7<<16);
			reg |= (0x0<<16);
			__raw_writel(reg, S5P_CLK_DIV1);

		case L0:
		default:
			break;

		}
		set_power(index);
		
	} else {
		if (index == L0) {
			/* IEM H/W enabled at 833Mhz speed */
			if(!iem_enabled)	
				iem_enter();
			set_target_perf(IEM_L7);
			udelay(100);
		} else {
		// Do nothing...
	}
	}

#endif

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	printk("Perf changed[L%d]\n",index);

out: 	
	clk_put(mpu_clk);
	clk_put(apll_clk);
	return ret;
}

static int __init s5pc100_cpu_init(struct cpufreq_policy *policy)
{
	struct clk * mpu_clk;
	u32 reg;

#ifdef USE_DVS
	ltc3714_init();
#endif

#ifdef CLK_OUT_PROBING
	
	reg = __raw_readl(S5P_CLK_OUT);
	reg &=~(0x1f << 12 | 0xf << 20);	// Mask Out CLKSEL bit field and DIVVAL
	reg |= (0x9 << 12 | 0x1 << 20);	// CLKSEL = ARMCLK/4, DIVVAL = 1 
	__raw_writel(reg, S5P_CLK_OUT);		// ARMCLK = CLK_OUT*4*(DIVVAL+1)
#endif
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	if (policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5pc100_getspeed(0);
#ifdef USE_FREQ_TABLE
	cpufreq_frequency_table_get_attr(s5pc100_freq_table, policy->cpu);
#else
	policy->cpuinfo.min_freq = clk_round_rate(mpu_clk, 0) / KHZ_T;
	policy->cpuinfo.max_freq = clk_round_rate(mpu_clk, VERY_HI_RATE) / KHZ_T;
#endif
	policy->cpuinfo.transition_latency = 40000;	//1us

	clk_put(mpu_clk);

#ifdef USE_IEM
	iem_init();
#endif


#ifdef USE_FREQ_TABLE
	return cpufreq_frequency_table_cpuinfo(policy, s5pc100_freq_table);
#else
	return 0;
#endif
}

static struct cpufreq_driver s5pc100_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5pc100_verify_speed,
	.target		= s5pc100_target,
	.get		= s5pc100_getspeed,
	.init		= s5pc100_cpu_init,
	.name		= "s5pc100",
};

static int __init s5pc100_cpufreq_init(void)
{
	return cpufreq_register_driver(&s5pc100_driver);
}

arch_initcall(s5pc100_cpufreq_init);
