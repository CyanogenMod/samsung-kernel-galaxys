/*
 * pmic_cam.h  --  PMIC abstraction for camera sensor 
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

#ifndef __PMIC_CAM_H_  
#define __PMIC_CAM_H_ 

#include "max8998.h"

#define LDO_CAM_CORE	MAX8998_DCDC4
#define LDO_CAM_IO	MAX8998_LDO11
#define LDO_CAM_5M	MAX8998_LDO12
#define LDO_CAM_A	MAX8998_LDO13
#define LDO_CAM_CIF	MAX8998_LDO14
#define LDO_CAM_AF	MAX8998_LDO15

#define pmic_ldo_enable(x)	max8998_ldo_enable_direct(x)	
#define pmic_ldo_disable(x)	max8998_ldo_disable_direct(x)

#endif /* __PMIC_CAM_H_  */
