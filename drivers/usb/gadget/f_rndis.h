/*
 * Gadget Driver for Android, with ADB and UMS and ACM and RNDIS support
 *
 * Copyright (C) 2009 Samsung Electronics, changlim choi
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

#ifndef __F_RNDIS_H
#define __F_RNDIS_H

#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
int rndis_function_add(struct usb_configuration *c, u8 ethaddr[ETH_ALEN]);
int rndis_function_config_changed(struct usb_composite_dev *cdev,	struct usb_configuration *c);

#endif /* __F_ACM_H */
