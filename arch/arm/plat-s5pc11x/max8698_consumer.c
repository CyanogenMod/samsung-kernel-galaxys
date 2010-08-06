/*
 *  linux/arch/arm/plat-s5pc11x/max8698_consumer.c
 *
 *  CPU frequency scaling for S5PC110
 *
 *  Copyright (C) 2009 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/cpu-freq.h>
#include <linux/platform_device.h>
#include <linux/regulator/max8698.h>
#include <linux/regulator/consumer.h>

#define DBG printk

#define PMIC_ARM		0
#define PMIC_INT		1
#define PMIC_BOTH	2

unsigned int step_curr;

enum PMIC_VOLTAGE {
	VOUT_0_75, 
	VOUT_0_80, 
	VOUT_0_85, 
	VOUT_0_90, 
	VOUT_0_95, 
	VOUT_1_00, 
	VOUT_1_05, 
	VOUT_1_10, 
	VOUT_1_15, 
	VOUT_1_20, 
	VOUT_1_25, 
	VOUT_1_30, 
	VOUT_1_35, 
	VOUT_1_40, 
	VOUT_1_45, 
	VOUT_1_50 	
};

/* ltc3714 voltage table */
static const unsigned int voltage_table[16] = {
	750, 800, 850, 900, 950, 1000, 1050,
	1100, 1150, 1200, 1250, 1300, 1350,
	1400, 1450, 1500
};


/* frequency voltage matching table */
static const unsigned int frequency_match[][3] = {
/* frequency, Mathced VDD ARM voltage , Matched VDD INT*/
#if 0
	{L0, VOUT_1_30, VOUT_1_30},
	{L1, VOUT_1_20, VOUT_1_20},
	{L2, VOUT_1_10, VOUT_1_20},
	{L3, VOUT_1_05, VOUT_1_20},
	{L4, VOUT_1_05, VOUT_1_20},
#else
	{L0, VOUT_1_20, VOUT_1_20},
	{L1, VOUT_1_10, VOUT_1_10},
#endif
};


struct regulator *Arm, *Int;


static int set_max8698(unsigned int pwr, enum perf_level p_lv)
{
	int voltage;
	int pmic_val;
	int ret = 0;
	DBG("%s : p_lv = %d : pwr = %d \n", __FUNCTION__, p_lv,pwr);
	
	if(pwr == PMIC_ARM) {
		voltage = frequency_match[p_lv][pwr + 1];
		pmic_val = voltage_table[voltage] * 1000;


		ret = regulator_enable(Arm);
	        if(ret != 0)
        	{
                	printk("\n regulator_enable failed \n");
			return -1;
        	}

		/*set Arm voltage*/	
		ret = regulator_set_voltage(Arm,pmic_val,pmic_val);
	        if(ret != 0)
        	{
                	printk("\n regulator_set_voltage failed \n");
        	}

		udelay(10);
		ret = regulator_disable(Arm);
                if(ret != 0)
                {
                        printk("\n regulator_disable failed \n");
                        return -1;
                }

		
	} else if(pwr == PMIC_INT) {
		voltage = frequency_match[p_lv][pwr + 1];
		pmic_val = voltage_table[voltage];

		udelay(10);
		
	} else if(pwr == PMIC_BOTH) {
		voltage = frequency_match[p_lv][1];
		pmic_val = voltage_table[voltage];

		udelay(10);

	}else {
		printk("[error]: set_power, check mode [pwr] value\n");
		return -EINVAL;
	}
	udelay(10);
	//printk("%s : end\n", __FUNCTION__);
	return 0;
}

static int find_voltage(int freq)
{
	int index = 0;

	if(freq > frequency_match[0][0]){
		printk(KERN_ERR "frequecy is over then support frequency\n");
		return 0;
	}

	for(index = 0 ; index < ARRAY_SIZE(frequency_match) ; index++){
		if(freq >= frequency_match[index][0])
			return index;
	}

	printk("Cannot find matched voltage on table\n");

	return 0;
}

int set_power(enum perf_level p_lv)
{
	DBG("%s : p_lv = %d\n", __FUNCTION__, p_lv);
	//if(step_curr != p_lv) 
	{
		set_max8698(PMIC_ARM, p_lv);
	//	set_max8698(PMIC_INT, p_lv);

		step_curr = p_lv;
	}
	return 0;
}

EXPORT_SYMBOL(set_power);

void max8698_init(void)
{
	step_curr = L0;
	set_power(step_curr);
}

EXPORT_SYMBOL(max8698_init);

static int max8698_consumer_probe(struct platform_device *pdev)
{
        int ret = 0;

	DBG("\nmax8698_consumer_probe \n");

        Arm = regulator_get(NULL, "vddarm");
        if (IS_ERR(Arm)) {
                printk(KERN_ERR "%s: cant get VCC_ARM\n", __func__);
                return PTR_ERR(Arm);
        }

	/*Initialize the pmic voltage*/

	max8698_init();
	
	return ret;	
}

static int max8698_consumer_remove(struct platform_device *pdev)
{
        int ret = 0;

	DBG("\nmax8698_consumer_exit called \n");
	regulator_put(Arm);
	return ret;

}
static struct platform_driver max8698_consumer_driver = {
	.driver = {
		   .name = "max8698-consumer",
		   .owner = THIS_MODULE,
		   },
	.probe = max8698_consumer_probe,
	.remove = max8698_consumer_remove,
};

static int max8698_consumer_init(void)
{
	DBG("\nmax8698_consumer_init \n");
	return platform_driver_register(&max8698_consumer_driver);
}
late_initcall(max8698_consumer_init);

static void max8698_consumer_exit(void)
{
	platform_driver_unregister(&max8698_consumer_driver);
}
module_exit(max8698_consumer_exit);

MODULE_AUTHOR("Amit Daniel");
MODULE_DESCRIPTION("MAX 8698 consumer driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:max8698-consumer");
