/* 
 * Copyright (c) 2008 Samsung Electronics
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
 * 
 * Revision History
 * ===============
 * 0.0 Initial version WLAN power wakeup
 * 0.1 Second version for Aries platform
 * 
 */
#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <plat/sdhci.h>
#include <plat/devs.h>
#include <linux/spinlock.h>
#include <linux/mmc/host.h>

#ifdef CUSTOMER_HW_SAMSUNG

#define WLGPIO_INFO(x) printk x
#define WLGPIO_DEBUG(x)

static unsigned long flags = 0;
static spinlock_t regon_lock = SPIN_LOCK_UNLOCKED;

static void s3c_WLAN_SDIO_on(void)
{
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_CLK, S3C_GPIO_SFN(GPIO_WLAN_SDIO_CLK_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_CMD, S3C_GPIO_SFN(GPIO_WLAN_SDIO_CMD_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_D0, S3C_GPIO_SFN(GPIO_WLAN_SDIO_D0_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_D1, S3C_GPIO_SFN(GPIO_WLAN_SDIO_D1_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_D2, S3C_GPIO_SFN(GPIO_WLAN_SDIO_D2_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_D3, S3C_GPIO_SFN(GPIO_WLAN_SDIO_D3_AF));
	s3c_gpio_setpull(GPIO_WLAN_SDIO_CLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_CMD, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_D0, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_D1, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_D2, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_D3, S3C_GPIO_PULL_NONE);
}

static void s3c_WLAN_SDIO_off(void)
{
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_CLK, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_CMD, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_D0, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_D1, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_D2, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_D3, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_CLK, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_CMD, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_D0, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_D1, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_D2, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_SDIO_D3, S3C_GPIO_PULL_NONE);
}

void bcm_wlan_power_on(int flag)
{
	if (flag == 1)
	{
		/* WLAN_REG_ON control */

		WLGPIO_INFO(("[WIFI] Device powering ON\n"));

		/* Enable sdio pins and configure it */
		s3c_WLAN_SDIO_on();

		spin_lock_irqsave(&regon_lock, flags);

		/* Power on WLAN chip */ 
		s3c_gpio_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_OUTPUT);
		s3c_gpio_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_OUTPUT);
		/* Set REG ON as High & configure it for sleep mode */
		s3c_gpio_setpin(GPIO_WLAN_BT_EN, 1);
		WLGPIO_INFO(("[WIFI] GPIO_WLAN_BT_EN = %d\n", gpio_get_value(GPIO_WLAN_BT_EN)));

		msleep(100);

		/* Active low pin: Make it high for NO RESET & configure it for sleep mode */
		s3c_gpio_setpin(GPIO_WLAN_nRST, 1);
		WLGPIO_INFO(("[WIFI] GPIO_WLAN_nRST = %d\n", gpio_get_value(GPIO_WLAN_nRST)));

		s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_BT_EN, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);

		spin_unlock_irqrestore(&regon_lock, flags);

		/*gpio_free(GPIO_WLAN_BT_EN);
		gpio_free(GPIO_WLAN_nRST);*/

		msleep(100);

		sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);
	}
	else
	{
		WLGPIO_DEBUG(("bcm_wlan_power_on: flag=%d - skip\n", flag));
	}
}


void bcm_wlan_power_off(int flag)
{
	if (flag == 1) {
		/* WLAN_REG_ON control */

		WLGPIO_INFO(("[WIFI] Device powering OFF\n"));

		spin_lock_irqsave(&regon_lock, flags);
		
		/* Active Low: Assert Reset line unconditionally while turning off WIFI*/
		s3c_gpio_setpin(GPIO_WLAN_nRST, 0);
		WLGPIO_INFO(("[WIFI] GPIO_WLAN_nRST = %d \n", gpio_get_value(GPIO_WLAN_nRST)));

		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);

		if(gpio_get_value(GPIO_BT_nRST) == 0)
		{
			/* Set REG ON as low, only if BT is not operational */
			s3c_gpio_setpin(GPIO_WLAN_BT_EN, 0);

			s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);
			s3c_gpio_slp_setpull_updown(GPIO_WLAN_BT_EN, S3C_GPIO_PULL_NONE);

			WLGPIO_INFO(("[WIFI] GPIO_WLAN_BT_EN = %d \n", gpio_get_value(GPIO_WLAN_BT_EN)));
		}

		spin_unlock_irqrestore(&regon_lock, flags);

		/* Disable SDIO pins */
		s3c_WLAN_SDIO_off();

		/* gpio_free(GPIO_WLAN_BT_EN);
		gpio_free(GPIO_WLAN_nRST); */

		msleep(100);
	    
		sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);
	}
	else {
		WLGPIO_DEBUG(("bcm_wlan_power_off: flag=%d - skip\n", flag));
}
	}	

#endif /* CUSTOMER_HW_SAMSUNG */