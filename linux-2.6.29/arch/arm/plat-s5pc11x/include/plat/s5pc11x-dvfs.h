/* /arch/arm/plat-s5pc11x/include/plat/s5pc11x-dvfs.h
 *
 * Copyright (c) 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __PLAT_S5PC11X_DVFS_H
#define __PLAT_S5PC11X_DVFS_H
#include <plat/cpu-freq.h>

//extern void s5pc110_lock_power_domain(unsigned int nToken);


#define MAXIMUM_FREQ 1000000
#define USE_FREQ_TABLE
//#undef USE_DVS
#define USE_DVS
#define VERY_HI_RATE  800*1000*1000
#define APLL_GEN_CLK  800*1000
#define KHZ_T		1000

//#define MPU_CLK		"dout_apll"
#define INDX_ERROR  65535


extern unsigned int s5pc11x_cpufreq_index;
extern unsigned int S5PC11X_FREQ_TAB;
extern unsigned int S5PC11X_MAXFREQLEVEL;


extern unsigned int s5pc11x_target_frq(unsigned int pred_freq, int flag);
extern int s5pc110_pm_target(unsigned int target_freq);
extern int is_conservative_gov(void);
extern int is_userspace_gov(void);
extern void set_dvfs_perf_level(void);
extern int set_voltage(enum perf_level p_lv);
extern int set_voltage_dvs(enum perf_level p_lv);

#if 0
#define S5PC100_LOCKHCLK_USBHOST		0xE2000001
#define S5PC100_LOCKHCLK_USBOTG		0xE2000002
#define S5PC100_LOCKHCLK_SDMMC0		0xE2000004
#define S5PC100_LOCKHCLK_SDMMC1		0xE2000008
#define S5PC100_LOCKHCLK_SDMMC2		0xE2000010
#endif

extern int s5pc110_dvfs_lock_high_hclk(unsigned int dToken);
extern int s5pc110_dvfs_unlock_high_hclk(unsigned int dToken);

#define NUMBER_OF_LOCKTOKEN 9
#if 0
#define DVFS_LOCK_TOKEN_1	 0x01 << 0
#define DVFS_LOCK_TOKEN_2	 0x01 << 1
#define DVFS_LOCK_TOKEN_3	 0x01 << 2
#define DVFS_LOCK_TOKEN_4	 0x01 << 3
#define DVFS_LOCK_TOKEN_5	 0x01 << 4
#else
#define DVFS_LOCK_TOKEN_1	 0
#define DVFS_LOCK_TOKEN_2	 1
#define DVFS_LOCK_TOKEN_3	 2
#define DVFS_LOCK_TOKEN_4	 3
#define DVFS_LOCK_TOKEN_5	 4
#define DVFS_LOCK_TOKEN_6	 5
#define DVFS_LOCK_TOKEN_7	 6
#define DVFS_LOCK_TOKEN_8	 7
#define DVFS_LOCK_TOKEN_9	 8
#endif

void s5pc110_lock_dvfs_high_level(unsigned int nToken, unsigned int level);
void s5pc110_unlock_dvfs_high_level(unsigned int nToken);

#endif /* __PLAT_S5PC11X_DVFS_H */
