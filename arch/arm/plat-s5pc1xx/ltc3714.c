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

#define PMIC_ARM		0
#define PMIC_INT		1
#define PMIC_BOTH	2

#define SER_BIT 		0
#define SRCLK_BIT 		1
#define RCLK_ARM_BIT 	2
#define RCLK_INT_BIT 	3
#define VID_CTRL_BIT 	4

unsigned int step_curr;

enum PMIC_VOLTAGE {
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
static const unsigned int voltage_table[11] = {
	0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9,
	0x8, 0x7, 0x6, 0x5,
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
	{L0, VOUT_1_35, VOUT_1_25},
	{L1, VOUT_1_20, VOUT_1_20},
	{L2, VOUT_1_20, VOUT_1_20},
	{L3, VOUT_1_05, VOUT_1_20},
	{L4, VOUT_1_00, VOUT_1_00},	//83/66
#endif		
};

/* LTC3714 Setting Routine */
static int ltc3714_gpio_setting(void)
{
	int iter, err;

	for(iter = SER_BIT; iter <= VID_CTRL_BIT ; iter++) {
		if (gpio_is_valid(S5PC1XX_GPH0(iter))) {
				
			err = gpio_request(S5PC1XX_GPH0(iter), "GPH0"); 
			
			if (err) {
				printk(KERN_ERR "failed to request GPF0 port(%d) for LTC3714 control\n", iter);
				return err;
			}
			
			if(iter == VID_CTRL_BIT) {
				gpio_direction_output(S5PC1XX_GPH0(iter), 1);
				udelay(10);
			} else {
				gpio_direction_output(S5PC1XX_GPH0(iter), 0);
			}

			s3c_gpio_setpull(S5PC1XX_GPH0(iter), S3C_GPIO_PULL_NONE);
			
		} else {
			printk(KERN_ERR "Invalid GPIO (%d) number\n", S5PC1XX_GPH0(iter));
		}
	}
	
	return 0;
}

static int set_ltc3714(unsigned int pwr, enum perf_level p_lv)
{
	int iter = 0;
	int voltage;
	int gpio_val;
	
	if(pwr == PMIC_ARM) {
		voltage = frequency_match[p_lv][pwr + 1];
		gpio_val = voltage_table[voltage];
		gpio_set_value(S5PC1XX_GPH0(RCLK_ARM_BIT), 0);
		gpio_set_value(S5PC1XX_GPH0(VID_CTRL_BIT), 1);

		udelay(10);
		
	} else if(pwr == PMIC_INT) {
		voltage = frequency_match[p_lv][pwr + 1];
		gpio_val = voltage_table[voltage];
		gpio_set_value(S5PC1XX_GPH0(RCLK_INT_BIT), 0);
		gpio_set_value(S5PC1XX_GPH0(SRCLK_BIT), 0);
		gpio_set_value(S5PC1XX_GPH0(VID_CTRL_BIT), 1);

		udelay(10);

		gpio_set_value(S5PC1XX_GPH0(RCLK_INT_BIT), 0);
		
	} else if(pwr == PMIC_BOTH) {
		voltage = frequency_match[p_lv][1];
		gpio_val = voltage_table[voltage];
		gpio_set_value(S5PC1XX_GPH0(RCLK_ARM_BIT), 0);
		gpio_set_value(S5PC1XX_GPH0(RCLK_INT_BIT), 0);
		gpio_set_value(S5PC1XX_GPH0(VID_CTRL_BIT), 1);

		udelay(10);

	}else {
		printk("[error]: set_power, check mode [pwr] value\n");
		return -EINVAL;
	}

	//printk("gpio_val = 0x%x\n", gpio_val);
	for (iter = 8; iter > 0; iter --) {
		gpio_set_value(S5PC1XX_GPH0(SRCLK_BIT), 0);
		gpio_set_value(S5PC1XX_GPH0(SER_BIT), (gpio_val>>(iter-1))&0x01);
		udelay(10);
		gpio_set_value(S5PC1XX_GPH0(SRCLK_BIT), 1);
		udelay(10);
	}

	gpio_set_value(S5PC1XX_GPH0(SRCLK_BIT), 0);
	gpio_set_value(S5PC1XX_GPH0(SER_BIT), 0);

	udelay(10);
	
	gpio_set_value(S5PC1XX_GPH0(VID_CTRL_BIT), 0);

	switch(pwr) {
		case PMIC_ARM:
			gpio_set_value(S5PC1XX_GPH0(RCLK_ARM_BIT), 1);
			udelay(10);		
			gpio_set_value(S5PC1XX_GPH0(RCLK_ARM_BIT), 0);
			break;
		case PMIC_INT:
			gpio_set_value(S5PC1XX_GPH0(RCLK_INT_BIT), 1);
			udelay(10);
			gpio_set_value(S5PC1XX_GPH0(RCLK_INT_BIT), 0);
			break;
		case PMIC_BOTH:
			gpio_set_value(S5PC1XX_GPH0(RCLK_ARM_BIT), 1);
			gpio_set_value(S5PC1XX_GPH0(RCLK_INT_BIT), 1);
			udelay(10);			
			gpio_set_value(S5PC1XX_GPH0(RCLK_ARM_BIT), 0);
			gpio_set_value(S5PC1XX_GPH0(RCLK_INT_BIT), 0);
			break;
		default:
			break;
		
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
	printk("%s : p_lv = %d\n", __FUNCTION__, p_lv);
	if(step_curr != p_lv) {
		set_ltc3714(PMIC_ARM, p_lv);
		set_ltc3714(PMIC_INT, p_lv);

		step_curr = p_lv;
	}
	return 0;
}

EXPORT_SYMBOL(set_power);

void ltc3714_init(void)
{
	step_curr = L0;
	ltc3714_gpio_setting();
	set_power(step_curr);
}

EXPORT_SYMBOL(ltc3714_init);
