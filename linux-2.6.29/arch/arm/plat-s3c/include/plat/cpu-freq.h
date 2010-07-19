/* arch/arm/plat-s3c/include/plat/cpu-freq.h
 *
 * Copyright (c) 2006,2007 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C CPU frequency scaling support - driver and board
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/cpufreq.h>

#if defined(CONFIG_CPU_S5PC100)

#define USE_FREQ_TABLE

//#define USE_DVS

#define KHZ_T		1000

#define MPU_CLK		"dout_arm"

/* definition for power setting function */
extern int set_power(unsigned int freq);
extern void ltc3714_init(void);

#define PMIC_ARM	0
#define PMIC_INT	1
#define PMIC_BOTH	2

#define CLK_OUT_PROBING	//TP80 on SMDKC100 board

typedef enum perf_level {
	L0,
	L1,
	L2,
	L3,
	L4,
};

static u32 clkdiv0_val[5][3] = {
	{0, 4, 1},	/* L0 : 832/166 */
	{0, 3, 1},	/* L1 : 666/166 */
	{1, 1, 1},	/* L2 : 333/166 */
	{3, 0, 1},	/* L3 : 166/166 */
	{7, 0, 0},	/* L4 " 83/83 */
	/*{ ARM_RATIO, D0_BUS_RATIO, PCLKD0_RATIO }*/
};
#endif	/* CONFIG_CPU_S5PC100 */


#if defined(CONFIG_CPU_S5PC110)

#define USE_FREQ_TABLE

//#define USE_DVS

#define KHZ_T		1000

#define MPU_CLK		"dout_apll"

#define CLK_OUT_PROBING	//TP80 on SMDKC100 board

enum perf_level {
	L0,
	L1,
	L2,
	L3,
	L4,
	L5,
	L6,
	L7,
};
#if 0 // this table not used
static u32 clkdiv0_val[4][3] = {
	{0, 3, 1},	/* L0 : 800/200/100 */
	{1, 1, 1},	/* L1 : 400/200/100 */
	{3, 0, 1},	/* L2 : 200/200/100 */
	{7, 0, 1},	/* L3 : 100/100/50 */
	//{7, 0, 0},	/* L4 " 83/83 */
	/*{ APLL_RATIO, HCLK_MSYS_RATIO, PCLK_MSYS_RATIO }*/
};
#endif

#define CLK_DIV0_MASK	((0x7<<0)|(0x7<<8)|(0x7<<12))	// APLL,HCLK_MSYS,PCLK_MSYS mask value 

#define INDX_ERROR  65535
#endif	/* CONFIG_CPU_S5PC100 */

struct s3c_cpufreq_info;
struct s3c_cpufreq_board;
struct s3c_iotimings;

struct s3c_freq {
	unsigned long	fclk;
	unsigned long	armclk;
	unsigned long	hclk_tns;	/* in 10ths of ns */
	unsigned long	hclk;
	unsigned long	pclk;
};

/* wrapper 'struct cpufreq_freqs' so that any drivers receiving the
 * notification can use this information that is not provided by just
 * having the core frequency alone.
 */

struct s3c_cpufreq_freqs {
	struct cpufreq_freqs	freqs;
	struct s3c_freq		old;
	struct s3c_freq		new;
};

#define to_s3c_cpufreq(_cf) container_of(_cf, struct s3c_cpufreq_freqs, freqs)

struct s3c_clkdivs {
	int		p_divisor;	/* fclk / pclk */
	int		h_divisor;	/* fclk / hclk */
	int		arm_divisor;	/* not all cpus have this. */
	unsigned char	dvs;		/* using dvs mode to arm. */
};

#define PLLVAL(_m, _p, _s) (((_m) << 12) | ((_p) << 4) | (_s))

struct s3c_pllval {
	unsigned long		freq;
	unsigned long		pll_reg;
};

struct s3c_cpufreq_config {
	struct s3c_freq		freq;
	struct s3c_pllval	pll;
	struct s3c_clkdivs	divs;
	struct s3c_cpufreq_info *info;	/* for core, not drivers */
	struct s3c_cpufreq_board *board;
};

/* s3c_cpufreq_board
 *
 * per-board configuraton information, such as memory refresh and
 * how to initialise IO timings.
 */
struct s3c_cpufreq_board {
	unsigned int	refresh;	/* refresh period in ns */
	unsigned int	auto_io:1;	/* automatically init io timings. */
	unsigned int	need_io:1;	/* set if needs io timing support. */

	/* any non-zero field in here is taken as an upper limit. */
	struct s3c_freq	max;	/* frequency limits */
};

/* Things depending on frequency scaling. */
#ifdef CONFIG_CPU_FREQ_S3C
#define __init_or_cpufreq
#else
#define __init_or_cpufreq __init
#endif

/* Board functions */

#ifdef CONFIG_CPU_FREQ_S3C
extern int s3c_cpufreq_setboard(struct s3c_cpufreq_board *board);
#else

static inline int s3c_cpufreq_setboard(struct s3c_cpufreq_board *board)
{
	return 0;
}
#endif  /* CONFIG_CPU_FREQ_S3C */
