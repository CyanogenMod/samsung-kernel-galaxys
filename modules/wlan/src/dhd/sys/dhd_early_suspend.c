/*
 * Broadcom Dongle Host Driver (DHD), Linux-specific network interface
 * Basically selected code segments from usb-cdc.c and usb-rndis.c
 *
 * Copyright (C) 1999-2009, Broadcom Corporation
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhd_linux.c,v 1.65.4.9.2.13.6.37 2009/06/05 00:40:45 Exp $
 */

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */
#include <linux/leds.h>
#include <linux/suspend.h>
#include <typedefs.h>

extern int dhd_deep_sleep(int flag, int etc);

static int dhd_early_sleep_pm_callback(struct notifier_block *nfb,
					unsigned long action,
					void *ignored);

static struct notifier_block dhd_early_sleep_pm_notifier = {
	.notifier_call = dhd_early_sleep_pm_callback,
	.priority = 0,
};

extern uint32 dhd_is_linkup;
 /* to make sync */
static int dhd_is_indeepsleep = FALSE;
static int dhd_early_drv_loaded = FALSE;


static int dhd_suspend (void)
{
	/* If not connected to AP, Wifi is going to sleep */
	if ( dhd_is_linkup == FALSE) {
		printk("[WiFi]%s: SUSPEND \n", __FUNCTION__);
		dhd_deep_sleep(TRUE, 1);
		/* now, we set deepsleep status */
		dhd_is_indeepsleep = 1;
	}
	return 0;
}


static int dhd_resume (void)
{
	if(dhd_is_indeepsleep) {
		dhd_deep_sleep(FALSE, 1);
		dhd_is_indeepsleep = 0;
		printk("[WiFi] %s: RESUME \n", __FUNCTION__);
	}

	return 0;
}

static int dhd_early_sleep_pm_callback(struct notifier_block *nfb,
					unsigned long action,
					void *ignored)
{
	switch (action) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		return NOTIFY_OK;
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
	//	printk(KERN_INFO"[WiFi] %s \n",__FUNCTION__);
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
extern uint g_chipactive_flag;
static void dhd_early_suspend(struct early_suspend *h)
{
	/* Waiting for dhd driver loaded fully */
	if(!g_chipactive_flag) 
		return;
	dhd_suspend();
}

static void dhd_late_resume(struct early_suspend *h)
{
	dhd_resume();
}

static struct early_suspend dhd_early_sleep_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 20,
	.suspend = dhd_early_suspend,
	.resume = dhd_late_resume,
};

int register_dhd_early_suspend(void)
{
	register_pm_notifier(&dhd_early_sleep_pm_notifier);
	register_early_suspend(&dhd_early_sleep_suspend_handler);
	dhd_early_drv_loaded = TRUE;

	return 0;
}

void unregister_dhd_early_suspend(void)
{
	if (dhd_early_drv_loaded == FALSE )
			return;
	unregister_early_suspend(&dhd_early_sleep_suspend_handler);
	unregister_pm_notifier(&dhd_early_sleep_pm_notifier);
}
#endif /* CONFIG_HAS_EARLYSUSPEND */
