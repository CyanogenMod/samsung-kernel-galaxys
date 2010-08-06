/*
 *  linux/arch/arm/plat-s3c64xx/s3c64xx-cpufreq.c
 *
 *  CPU frequency scaling for S3C64XX
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

/* frequency */
static struct cpufreq_frequency_table s5pc100_freq_table[] = {
	{L0, 834*1000},
	{L1, 666*1000},
	{L2, 333*1000},
	{L3, 166*1000},
	{L4, 83*1000},
	{0, CPUFREQ_TABLE_END},
};

static const u32 corevdd_value[8] = {
	0x10, 0x13, 0x17, 0x1f, 0x37, 0x3f, 0x77, 0x7f
};

static const u32 target_perf_level[8] = {
	0x08, 0x18, 0x28, 0x38, 0x48, 0x60, 0x70, 0x80
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

void set_iem_ctrl(u32 enable, enum perf_level p_lv)
{
	__raw_writel((0xff<<16)|(enable<<0), S5P_IEM_CONTROL);
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
	set_apllcon_iem_pref_level(1, 417, 3, 1);	// 417 * 12 / 3 / 2 = 833
	set_apllcon_iem_pref_level(2, 417, 3, 1);	// 417 * 12 / 3 / 2 = 833
	set_apllcon_iem_pref_level(3, 445, 4, 1);	// 445 * 12 / 4 / 2 = 667
	set_apllcon_iem_pref_level(4, 445, 4, 1);	// 445 * 12 / 4 / 2 = 667

	set_clk_div_iem_perf_level(1, 1, 4, 1, 5);	// arm=833/1/1=833, d0=166
	set_clk_div_iem_perf_level(2, 1, 4, 5, 1);	// arm=833/1/5=166, d0=166
	set_clk_div_iem_perf_level(3, 1, 4, 5, 4);	// arm=667/1/5=133, d0=33
	set_clk_div_iem_perf_level(4, 1, 4, 1, 4);	// arm=667/1/1=667, d0=166

	__raw_writel(0xFAC688, S5P_DVCIDX_MAP);		// IECCFGDVCIDXMAP[23:0]
	__raw_writel(0xCB5E8, S5P_FREQ_CPU);		// 833000 (KHz)
	__raw_writel(0x28870, S5P_FREQ_DPM);		// 166000 (KHz)
}

void enable_iec(void)
{
	u32 regval;

	regval = __raw_readl(S5P_IECDPCCR);
	regval |= (1<<0); 	//IEC enable

	__raw_writel(regval, S5P_IECDPCCR);
}

void set_target_perf(enum perf_level p_lv)
{
	__raw_writel( target_perf_level[p_lv], S5P_IECDPCTGTPERF);
}

void iem_init(void)
{
	set_dcg_idx_and_map();
	set_target_perf(1);
	set_sample_iem_clk();
	set_iem_ctrl(1, L0);
	enable_iec();
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
		
		set_power(index);
		
		if(index == L0) {	// Need to change APLL by using IEM controller
		
			reg = __raw_readl(S5P_CLK_DIV0);
			reg &=~((0x7<<12)|(0x7<<8));
			reg |= ((clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));
			__raw_writel(reg, S5P_CLK_DIV0);
			
			reg &=~(0x7<<4);
			reg |= (clkdiv0_val[index][0]<<4);
			__raw_writel(reg, S5P_CLK_DIV0);

			set_target_perf(0);
			
		} else {
			reg = __raw_readl(S5P_CLK_DIV0);
			reg &=~((0x7<<12)|(0x7<<8));
			reg |= ((clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));
			__raw_writel(reg, S5P_CLK_DIV0);
			
			reg &=~(0x7<<4);
			reg |= (clkdiv0_val[index][0]<<4);
			__raw_writel(reg, S5P_CLK_DIV0);

		}
	} else if(freqs.new < freqs.old) {
		if(index == L1) {	// Need to change APLL by using IEM controller
		
			set_target_perf(3);
			reg = __raw_readl(S5P_CLK_DIV0);
			reg &=~(0x7<<4);
			reg |= (clkdiv0_val[index][0]<<4);
			__raw_writel(reg, S5P_CLK_DIV0);
		
			reg &=~((0x7<<12)|(0x7<<8));
			reg |= ((clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));			
			__raw_writel(reg, S5P_CLK_DIV0);
			
			
		} else {
			reg = __raw_readl(S5P_CLK_DIV0);
			reg &=~(0x7<<4);
			reg |= (clkdiv0_val[index][0]<<4);
			__raw_writel(reg, S5P_CLK_DIV0);
		
			reg &=~((0x7<<12)|(0x7<<8));
			reg |= ((clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));
			__raw_writel(reg, S5P_CLK_DIV0);

		}

		set_power(index);
		
	} else {
		// Do nothing...
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
	__raw_writel(reg, S5P_CLK_OUT);
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
