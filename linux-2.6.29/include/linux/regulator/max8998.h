/*
 * max8998.h  --  Voltage regulation for the Maxim 8998
 *
 * Copyright (C) 2009 Samsung Electrnoics
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

#ifndef REGULATOR_MAX8998
#define REGULATOR_MAX8998

#include <linux/regulator/machine.h>

enum {
	MAX8998_LDO1 = 1,
	MAX8998_LDO2,
	MAX8998_LDO3,
	MAX8998_LDO4,
	MAX8998_LDO5,
	MAX8998_LDO6,
	MAX8998_LDO7,
	MAX8998_LDO8,
	MAX8998_LDO9,
	MAX8998_LDO10,
	MAX8998_LDO11,
	MAX8998_LDO12,
	MAX8998_LDO13,
	MAX8998_LDO14,
	MAX8998_LDO15,
	MAX8998_LDO16,
	MAX8998_LDO17,
	MAX8998_DCDC1,
	MAX8998_DCDC2,
	MAX8998_DCDC3,
	MAX8998_DCDC4,
};




/**
 * max8998_subdev_data - regulator data
 * @id: regulator Id
 * @initdata: regulator init data (contraints, supplies, ...)
 */
struct max8998_subdev_data {
	int		id;
	struct regulator_init_data	*initdata;
};

/**
 * max8998_platform_data - platform data for max8998
 * @num_regulators: number of regultors used
 * @regulators: regulator used
 */
struct max8998_platform_data {
	int num_regulators;
	struct max8998_subdev_data *regulators;
};

/*MAX8998_LDO1 <= ldo <= MAX8998_DCDC4*/
int max8998_ldo_enable_direct(int ldo);
int max8998_ldo_disable_direct(int ldo);
int max8998_ldo_is_enabled_direct(int ldo);
/*750000uV <= min_uV,max_uV <= 3600000, Applied voltage will be 
 * closest to min_uV*/
int max8998_ldo_set_voltage_direct(int ldo, int min_uV, int max_uV);
int max8998_set_dvsarm_direct(u32 armSet, u32 voltage);
int max8998_set_dvsint_direct(u32 armSet, u32 voltage);

#endif
