/*
 * max8998.c  --  Voltage and current regulation for the Maxim 8998
 *
 * Copyright (C) 2009 Samsung Electronics
 * Kyungmin Park <kyungmin.park@samsung.com>
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
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/max8998.h>
#include <linux/mutex.h>
#include <mach/max8998_function.h>

/* Registers */
#ifdef MAX8698
#define MAX8698_REG_ONOFF1	0x00
#define MAX8698_REG_ONOFF2	0x01
#define MAX8698_REG_DVSARM12	0x04
#define MAX8698_REG_DVSARM34	0x05
#define MAX8698_REG_DVSINT12	0x06
#define MAX8698_REG_BUCK3	0x07
#define MAX8698_REG_LDO23	0x08
#define MAX8698_REG_LDO8	0x0D

#else

#define MAX8998_REG_ONOFF1	0x11
#define MAX8998_REG_ONOFF2	0x12
#define MAX8998_REG_ONOFF3	0x13
#define MAX8998_REG_ONOFF4	0x14
#define MAX8998_REG_DVSARM1	0x15
#define MAX8998_REG_DVSARM2	0x16
#define MAX8998_REG_DVSARM3	0x17
#define MAX8998_REG_DVSARM4	0x18
#define MAX8998_REG_DVSINT1	0x19
#define MAX8998_REG_DVSINT2	0x1A
#define MAX8998_REG_BUCK3	0x1B
#define MAX8998_REG_BUCK4	0x1C
#define MAX8998_REG_LDO23	0x1D
#define MAX8998_REG_LDO89	0x22
#define MAX8998_REG_LDO1011	0x23
#define MAX8998_REG_LDO12	0x24
#define MAX8998_REG_LDO13	0x25
#define MAX8998_REG_LDO14	0x26
#define MAX8998_REG_LDO15	0x27
#define MAX8998_REG_LDO16	0x28
#define MAX8998_REG_LDO17	0x29

#endif

DEFINE_SPINLOCK(pmic_access_lock);

#define DBG(fmt...)
//#define DBG(fmt...) printk(fmt)

struct max8998_data {
	struct i2c_client	*client;
	struct device		*dev;

	struct mutex		mutex;

	int			num_regulators;
	struct regulator_dev	**rdev;
};

struct i2c_client	*max8998_client;

#ifdef MAX8698

static u8 max8698_cache_regs[16] = {
	0xFA, 0xB1, 0xFF, 0xF9,
	0x99, 0x99, 0x99, 0x02,
	0x88, 0x02, 0x0C, 0x0A,
	0x0E, 0x33, 0x0E, 0x16,
};
#else
u8 max8998_cache_regs[] = {
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0xEF, 0xB8, 0x00, 0xF9, //0x11 //NAGSM_Android_HDLNC_Camera_wsj_20100429 : to maintain LDO4 power on state
        0x12, 0xFF, 0xFF, 0xFF, //0x15
        0x12, 0x12, 0x02, 0x04, //0x19
        0x88, 0x02, 0x02, 0x02, //0x1D
	0x02, 0x30, 0xAC, 0x0A, //0x21
	0x14, 0x06, 0x10, 0x11,	//0x25
	0x11,			//0x29
};
#endif

struct max8998_data *client_data_p = NULL;

void lpm_mode_check(void);

static int max8998_i2c_cache_read(struct i2c_client *client, u8 reg, u8 *dest)
{
	*dest = max8998_cache_regs[reg];
	return 0;
}

static int max8998_i2c_read(struct i2c_client *client, u8 reg, u8 *dest)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return -EIO;

	max8998_cache_regs[reg] = ret;

	*dest = ret & 0xff;
	return 0;
}

static int max8998_i2c_write(struct i2c_client *client, u8 reg, u8 value)
{
	max8998_cache_regs[reg] = value;
	return i2c_smbus_write_byte_data(client, reg, value);
}

static u8 max8998_read_reg(struct max8998_data *max8998, u8 reg)
{

	u8 val = 0;
	DBG("func =%s called for reg= %x\n",__func__,reg);

#ifndef CONFIG_CPU_FREQ
	mutex_lock(&max8998->mutex);
#endif
	max8998_i2c_cache_read(max8998->client, reg, &val);
#ifndef CONFIG_CPU_FREQ
	mutex_unlock(&max8998->mutex);
#endif

	return val;
}

static int max8998_write_reg(struct max8998_data *max8998, u8 value, u8 reg)
{

	DBG("func =%s called for reg= %x, data=%x\n",__func__,reg,value);

#ifndef CONFIG_CPU_FREQ
	mutex_lock(&max8998->mutex);
#endif

	max8998_i2c_write(max8998->client, reg, value);
#ifndef CONFIG_CPU_FREQ
	mutex_unlock(&max8998->mutex);
#endif
	return 0;
}

static const int ldo23_voltage_map[] = {
	 800,  850,  900,  950, 1000,
	1050, 1100, 1150, 1200, 1250,
	1300,
};

static const int ldo4567_voltage_map[] = {
	1600, 1700, 1800, 1900, 2000,
	2100, 2200, 2300, 2400, 2500,
	2600, 2700, 2800, 2900, 3000,
	3100, 3200, 3300, 3400, 3500,
	3600,
};

static const int ldo8_voltage_map[] = {
	3000, 3100, 3200, 3300, 3400,
	3500, 3600,
};

static const int ldo9_voltage_map[] = {
	2800, 2900, 3000, 3100,
};

static const int ldo10_voltage_map[] = {
	 950, 1000, 1050, 1100, 1150, 1200, 1250, 1300, 
};

static const int ldo11_voltage_map[] = {
        1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500,
        2600, 2700, 2800, 2900, 3000, 3100, 3200, 3300, 3400, 3500,
        3600,
};

static const int ldo1213_voltage_map[] = {
	 800,  900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700,
	1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700,
	2800, 2900, 3000, 3100, 3200, 3300, 
};

static const int ldo1415_voltage_map[] = {
	1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 
	2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000, 3100, 
	3200, 3300,
};

static const int dcdc12_voltage_map[] = {
	 750,  775,  800,  825,  850,  875,  900,  925,  950,  975,
	1000, 1025, 1050, 1075, 1100, 1125, 1150, 1175, 1200, 1225, 
	1250, 1275, 1300, 1325, 1350, 1375, 1400, 1425, 1450, 1475, 
	1500, 1525,
};

static const int dcdc4_voltage_map[] = {
	 800,  900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700,
	1800, 1900, 2000, 2100, 2200, 2300,
};


static const int *ldo_voltage_map[] = {
	NULL,
	ldo23_voltage_map,	/* LDO1 */
	ldo23_voltage_map,	/* LDO2 */
	ldo23_voltage_map,	/* LDO3 */
	ldo4567_voltage_map,	/* LDO4 */
	ldo4567_voltage_map,	/* LDO5 */
	ldo4567_voltage_map,	/* LDO6 */
	ldo4567_voltage_map,	/* LDO7 */
	ldo8_voltage_map,	/* LDO8 */
	ldo9_voltage_map,	/* LDO8 */
	ldo10_voltage_map,	/* LDO10 */
	ldo11_voltage_map,	/* LDO11 */
	ldo1213_voltage_map,	/* LDO12 */
	ldo1213_voltage_map,	/* LDO13 */
	ldo1415_voltage_map,	/* LDO14 */
	ldo1415_voltage_map,	/* LDO15 */
	ldo11_voltage_map,      /* LDO16 */
	ldo11_voltage_map,      /* LDO17 */
	dcdc12_voltage_map,	/* DCDC1 */
	dcdc12_voltage_map,	/* DCDC2 */
	ldo4567_voltage_map,	/* DCDC3 */
	dcdc4_voltage_map,	/* DCDC4 */
};

static int max8998_get_ldo(struct regulator_dev *rdev)
{
	return rdev_get_id(rdev);
}

static int max8998_ldo_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int ldo = max8998_get_ldo(rdev);
	return 1000 * ldo_voltage_map[ldo][selector];
}

static int max8998_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct max8998_data *max8998 = rdev_get_drvdata(rdev);
	int ldo = max8998_get_ldo(rdev);
	int value, shift;

	if (ldo <= MAX8998_LDO5) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		//shift = 6 - ldo;
		shift = 5 - ldo;
	} else if (ldo <= MAX8998_LDO13) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF2);
		shift = 13 - ldo;
	} else if (ldo <= MAX8998_LDO17) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF3);
		shift = 21 - ldo;
	} else {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 7 - (ldo - MAX8998_DCDC1);
	}
	return (value >> shift) & 0x1;
}

static int max8998_ldo_enable(struct regulator_dev *rdev)
{
	struct max8998_data *max8998 = rdev_get_drvdata(rdev);
	int ldo = max8998_get_ldo(rdev);
	int value, shift;

	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	spin_lock(&pmic_access_lock);

	if (ldo <= MAX8998_LDO5) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 5 - ldo;
		value |= (1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF1);
	} else if (ldo <= MAX8998_LDO13) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF2);
		shift = 13 - ldo;
		value |= (1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF2);
	} else if (ldo <= MAX8998_LDO17) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF3);
		shift = 21 - ldo;
		value |= (1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF3);
	} else {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 7 - (ldo - MAX8998_DCDC1);
		value |= (1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF1);
	}

	spin_unlock(&pmic_access_lock);

	return 0;
}

static int max8998_ldo_disable(struct regulator_dev *rdev)
{
	struct max8998_data *max8998 = rdev_get_drvdata(rdev);
	int ldo = max8998_get_ldo(rdev);
	int value, shift;
	
	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	spin_lock(&pmic_access_lock);

	if (ldo <= MAX8998_LDO5) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		//shift = 6 - ldo;
		shift = 5 - ldo;
		value &= ~(1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF1);
	} else if (ldo <= MAX8998_LDO13) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF2);
		shift = 13 - ldo;
		value &= ~(1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF2);
	} else if (ldo <= MAX8998_LDO17) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF3);
		shift = 21 - ldo;
		value &= ~(1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF3);
	} else {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 7 - (ldo - MAX8998_DCDC1);
		value &= ~(1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF1);
	}

	spin_unlock(&pmic_access_lock);

	return 0;
}

static int max8998_ldo_get_voltage(struct regulator_dev *rdev)
{
	struct max8998_data *max8998 = rdev_get_drvdata(rdev);
	int ldo = max8998_get_ldo(rdev);
	int value, shift = 0, mask = 0xff, reg;

	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	if (ldo == MAX8998_LDO3) {
		reg  = MAX8998_REG_LDO23;
		mask = 0x0F;
	} else if (ldo == MAX8998_LDO2) {
		reg  = MAX8998_REG_LDO23;
		shift = 4;
		mask = 0x0F;
	} else if (ldo == MAX8998_LDO8) {
		reg  = MAX8998_REG_LDO89;
		shift = 4;
		mask = 0x07;
	} else if (ldo == MAX8998_LDO9) {
		reg  = MAX8998_REG_LDO89;
		shift = 2;
		mask = 0x03;
	} else if (ldo == MAX8998_LDO10) {
		reg  = MAX8998_REG_LDO1011;
		shift = 5;
		mask = 0x07;
	} else if (ldo == MAX8998_LDO11) {
		reg  = MAX8998_REG_LDO1011;
		mask = 0x1F;
	} else if (ldo == MAX8998_LDO12) {
		reg  = MAX8998_REG_LDO12;
	} else if (ldo == MAX8998_LDO13) {
		reg  = MAX8998_REG_LDO13;
	} else if (ldo == MAX8998_LDO14) {
		reg  = MAX8998_REG_LDO14;
	} else if (ldo == MAX8998_LDO15) {
		reg  = MAX8998_REG_LDO15;
	} else if (ldo == MAX8998_LDO16) {
		reg  = MAX8998_REG_LDO16;
	} else if (ldo == MAX8998_LDO17) {
		reg  = MAX8998_REG_LDO17;
	} else if (ldo == MAX8998_DCDC1) {
		//reg = 0x04;
		reg = MAX8998_REG_DVSARM1;
		mask = 0x1f;
	} else if (ldo == MAX8998_DCDC2) {
		//reg = 0x06;
		reg = MAX8998_REG_DVSINT1;
		mask = 0x1f;
	} else if (ldo == MAX8998_DCDC3) {
		reg = MAX8998_REG_BUCK3;
		mask = 0xFF;
	} else if (ldo == MAX8998_DCDC4) {
		reg = MAX8998_REG_BUCK4;
		mask = 0xff;
	} else {// for ldo4,5,6,7
		reg = MAX8998_REG_LDO23 + (ldo - MAX8998_LDO3);
		mask = 0xff;
	}

	value = max8998_read_reg(max8998, reg);
	value >>= shift;
	value &= mask;

	return 1000 * ldo_voltage_map[ldo][value];
}

static int max8998_ldo_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV)
{
	struct max8998_data *max8998 = rdev_get_drvdata(rdev);
	int ldo = max8998_get_ldo(rdev);
	int value, shift = 0, mask = 0xff, reg;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = ldo_voltage_map[ldo];
	int i = 0;
	DBG("func =%s called for regulator = %d, min_v=%d, max_v=%d\n",__func__,ldo,min_uV,max_uV);

	if (min_vol < 750 ||
	    max_vol > 3600)
		return -EINVAL;

	while (vol_map[i]) {
		if (vol_map[i] >= min_vol)
			break;
		i++;
	}
	
	DBG("Found voltage=%d\n",vol_map[i]);

	if (!vol_map[i])
		return -EINVAL;

	if (vol_map[i] > max_vol)
		return -EINVAL;

	if (ldo == MAX8998_LDO3) {
		reg  = MAX8998_REG_LDO23;
		mask = 0x0F;
	} else if (ldo == MAX8998_LDO2) {
		reg  = MAX8998_REG_LDO23;
		shift = 4;
		mask = 0x0F;
	} else if (ldo == MAX8998_LDO8) {
		reg  = MAX8998_REG_LDO89;
		shift = 4;
		mask = 0x07;
	} else if (ldo == MAX8998_LDO9) {
		reg  = MAX8998_REG_LDO89;
		shift = 2;
		mask = 0x03;
	} else if (ldo == MAX8998_LDO10) {
		reg  = MAX8998_REG_LDO1011;
		shift = 5;
		mask = 0x07;
	} else if (ldo == MAX8998_LDO11) {
		reg  = MAX8998_REG_LDO1011;
		mask = 0x1F;
	} else if (ldo == MAX8998_LDO12) {
		reg  = MAX8998_REG_LDO12;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO13) {
		reg  = MAX8998_REG_LDO13;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO14) {
		reg  = MAX8998_REG_LDO14;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO15) {
		reg  = MAX8998_REG_LDO15;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO16) {
		reg  = MAX8998_REG_LDO16;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO17) {
		reg  = MAX8998_REG_LDO17;
		mask = 0xFF;
	} else if (ldo == MAX8998_DCDC1) {
		//reg = 0x04;
		reg = MAX8998_REG_DVSARM1;
		mask = 0x1f;
	} else if (ldo == MAX8998_DCDC2) {
		//reg = 0x06;
		reg = MAX8998_REG_DVSINT1;
		mask = 0x1f;
	} else if (ldo == MAX8998_DCDC3) {
		reg = MAX8998_REG_BUCK3;
		mask = 0xff;
	} else if (ldo == MAX8998_DCDC4) {
		reg = MAX8998_REG_BUCK4;
		mask = 0xff;
	} else {// for ldo4,5,6,7
		reg = MAX8998_REG_LDO23 + (ldo - MAX8998_LDO3);
		mask = 0xff;
	}

	spin_lock(&pmic_access_lock);

	value = max8998_read_reg(max8998, reg);
	value &= ~(mask << shift);
	value |= (i << shift);
	max8998_write_reg(max8998, value, reg);

	spin_unlock(&pmic_access_lock);

	return 0;
}



///////////////////////////////////////////////////////////////////////////////////////
//DIRECT APIS

int max8998_ldo_enable_direct(int ldo)
{
	struct max8998_data *max8998 = client_data_p;
	int value, shift;

	if((ldo < MAX8998_LDO1) || (ldo > MAX8998_DCDC4))
	{
		printk("ERROR: Invalid argument passed\n");
		return -EINVAL;
	}

	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	spin_lock(&pmic_access_lock);

	if (ldo <= MAX8998_LDO5) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 5 - ldo;
		value |= (1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF1);
	} else if (ldo <= MAX8998_LDO13) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF2);
		shift = 13 - ldo;
		value |= (1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF2);
	} else if (ldo <= MAX8998_LDO17) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF3);
		shift = 21 - ldo;
		value |= (1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF3);
	} else {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 7 - (ldo - MAX8998_DCDC1);
		value |= (1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF1);
	}

	spin_unlock(&pmic_access_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(max8998_ldo_enable_direct);

int max8998_ldo_disable_direct(int ldo)
{
	struct max8998_data *max8998 = client_data_p;
	int value, shift;

	if((ldo < MAX8998_LDO1) || (ldo > MAX8998_DCDC4))
	{
		printk("ERROR: Invalid argument passed\n");
		return -EINVAL;
	}

	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	spin_lock(&pmic_access_lock);

	if (ldo <= MAX8998_LDO5) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 5 - ldo;
		value &= ~(1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF1);
	} else if (ldo <= MAX8998_LDO13) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF2);
		shift = 13 - ldo;
		value &= ~(1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF2);
	} else if (ldo <= MAX8998_LDO17) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF3);
		shift = 21 - ldo;
		value &= ~(1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF3);
	} else {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 7 - (ldo - MAX8998_DCDC1);
		value &= ~(1 << shift);
		max8998_write_reg(max8998, value, MAX8998_REG_ONOFF1);
	}

	spin_unlock(&pmic_access_lock);

	return 0;
}

EXPORT_SYMBOL_GPL(max8998_ldo_disable_direct);


int max8998_ldo_set_voltage_direct(int ldo,
				int min_uV, int max_uV)
{
	struct max8998_data *max8998 = client_data_p;
	int value, shift = 0, mask = 0xff, reg;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	int i = 0;
	const int *vol_map = NULL;

	if((ldo < MAX8998_LDO1) || (ldo > MAX8998_DCDC4))
	{
		printk("ERROR: Invalid argument passed\n");
		return -EINVAL;
	}

	vol_map = ldo_voltage_map[ldo];
	DBG("func =%s called for regulator = %d, min_v=%d, max_v=%d\n",__func__,ldo,min_uV,max_uV);

	if (min_vol < 750 ||
	    max_vol > 3600)
		return -EINVAL;

	while (vol_map[i]) {
		if (vol_map[i] >= min_vol)
			break;
		i++;
	}
	
	DBG("Found voltage=%d\n",vol_map[i]);

	if (!vol_map[i])
		return -EINVAL;

	if (vol_map[i] > max_vol)
		return -EINVAL;

	if (ldo == MAX8998_LDO3) {
		reg  = MAX8998_REG_LDO23;
		mask = 0x0F;
	} else if (ldo == MAX8998_LDO2) {
		reg  = MAX8998_REG_LDO23;
		shift = 4;
		mask = 0x0F;
	} else if (ldo == MAX8998_LDO8) {
		reg  = MAX8998_REG_LDO89;
		shift = 4;
		mask = 0x07;
	} else if (ldo == MAX8998_LDO9) {
		reg  = MAX8998_REG_LDO89;
		shift = 2;
		mask = 0x03;
	} else if (ldo == MAX8998_LDO10) {
		reg  = MAX8998_REG_LDO1011;
		shift = 5;
		mask = 0x07;
	} else if (ldo == MAX8998_LDO11) {
		reg  = MAX8998_REG_LDO1011;
		mask = 0x1F;
	} else if (ldo == MAX8998_LDO12) {
		reg  = MAX8998_REG_LDO12;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO13) {
		reg  = MAX8998_REG_LDO13;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO14) {
		reg  = MAX8998_REG_LDO14;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO15) {
		reg  = MAX8998_REG_LDO15;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO16) {
		reg  = MAX8998_REG_LDO16;
		mask = 0xFF;
	} else if (ldo == MAX8998_LDO17) {
		reg  = MAX8998_REG_LDO17;
		mask = 0xFF;
	} else if (ldo == MAX8998_DCDC1) {
		//reg = 0x04;
		reg = MAX8998_REG_DVSARM1;
		mask = 0x1f;
	} else if (ldo == MAX8998_DCDC2) {
		//reg = 0x06;
		reg = MAX8998_REG_DVSINT1;
		mask = 0x1f;
	} else if (ldo == MAX8998_DCDC3) {
		reg = MAX8998_REG_BUCK3;
		mask = 0xff;
	} else if (ldo == MAX8998_DCDC4) {
		reg = MAX8998_REG_BUCK4;
		mask = 0xff;
	} else {// for ldo4,5,6,7
		reg = MAX8998_REG_LDO23 + (ldo - MAX8998_LDO3);
		mask = 0xff;
	}

	spin_lock(&pmic_access_lock);

	value = max8998_read_reg(max8998, reg);
	value &= ~(mask << shift);
	value |= (i << shift);
	max8998_write_reg(max8998, value, reg);

	spin_unlock(&pmic_access_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(max8998_ldo_set_voltage_direct);

int max8998_ldo_is_enabled_direct(int ldo)
{
	struct max8998_data *max8998 = client_data_p;
	int value, shift;
	if((ldo < MAX8998_LDO1) || (ldo > MAX8998_DCDC4))
	{
		printk("ERROR: Invalid argument passed\n");
		return -EINVAL;
	}
	DBG("func =%s called for regulator = %d\n",__func__,ldo);
	if (ldo <= MAX8998_LDO5) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 5 - ldo;
	} else if (ldo <= MAX8998_LDO13) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF2);
		shift = 13 - ldo;
	} else if (ldo <= MAX8998_LDO17) {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF3);
		shift = 21 - ldo;
	} else {
		value = max8998_read_reg(max8998, MAX8998_REG_ONOFF1);
		shift = 7 - (ldo - MAX8998_DCDC1);
	}
	return (value >> shift) & 0x1;
}
EXPORT_SYMBOL_GPL(max8998_ldo_is_enabled_direct);


int max8998_set_dvsarm_direct(u32 armSet, u32 voltage)
{
	struct max8998_data *max8998 = client_data_p;
        int reg, value, i;
	DBG("func =%s called with set=%d, voltage=%d",__func__,armSet,voltage);
	if(armSet == DVSARM1)
		reg = MAX8998_REG_DVSARM1;
	else if(armSet == DVSARM2)
		reg = MAX8998_REG_DVSARM2;	
	else if(armSet == DVSARM3)
		reg = MAX8998_REG_DVSARM3;	
	else if(armSet == DVSARM4)
		reg = MAX8998_REG_DVSARM4;
	else {
		printk("Invalid parameter of dvs arm\n");
		return -EINVAL;
	}
	
	for( i =0; i < sizeof(dcdc12_voltage_map)/4; i++)
	{
		if(dcdc12_voltage_map[i] == voltage)
			break;
	}
	if(i == sizeof(dcdc12_voltage_map)/4)
	{
		printk("Invalid parameter of voltage value for dvs arm\n");
		return -EINVAL;
	}

	value = i;
	max8998_write_reg(max8998, value, reg);	
	return 0;
}
EXPORT_SYMBOL_GPL(max8998_set_dvsarm_direct);

int max8998_set_dvsint_direct(u32 armSet, u32 voltage)
{
	struct max8998_data *max8998 = client_data_p;
        int reg, i, value;
	DBG("func =%s called with set=%d, voltage=%d",__func__, armSet, voltage);
	if(armSet == DVSINT1)
		reg = MAX8998_REG_DVSINT1;
	else if(armSet == DVSINT2)
		reg = MAX8998_REG_DVSINT2;	
	else {
		printk("Invalid parameter for dvs int\n");
		return -EINVAL;
	}

	for( i =0; i < sizeof(dcdc12_voltage_map)/4; i++)
	{
		if(dcdc12_voltage_map[i] == voltage)
			break;
	}
	if(i == sizeof(dcdc12_voltage_map)/4)
	{
		printk("Invalid parameter of voltage value for dvs int\n");
		return -EINVAL;
	}

	value = i;
	max8998_write_reg(max8998, value, reg);	
	return 0;
}
EXPORT_SYMBOL_GPL(max8998_set_dvsint_direct);



///////////////////////////////////////////////////////////////////////////////////////

static struct regulator_ops max8998_ldo_ops = {
//	.list_voltage	= max8998_ldo_list_voltage,
	.is_enabled	= max8998_ldo_is_enabled,
	.enable		= max8998_ldo_enable,
	.disable	= max8998_ldo_disable,
	.get_voltage	= max8998_ldo_get_voltage,
	.set_voltage	= max8998_ldo_set_voltage,
};

static struct regulator_ops max8998_dcdc_ops = {
//	.list_voltage	= max8998_ldo_list_voltage,
	.is_enabled	= max8998_ldo_is_enabled,
	.enable		= max8998_ldo_enable,
	.disable	= max8998_ldo_disable,
	.get_voltage	= max8998_ldo_get_voltage,
	.set_voltage	= max8998_ldo_set_voltage,
};

static struct regulator_desc regulators[] = {
	{
		.name		= "LDO1",
		.id		= MAX8998_LDO1,
		.ops		= &max8998_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO2",
		.id		= MAX8998_LDO2,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo23_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO3",
		.id		= MAX8998_LDO3,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo23_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO4",
		.id		= MAX8998_LDO4,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO5",
		.id		= MAX8998_LDO5,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO6",
		.id		= MAX8998_LDO6,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO7",
		.id		= MAX8998_LDO7,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO8",
		.id		= MAX8998_LDO8,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo8_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO9",
		.id		= MAX8998_LDO9,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO10",
		.id		= MAX8998_LDO10,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO11",
		.id		= MAX8998_LDO11,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO12",
		.id		= MAX8998_LDO12,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO13",
		.id		= MAX8998_LDO13,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO14",
		.id		= MAX8998_LDO14,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO15",
		.id		= MAX8998_LDO15,
		.ops		= &max8998_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO16",
		.id		= MAX8998_LDO16,
		.ops		= &max8998_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO17",
		.id		= MAX8998_LDO17,
		.ops		= &max8998_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "DCDC1",
		.id		= MAX8998_DCDC1,
		.ops		= &max8998_dcdc_ops,
	//	.n_voltages	= ARRAY_SIZE(dcdc12_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "DCDC2",
		.id		= MAX8998_DCDC2,
		.ops		= &max8998_dcdc_ops,
	//	.n_voltages	= ARRAY_SIZE(dcdc12_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "DCDC3",
		.id		= MAX8998_DCDC3,
		.ops		= &max8998_dcdc_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "DCDC4",
		.id		= MAX8998_DCDC4,
		.ops		= &max8998_dcdc_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	},
};

static int __devinit max8998_pmic_probe(struct i2c_client *client,
				const struct i2c_device_id *i2c_id)
{
	struct regulator_dev **rdev;
	struct max8998_platform_data *pdata = client->dev.platform_data;
	struct max8998_data *max8998;
	int i = 0, id, ret;


	if (!pdata)
		return -EINVAL;

	max8998 = kzalloc(sizeof(struct max8998_data), GFP_KERNEL);
	if (!max8998)
		return -ENOMEM;

	max8998->rdev = kzalloc(sizeof(struct regulator_dev *) * (pdata->num_regulators + 1), GFP_KERNEL);
	if (!max8998->rdev) {
		kfree(max8998);
		return -ENOMEM;
	}

	max8998->client = client;
	max8998->dev = &client->dev;
	mutex_init(&max8998->mutex);

	max8998_client=client;

	max8998->num_regulators = pdata->num_regulators;
	for (i = 0; i < pdata->num_regulators; i++) {

		DBG("regulator_register called for ldo=%d\n",pdata->regulators[i].id);
		id = pdata->regulators[i].id - MAX8998_LDO1;

		max8998->rdev[i] = regulator_register(&regulators[id],
			max8998->dev, pdata->regulators[i].initdata, max8998);

		ret = IS_ERR(max8998->rdev[i]);
		if (ret)
			dev_err(max8998->dev, "regulator init failed\n");
	}

	rdev = max8998->rdev;

	max8998_i2c_read(client, MAX8998_REG_ONOFF1, (u8 *) &ret);

	i2c_set_clientdata(client, max8998);
	client_data_p = max8998; // store 8998 client data to be used later

	lpm_mode_check();
	
	return 0;
}

static int __devexit max8998_pmic_remove(struct i2c_client *client)
{
	struct max8998_data *max8998 = i2c_get_clientdata(client);
	struct regulator_dev **rdev = max8998->rdev;
	int i;

	for (i = 0; i <= max8998->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
	kfree(max8998->rdev);
	kfree(max8998);
	i2c_set_clientdata(client, NULL);
	client_data_p = NULL;

	return 0;
}

static const struct i2c_device_id max8998_ids[] = {
	{ "max8998", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, max8998_ids);

static struct i2c_driver max8998_pmic_driver = {
	.probe		= max8998_pmic_probe,
	.remove		= __devexit_p(max8998_pmic_remove),
	.driver		= {
		.name	= "max8998",
	},
	.id_table	= max8998_ids,
};

static int __init max8998_pmic_init(void)
{
	return i2c_add_driver(&max8998_pmic_driver);
}
//module_init(max8998_pmic_init);
subsys_initcall(max8998_pmic_init);

static void __exit max8998_pmic_exit(void)
{
	i2c_del_driver(&max8998_pmic_driver);
};
module_exit(max8998_pmic_exit);

MODULE_DESCRIPTION("MAXIM 8998 voltage regulator driver");
MODULE_AUTHOR("Kyungmin Park <kyungmin.park@samsung.com>");
MODULE_LICENSE("GPL");

#include "max8998_function.c"
