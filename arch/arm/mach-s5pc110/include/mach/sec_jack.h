/*
 * Copyright (C) 2008 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_ARCH_SEC_HEADSET_H
#define __ASM_ARCH_SEC_HEADSET_H

#define SEC_HEADSET_ADC_CHANNEL 3
enum
{
	SEC_JACK_NO_DEVICE					= 0x0,
	SEC_HEADSET_4_POLE_DEVICE			= 0x01 << 0,	
	SEC_HEADSET_3_POLE_DEVICE			= 0x01 << 1,
//doesn't use below 3 dev.	
	SEC_TTY_DEVICE						= 0x01 << 2,
	SEC_FM_HEADSET						= 0x01 << 3,
	SEC_FM_SPEAKER						= 0x01 << 4,	
	SEC_TVOUT_DEVICE					= 0x01 << 5,
	SEC_EXTRA_DOCK_SPEAKER				= 0x01 << 6,	
	SEC_EXTRA_CAR_DOCK_SPEAKER			= 0x01 << 7,		
	SEC_UNKNOWN_DEVICE					= 0x01 << 8,	
};

enum
{
	JACK_DETACHED		= 0x0,
	JACK_ATTACHED		= 0x1,
};

struct sec_gpio_info 
{
	int	eint;
	int	gpio;
	int	gpio_af;
	int	low_active;
};

struct sec_jack_port
{
	struct sec_gpio_info	det_jack;
	struct sec_gpio_info	send_end;
	struct sec_gpio_info send_end35;
};

struct sec_jack_platform_data
{
	struct sec_jack_port	*port;
	int			nheadsets;
};

unsigned int get_headset_status(void);
void select_gpio_earmic_bias(void); //hdlnc_ysyim_2010-05-04
void select_enable_irq(void);
void select_disable_irq(void);	
int get_gpio_send_end_state(void);
#endif
