/*
 * max8698.c  --  Voltage and current regulation for the Maxim 8698
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
#include <linux/regulator/max8698.h>
#include <linux/mutex.h>

/* Registers */
#define MAX8698_REG_ONOFF1	0x00
#define MAX8698_REG_ONOFF2	0x01
#define MAX8698_REG_DVSARM12	0x04
#define MAX8698_REG_DVSARM34	0x05
#define MAX8698_REG_DVSINT12	0x06
#define MAX8698_REG_BUCK3	0x07
#define MAX8698_REG_LDO23	0x08
#define MAX8698_REG_LDO8	0x0D

struct max8698_data {
	struct i2c_client	*client;
	struct device		*dev;

	struct mutex		mutex;

	int			num_regulators;
	struct regulator_dev	**rdev;
};

static u8 max8698_cache_regs[16] = {
	0xFA, 0xB1, 0xFF, 0xF9,
	0x99, 0x99, 0x99, 0x02,
	0x88, 0x02, 0x0C, 0x0A,
	0x0E, 0x33, 0x0E, 0x16,
};

static int max8698_i2c_cache_read(struct i2c_client *client, u8 reg, u8 *dest)
{
	*dest = max8698_cache_regs[reg];
	return 0;
}

static int max8698_i2c_read(struct i2c_client *client, u8 reg, u8 *dest)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return -EIO;

	max8698_cache_regs[reg] = ret;

	*dest = ret & 0xff;
	return 0;
}

static int max8698_i2c_write(struct i2c_client *client, u8 reg, u8 value)
{
	max8698_cache_regs[reg] = value;
	return i2c_smbus_write_byte_data(client, reg, value);
}

static u8 max8698_read_reg(struct max8698_data *max8698, u8 reg)
{
	u8 val = 0;

	mutex_lock(&max8698->mutex);
	max8698_i2c_cache_read(max8698->client, reg, &val);
	mutex_unlock(&max8698->mutex);

	return val;
}

static int max8698_write_reg(struct max8698_data *max8698, u8 value, u8 reg)
{

	mutex_lock(&max8698->mutex);

	max8698_i2c_write(max8698->client, reg, value);

	mutex_unlock(&max8698->mutex);

	return 0;
}

static const int ldo23_voltage_map[] = {
	 800,  850,  900,  950, 1000,
	1050, 1100, 1150, 1200, 1250,
	1300,
};

static const int ldo45679_voltage_map[] = {
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

static const int dcdc12_voltage_map[] = {
	 750,  800,  850,  900,  950,
	1000, 1050, 1100, 1150, 1200,
	1250, 1300, 1350, 1400, 1450,
	1500,
};

static const int *ldo_voltage_map[] = {
	NULL,
	ldo23_voltage_map,	/* LDO1 */
	ldo23_voltage_map,	/* LDO2 */
	ldo23_voltage_map,	/* LDO3 */
	ldo45679_voltage_map,	/* LDO4 */
	ldo45679_voltage_map,	/* LDO5 */
	ldo45679_voltage_map,	/* LDO6 */
	ldo45679_voltage_map,	/* LDO7 */
	ldo8_voltage_map,	/* LDO8 */
	ldo45679_voltage_map,	/* LDO9 */
	dcdc12_voltage_map,	/* DCDC1 */
	dcdc12_voltage_map,	/* DCDC2 */
	ldo45679_voltage_map,	/* DCDC3 */
};

static int max8698_get_ldo(struct regulator_dev *rdev)
{
	return rdev_get_id(rdev);
}

static int max8698_ldo_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int ldo = max8698_get_ldo(rdev);
	return 1000 * ldo_voltage_map[ldo][selector];
}

static int max8698_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int ldo = max8698_get_ldo(rdev);
	int value, shift;

	if (ldo <= MAX8698_LDO5) {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF1);
		shift = 6 - ldo;
	} else if (ldo <= MAX8698_LDO9) {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF2);
		shift = 13 - ldo;
	} else {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF1);
		shift = 7 - (ldo - MAX8698_DCDC1);
	}
	return (value >> shift) & 0x1;
}

static int max8698_ldo_enable(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int ldo = max8698_get_ldo(rdev);
	int value, shift;

	if (ldo <= MAX8698_LDO5) {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF1);
		shift = 6 - ldo;
		value |= (1 << shift);
		max8698_write_reg(max8698, value, MAX8698_REG_ONOFF1);
	} else if (ldo <= MAX8698_LDO9) {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF2);
		shift = 13 - ldo;
		value |= (1 << shift);
		max8698_write_reg(max8698, value, MAX8698_REG_ONOFF2);
	} else {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF1);
		shift = 7 - (ldo - MAX8698_DCDC1);
		value |= (1 << shift);
		max8698_write_reg(max8698, value, MAX8698_REG_ONOFF1);
	}

	return 0;
}

static int max8698_ldo_disable(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int ldo = max8698_get_ldo(rdev);
	int value, shift;
	
	if (ldo <= MAX8698_LDO5) {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF1);
		shift = 6 - ldo;
		value &= ~(1 << shift);
		max8698_write_reg(max8698, value, MAX8698_REG_ONOFF1);
	} else if (ldo <= MAX8698_LDO9) {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF2);
		shift = 13 - ldo;
		value &= ~(1 << shift);
		max8698_write_reg(max8698, value, MAX8698_REG_ONOFF2);
	} else {
		value = max8698_read_reg(max8698, MAX8698_REG_ONOFF1);
		shift = 7 - (ldo - MAX8698_DCDC1);
		value |= (1 << shift);
		max8698_write_reg(max8698, value, MAX8698_REG_ONOFF1);
	}

	return 0;
}

static int max8698_ldo_get_voltage(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int ldo = max8698_get_ldo(rdev);
	int value, shift = 0, mask = 0xff, reg;

	if (ldo == MAX8698_LDO2) {
		reg  = MAX8698_REG_LDO23;
		mask = 0x0f;
	} else if (ldo == MAX8698_LDO3) {
		reg  = MAX8698_REG_LDO23;
		shift = 4;
	} else if (ldo == MAX8698_LDO8) {
		reg = ldo + 5;
		shift = 4;
	} else if (ldo <= MAX8698_LDO9) {
		reg = ldo + 5;
	} else if (ldo == MAX8698_DCDC1) {
		reg = 0x04;
		mask = 0x0f;
	} else if (ldo == MAX8698_DCDC2) {
		reg = 0x06;
		mask = 0x0f;
	} else
		reg = 0x07;

	value = max8698_read_reg(max8698, reg);
	value >>= shift;
	value &= mask;

	return 1000 * ldo_voltage_map[ldo][value];
}

static int max8698_ldo_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int ldo = max8698_get_ldo(rdev);
	int value, shift = 0, mask = 0xff, reg;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = ldo_voltage_map[ldo];
	int i = 0;

	if (min_vol < 800 ||
	    max_vol > 3600)
		return -EINVAL;

	while (vol_map[i]) {
		if (vol_map[i] >= min_vol)
			break;
		i++;
	}

	if (!vol_map[i])
		return -EINVAL;

	if (vol_map[i] > max_vol)
		return -EINVAL;

	if (ldo == MAX8698_LDO2) {
		reg  = MAX8698_REG_LDO23;
		mask = 0x0f;
	} else if (ldo == MAX8698_LDO3) {
		reg  = MAX8698_REG_LDO23;
		shift = 4;
		mask = 0xf0;
	} else if (ldo == MAX8698_LDO8) {
		reg = ldo + 5;
		shift = 4;
		mask = 0xf0;
	} else if (ldo <= MAX8698_LDO9) {
		reg = ldo + 5;
		mask = 0xff;
	} else if (ldo == MAX8698_DCDC1) {
		reg = 0x04;
		mask = 0x0f;
	} else if (ldo == MAX8698_DCDC2) {
		reg = 0x06;
		mask = 0x0f;
	} else {
		reg = 0x07;
		mask = 0xff;
	}

	value = max8698_read_reg(max8698, reg);
	value &= ~mask;
	value |= (i << shift);
	max8698_write_reg(max8698, value, reg);

	return 0;
}

static struct regulator_ops max8698_ldo_ops = {
//	.list_voltage	= max8698_ldo_list_voltage,
	.is_enabled	= max8698_ldo_is_enabled,
	.enable		= max8698_ldo_enable,
	.disable	= max8698_ldo_disable,
	.get_voltage	= max8698_ldo_get_voltage,
	.set_voltage	= max8698_ldo_set_voltage,
};

static struct regulator_ops max8698_dcdc_ops = {
//	.list_voltage	= max8698_ldo_list_voltage,
	.is_enabled	= max8698_ldo_is_enabled,
	.enable		= max8698_ldo_enable,
	.disable	= max8698_ldo_disable,
	.get_voltage	= max8698_ldo_get_voltage,
	.set_voltage	= max8698_ldo_set_voltage,
};

static struct regulator_desc regulators[] = {
	{
		.name		= "LDO1",
		.id		= MAX8698_LDO1,
		.ops		= &max8698_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO2",
		.id		= MAX8698_LDO2,
		.ops		= &max8698_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo23_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO3",
		.id		= MAX8698_LDO3,
		.ops		= &max8698_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo23_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO4",
		.id		= MAX8698_LDO4,
		.ops		= &max8698_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO5",
		.id		= MAX8698_LDO5,
		.ops		= &max8698_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO6",
		.id		= MAX8698_LDO6,
		.ops		= &max8698_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO7",
		.id		= MAX8698_LDO7,
		.ops		= &max8698_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO8",
		.id		= MAX8698_LDO8,
		.ops		= &max8698_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo8_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO9",
		.id		= MAX8698_LDO9,
		.ops		= &max8698_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "DCDC1",
		.id		= MAX8698_DCDC1,
		.ops		= &max8698_dcdc_ops,
	//	.n_voltages	= ARRAY_SIZE(dcdc12_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "DCDC2",
		.id		= MAX8698_DCDC2,
		.ops		= &max8698_dcdc_ops,
	//	.n_voltages	= ARRAY_SIZE(dcdc12_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "DCDC3",
		.id		= MAX8698_DCDC3,
		.ops		= &max8698_dcdc_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	},
};

static int __devinit max8698_pmic_probe(struct i2c_client *client,
				const struct i2c_device_id *i2c_id)
{
	struct regulator_dev **rdev;
	struct max8698_platform_data *pdata = client->dev.platform_data;
	struct max8698_data *max8698;
	int i = 0, id, ret;

	if (!pdata)
		return -EINVAL;

	max8698 = kzalloc(sizeof(struct max8698_data), GFP_KERNEL);
	if (!max8698)
		return -ENOMEM;

	max8698->rdev = kzalloc(sizeof(struct regulator_dev *) * (pdata->num_regulators + 1), GFP_KERNEL);
	if (!max8698->rdev) {
		kfree(max8698);
		return -ENOMEM;
	}

	max8698->client = client;
	max8698->dev = &client->dev;
	mutex_init(&max8698->mutex);


	max8698->num_regulators = pdata->num_regulators;
	for (i = 0; i < pdata->num_regulators; i++) {
		id = pdata->regulators[i].id - MAX8698_LDO1;
		max8698->dev->platform_data = pdata->regulators[i].platform_data; // hack ??
		max8698->rdev[i] = regulator_register(&regulators[id],
			//max8698->dev, pdata->regulators[i].initdata, max8698);
			max8698->dev, max8698);

		ret = IS_ERR(max8698->rdev[i]);
		if (ret)
			dev_err(max8698->dev, "regulator init failed\n");
	}

	max8698->dev->platform_data = pdata; // restoring back
	rdev = max8698->rdev;

	max8698_i2c_read(client, MAX8698_REG_ONOFF1, (u8 *) &ret);

	i2c_set_clientdata(client, max8698);
	return 0;
}

static int __devexit max8698_pmic_remove(struct i2c_client *client)
{
	struct max8698_data *max8698 = i2c_get_clientdata(client);
	struct regulator_dev **rdev = max8698->rdev;
	int i;

	for (i = 0; i <= max8698->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
	kfree(max8698->rdev);
	kfree(max8698);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id max8698_ids[] = {
	{ "max8698", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, max8698_ids);

static struct i2c_driver max8698_pmic_driver = {
	.probe		= max8698_pmic_probe,
	.remove		= __devexit_p(max8698_pmic_remove),
	.driver		= {
		.name	= "max8698",
	},
	.id_table	= max8698_ids,
};

static int __init max8698_pmic_init(void)
{
	return i2c_add_driver(&max8698_pmic_driver);
}
module_init(max8698_pmic_init);

static void __exit max8698_pmic_exit(void)
{
	i2c_del_driver(&max8698_pmic_driver);
};
module_exit(max8698_pmic_exit);

MODULE_DESCRIPTION("MAXIM 8698 voltage regulator driver");
MODULE_AUTHOR("Kyungmin Park <kyungmin.park@samsung.com>");
MODULE_LICENSE("GPL");
