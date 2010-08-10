/*
 * Gadget Driver for Android, with ADB and UMS and ACM support
 *
 * Copyright (C) 2009 Samsung Electronics, Seung-Soo Yang
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <linux/usb/android.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>

#include "u_serial.h"
#include "f_mass_storage.h"
#include "f_adb.h"
#include "f_acm.h"

#ifdef DM_PORT_ACTIVE
#include "f_logging.h"
#endif

#include "f_mtp.h"
#include "fsa9480_i2c.h"
#include "f_rndis.h"
#include "u_ether.h"
#include "gadget_chips.h"

#include <mach/max8998_function.h>


MODULE_AUTHOR("SeungSoo Yange");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static const char longname[] = "Gadget Android";

#define DRIVER_DESC		"Android Composite USB"
#define DRIVER_VERSION	__DATE__
/* if you want to use VTP function, please enable below Feature : VTP_MODE*/
//#define VTP_MODE


/* Default vendor and product IDs, overridden by platform data */
#define VENDOR_ID		0x04e8	/* Samsung */
//#define ADB_PRODUCT_ID 	0x6601	/* Swallowtail*/
//#define ADB_PRODUCT_ID 	0x681C	/* S3C6410 Swallowtail*/
#define KIES_PRODUCT_ID 	0x6877	/* S3C6410 Swallowtail*/
#define MTP_PRODUCT_ID 	0x68A9	/* S3C6410 Swallowtail*/

#define ADB_PRODUCT_ID 	0x681C	/* S3C6410 Swallowtail*/
#define PRODUCT_ID		0x681D
#define RNDIS_PRODUCT_ID	0x6881
#define RNDIS_VENDOR_NUM	0x0525	/* NetChip */
#define RNDIS_PRODUCT_NUM	0xa4a2	/* Ethernet/RNDIS Gadget */
#define CDC_VENDOR_NUM		0x0525	/* NetChip */
#define CDC_PRODUCT_NUM		0xa4a1	/* Linux-USB Ethernet Gadget */

extern void ap_usb_power_on(int set_vaue);

void get_usb_serial(char *usb_serial_number)
{
	char temp_serial_number[13] = {0};

	u32 serial_number=0;
	
	serial_number = (system_serial_high << 16) + (system_serial_low >> 16);

#if defined(CONFIG_S5PC110_T959_BOARD)
	sprintf(temp_serial_number,"T959%08x",serial_number);
#elif defined(CONFIG_S5PC110_KEPLER_BOARD)
	sprintf(temp_serial_number,"I897%08x",serial_number);
#else
	 sprintf(temp_serial_number,"9000%08x",serial_number);
#endif

	strcpy(usb_serial_number,temp_serial_number);
}

/* 
 * Originally, adbd will enable adb function at booting 
 * if the value of persist.service.adb.enable is 1
 * ADB_ENABLE_AT_BOOT intends to enable adb function 
 * in case of no enabling by adbd or improper RFS
 */
#define ADB_ENABLE_AT_BOOT	0

struct android_dev {
	struct usb_gadget *gadget;
	struct usb_composite_dev *cdev;

	int product_id;
	int adb_product_id;
	int rndis_product_id;
	int mtp_product_id;
    int kies_product_id;
	int version;
	int adb_enabled;
#ifdef DM_PORT_ACTIVE
	int dm_enabled;
#endif
	int nluns;
};

static atomic_t adb_enable_excl;
#ifdef DM_PORT_ACTIVE
static atomic_t dm_enable_excl;
#endif
static struct android_dev *_android_dev;

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX	0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

/* String Table */
static struct usb_string strings_dev[] = {
	/* These dummy values should be overridden by platform data */
	[STRING_MANUFACTURER_IDX].s = "SAMSUNG",
	[STRING_PRODUCT_IDX].s = "SAMSUNG_Android",
	[STRING_SERIAL_IDX].s = "S5P_C110_Android",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};
static u8 hostaddr[ETH_ALEN];

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
	.bDeviceClass		  = USB_CLASS_MASS_STORAGE,
	.bDeviceSubClass	  = 0x06,//US_SC_SCSI,
	.bDeviceProtocol	  = 0x50,//US_PR_BULK,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct usb_device_descriptor eth_device_desc = {
	.bDeviceClass =		USB_CLASS_COMM,
	.bDeviceSubClass =	0,
	.bDeviceProtocol =	0,
	.idVendor =		__constant_cpu_to_le16 (CDC_VENDOR_NUM),
	.idProduct =		__constant_cpu_to_le16 (CDC_PRODUCT_NUM),
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,

	/* REVISIT SRP-only hardware is possible, although
	 * it would not be called "OTG" ...
	 */
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};

void android_usb_set_connected(int connected)
{
	if (_android_dev && _android_dev->cdev && _android_dev->cdev->gadget) {
		if (connected)
			usb_gadget_connect(_android_dev->cdev->gadget);
		else
			usb_gadget_disconnect(_android_dev->cdev->gadget);
	}
}

#if ADB_ENABLE_AT_BOOT
static void enable_adb(struct android_dev *dev, int enable);
#endif

static int __init android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret;

	//ret = acm_bind_config(c, 0);

	ret = acm_function_add(c);
	if (ret) {
		printk("[%s] Fail to acm_function_add()\n", __func__);
		return ret;
	}

	ret = mass_storage_function_add(c, dev->nluns);
	if (ret) {
		printk("[%s] Fail to ums_function_add()\n", __func__);
		return ret;
	}

	ret = adb_function_add(c);
	if (ret) {
		printk("[%s] Fail to adb_function_add()\n", __func__);
		return ret;
	}
	
	ret = mtp_function_add(c);
	if (ret) {
		printk("[%s] Fail to mtp_function_add()\n", __func__);
		return ret;
	}	
	
	ret = rndis_bind_config(c, hostaddr);
	if (ret) {
		printk("[%s] Fail to rndis_bind_config()\n", __func__);
		return ret;
	}

#ifdef DM_PORT_ACTIVE
	ret = dm_function_add(c);
	if (ret) {
		printk("[%s] Fail to dm_function_add()\n", __func__);
		return ret;
	}
#endif
return ret;
}

static int ums_only_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret;

	dev->cdev->desc.idProduct = __constant_cpu_to_le16(dev->product_id);
	dev->cdev->desc.bDeviceClass = USB_CLASS_MASS_STORAGE;
	dev->cdev->desc.bDeviceSubClass = 0x06;//US_SC_SCSI;
	dev->cdev->desc.bDeviceProtocol = 0x50;//US_PR_BULK;

	ret = mass_storage_function_config_changed(dev->cdev, c, dev->nluns);
	if (ret) {
		printk("[%s] Fail to mass_storage_function_config_changed()\n", __func__);
		return ret;
	}	
	return ret;
}

static int acm_ums_adb_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret;

	dev->cdev->desc.idProduct =	__constant_cpu_to_le16(dev->adb_product_id);
	dev->cdev->desc.bDeviceClass	  = USB_CLASS_COMM;
	dev->cdev->desc.bDeviceSubClass   = 0x00;
	dev->cdev->desc.bDeviceProtocol   = 0x00;
	
	ret = acm_function_config_changed(dev->cdev, c);
	if (ret) {
		printk("[%s] Fail to acm_function_config_changed()\n", __func__);
		return ret;
	}
	ret = mass_storage_function_config_changed(dev->cdev, c, dev->nluns);
	if (ret) {
		printk("[%s] Fail to mass_storage_function_config_changed()\n", __func__);
		return ret;
	}
	ret = adb_function_config_changed(dev->cdev, c);
	if (ret) {
		printk("[%s] Fail to adb_function_config_changed()\n", __func__);
		return ret;
	}
	return ret;
}

static int acm_mtp_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret;

	dev->cdev->desc.idProduct =	__constant_cpu_to_le16(dev->kies_product_id);
	dev->cdev->desc.bDeviceClass	  = USB_CLASS_COMM;
	dev->cdev->desc.bDeviceSubClass   = 0x00;
	dev->cdev->desc.bDeviceProtocol   = 0x00;
	
	ret = acm_function_config_changed(dev->cdev, c);
	if (ret) {
		printk("[%s] Fail to acm_function_config_changed()\n", __func__);
		return ret;
	}
	ret = mtp_function_config_changed(dev->cdev, c);
	if (ret) {
		printk("[%s] Fail to mtp_function_config_changed()\n", __func__);
		return ret;
	}
	return ret;
}

static int mtp_only_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret;

	dev->cdev->desc.idProduct = __constant_cpu_to_le16(dev->mtp_product_id);
	//dev->cdev->desc.bDeviceClass	  = USB_CLASS_STILL_IMAGE;
	//dev->cdev->desc.bDeviceSubClass   = 0x06;
	//dev->cdev->desc.bDeviceProtocol   = 0x01;

	dev->cdev->desc.bDeviceClass	  = 0x00;
	dev->cdev->desc.bDeviceSubClass   = 0x00;
	dev->cdev->desc.bDeviceProtocol   = 0x01;
	
	ret = mtp_function_config_changed(dev->cdev, c);
	if (ret) {
		printk("[%s] Fail to mtp_function_config_changed()\n", __func__);
		return ret;
	}
	return ret;
}

static int rndis_only_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret;

	dev->cdev->desc.idProduct = __constant_cpu_to_le16(RNDIS_PRODUCT_ID);
	dev->cdev->desc.bDeviceClass	  = USB_CLASS_COMM;
	dev->cdev->desc.bDeviceSubClass   = 0;
	dev->cdev->desc.bDeviceProtocol   = 0;
	
	ret = rndis_function_config_changed(dev->cdev, c);
	if (ret) {
		printk("[%s] Fail to rndis_function_config_changed()\n", __func__);
		return ret;
	}
	return ret;
}

#ifdef VODA
static int acm_only_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret;

	dev->cdev->desc.idProduct =	__constant_cpu_to_le16(dev->kies_product_id);
	dev->cdev->desc.bDeviceClass	  = USB_CLASS_COMM;
	dev->cdev->desc.bDeviceSubClass   = 0x00;
	dev->cdev->desc.bDeviceProtocol   = 0x00;
	
	ret = acm_function_config_changed(dev->cdev, c);
	if (ret) {
		printk("[%s] Fail to acm_function_config_changed()\n", __func__);
		return ret;
	}
	return ret;
}
#endif


#define	ANDROID_DEBUG_CONFIG_STRING "ACM + UMS + ADB (Debugging mode)"
#define	ANDROID_NO_DEBUG_KIES_CONFIG_STRING "ACM+MTP (No Debugging mode)"
#define	ANDROID_NO_DEBUG_MTP_CONFIG_STRING "MTP Olny (No Debugging mode)"
#define	ANDROID_NO_DEBUG_UMS_CONFIG_STRING "UMS Only (No debugging mode)"
#define	ANDROID_NO_DEBUG_RNDIS_CONIFG_STRING "RNDIS Only (No debugging mode)"
#ifdef VODA
#define	ANDROID_NO_DEBUG_ACM_CONFIG_STRING "ACM Only (No debugging mode)"
#endif


static struct usb_configuration android_config  = {
	.label		= ANDROID_NO_DEBUG_UMS_CONFIG_STRING,
	.bind		= android_bind_config,
	.bConfigurationValue = 1,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0x30, /*  x2 = 160ma */
};

static struct usb_configuration ums_only_config  = {
	.label		= ANDROID_NO_DEBUG_UMS_CONFIG_STRING,
	.bind		= ums_only_bind_config,
	.bConfigurationValue = 2,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0x30, /*  x2 = 160ma */
};

static struct usb_configuration acm_ums_adb_config  = {
	.label		= ANDROID_DEBUG_CONFIG_STRING,
	.bind		= acm_ums_adb_bind_config,
	.bConfigurationValue = 3,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0x30, /*  x2 = 160ma */
};

static struct usb_configuration acm_mtp_config  = {
	.label		= ANDROID_NO_DEBUG_KIES_CONFIG_STRING,
	.bind		= acm_mtp_bind_config,
	.bConfigurationValue = 4,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0x30, /*  x2 = 160ma */
};

static struct usb_configuration mtp_only_config  = {
	.label		= ANDROID_NO_DEBUG_MTP_CONFIG_STRING,
	.bind		= mtp_only_bind_config,
	.bConfigurationValue = 5,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0x30, /*  x2 = 160ma */
};

static struct usb_configuration rndis_only_config  = {
	.label		= ANDROID_NO_DEBUG_RNDIS_CONIFG_STRING,
	.bind		= rndis_only_bind_config,
	.bConfigurationValue = 6,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0x30, /*  x2 = 160ma */
};

#ifdef VODA
static struct usb_configuration acm_only_config  = {
	.label		= ANDROID_NO_DEBUG_ACM_CONFIG_STRING,
	.bind		= acm_only_bind_config,
	.bConfigurationValue = 7,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0x30, /*  x2 = 160ma */
};
#endif



static int __init android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum;
	int			id;
	int			ret;
	int			status;
	char usb_serial_number[13] = {0,};


	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */

	eth_device_desc.idVendor = cpu_to_le16(RNDIS_VENDOR_NUM);
	eth_device_desc.idProduct = cpu_to_le16(RNDIS_PRODUCT_NUM);
	
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;

	get_usb_serial(usb_serial_number);
	strings_dev[STRING_SERIAL_IDX].id = id;

	if( (usb_serial_number[0] + 
		 usb_serial_number[1] + 
		 usb_serial_number[2]) != 0 )
		strcpy((char *)(strings_dev[STRING_SERIAL_IDX].s), usb_serial_number);
	printk("[ADB_UMS_ACM_RNDIS_MTP] string_dev = %s \n",strings_dev[STRING_SERIAL_IDX].s);

	device_desc.iSerialNumber = id;
	device_desc.bcdDevice = cpu_to_le16(0x0400);

#if 0
	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
	{
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	}
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}
#endif
	
	if (gadget_is_otg(cdev->gadget)) 
		android_config.descriptors = otg_desc;
	
	if (gadget->ops->wakeup)
		android_config.bmAttributes |= USB_CONFIG_ATT_WAKEUP;

	/* set up network link layer */
	status = gether_setup(cdev->gadget, hostaddr);
	if (status < 0)
		return status;

	/* register our configuration */
	ret = usb_add_config(cdev, &android_config);
	if (ret) {
		printk(KERN_ERR "usb_add_config failed\n");
		goto acm_fail;
	}

	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;
	
	ret = usb_change_config(dev->cdev, &ums_only_config);
	if (ret) {
		printk("[%s] Fail to usb_change_config()\n", __func__);
		return ret;
	}

#if ADB_ENABLE_AT_BOOT
	printk("[%s] Enabling adb function at booting\n", __func__);
	enable_adb(dev, USBSTATUS_ADB);
#endif

	INFO(cdev, "%s, version: " DRIVER_VERSION "\n", DRIVER_DESC);

	return 0;
	
acm_fail:
	gether_cleanup();
	return ret;
}

/*
	Google gadget doesn't support module type building
static int __exit android_unbind(struct usb_composite_dev *cdev)
{
	gserial_cleanup();
	return 0;
}
*/

static struct usb_composite_driver android_usb_driver = {
	//driver.name refered by S3C-UDC for setting config_gadget_driver
	.name		= "android_adb_ums_acm_rndis_mtp",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.bind		= android_bind,
//	.unbind 	= __exit_p(android_unbind),
};

int currentusbstatus=0;
int oldusbstatus=0;
int UmsCDEnable=0;
int ums_mount_status = 0;
int askonstatus = 0;
int inaskonstatus=0;
static int prev_status_before_adb;  // previous USB setting before using ADB
static int prev_enable_status;  // previous USB setting
extern int mtp_mode_on;

static void enable_adb(struct android_dev *dev, int enable)
{
	int ret;

	if (enable != dev->adb_enabled) {
		prev_enable_status = dev->adb_enabled;
		printk("[USB] %s - enable(0x%02x), prev_status(0x%02x)\n", __func__, enable, prev_enable_status);
		printk("Here is enable_adb@@@@\n");

		if (dev->cdev && dev->cdev->gadget) {
			usb_gadget_disconnect(dev->cdev->gadget);
		}
askonstatus=0;
oldusbstatus = currentusbstatus;
currentusbstatus=enable;

recheck:
		ums_mount_status=0;

		/* set product ID to the appropriate value */
		if (enable== USBSTATUS_SAMSUNG_KIES)
		{
			printk("[enable_adb]USBSTATUS_SAMSUNG_KIES\n");
			mtp_mode_on = 1;
			ret = usb_change_config(dev->cdev, &acm_mtp_config);
			if (ret) {
				printk("[%s] Fail to acm_mtp_config()\n", __func__);
			}
			dev->adb_enabled = enable;
		}
		else if(enable == USBSTATUS_MTPONLY)
		{
			printk("[enable_adb]USBSTATUS_MTPONLY\n");
			mtp_mode_on = 1;
			ret = usb_change_config(dev->cdev, &mtp_only_config);
			if (ret) {
				printk("[%s] Fail to mtp_only_config()\n", __func__);
			}
			dev->adb_enabled = enable;
		}
		else if(enable == USBSTATUS_ASKON)
		{		
			printk("[enable_adb]USBSTATUS_ASKON\n");
			mtp_mode_on = 0;
			askonstatus=1;
			dev->adb_enabled = enable;
		}
		else if(enable == USBSTATUS_VTP) 
		{
			printk("[enable_adb]USBSTATUS_VTP\n");
#ifdef VTP_MODE
		mtp_mode_on = 0;
		UmsCDEnable=1;
		ret = usb_change_config(dev->cdev, &ums_only_config);
		if (ret) {
			printk("[%s] Fail to ums_only_config()\n", __func__);
				}
		dev->adb_enabled = enable;
#else
		mtp_mode_on = 0;
        ret = usb_change_config(dev->cdev, &rndis_only_config);
			if (ret) {
				printk("[%s] Fail to rndis_only_config()\n", __func__);
			}
			dev->adb_enabled = enable;
#endif
		}
		else if(enable == USBSTATUS_UMS)
		{
			printk("[enable_adb]USBSTATUS_UMS\n");
			mtp_mode_on = 0;
			UmsCDEnable=0;
			if(prev_enable_status == USBSTATUS_ADB && prev_status_before_adb != USBSTATUS_UMS) {
				printk("[USB] %s - prev_status(0x%02x), prev_status_before_adb setting(0x%02x)\n",
						__func__, prev_enable_status, prev_status_before_adb);
				enable = prev_status_before_adb;  // set previous status
				prev_enable_status = prev_status_before_adb = 0; //reset
				goto recheck;
				}

			ret = usb_change_config(dev->cdev, &ums_only_config);
			if (ret) {
				printk("[%s] Fail to ums_only_config()\n", __func__);
			}
			dev->adb_enabled = enable;
		}
		else if(enable == USBSTATUS_ADB)
		{
			printk("[enable_adb]USBSTATUS_ADB\n");
			mtp_mode_on = 0;
			prev_status_before_adb = prev_enable_status;
			ret = usb_change_config(dev->cdev, &acm_ums_adb_config);
			if (ret) {
				printk("[%s] Fail to acm_ums_adb_config()\n", __func__);
			}
			dev->adb_enabled = enable;
		}

/*
	for reenumeration in case of booting-up connected with host
	because if connected, host don't enumerate 
*/
		if(!mtp_mode_on)
			ap_usb_power_on(1);
		else
			ap_usb_power_on(0);

		if (dev->cdev && dev->cdev->gadget ) {
             usb_gadget_connect(dev->cdev->gadget);
           	}
		}
	}

static void disable_mtp(struct android_dev *dev)
{
	int ret;
	
	if (dev->cdev && dev->cdev->gadget) {
	usb_gadget_disconnect(dev->cdev->gadget);
	}

#ifdef VODA
    mdelay(1000);
	ret = usb_change_config(dev->cdev, &acm_only_config);
#else
	ap_usb_power_on(0);
#endif

	if (dev->cdev && dev->cdev->gadget ) {
		 usb_gadget_connect(dev->cdev->gadget);
		}
}

extern void UsbIndicator(u8 state);

static void enable_askon(struct android_dev *dev, int enable)
{
	int ret;

		ums_mount_status=0;

	
	if (dev->cdev && dev->cdev->gadget) {
	usb_gadget_disconnect(dev->cdev->gadget);
	}

oldusbstatus = currentusbstatus;
currentusbstatus=enable;


		/* set product ID to the appropriate value */
		if (enable== USBSTATUS_SAMSUNG_KIES)
		{
			printk("[enable_askon]USBSTATUS_SAMSUNG_KIES\n");
			mtp_mode_on = 1;
			inaskonstatus = 1;
			UsbIndicator(1);
			ret = usb_change_config(dev->cdev, &acm_mtp_config);
			if (ret) {
				printk("[%s] Fail to acm_mtp_config()\n", __func__);
			}
		}
		else if(enable == USBSTATUS_MTPONLY)
		{
			printk("[enable_askon]USBSTATUS_MTPONLY\n");
			mtp_mode_on = 1;
			inaskonstatus = 1;			
			UsbIndicator(1);
			ret = usb_change_config(dev->cdev, &mtp_only_config);
			if (ret) {
				printk("[%s] Fail to mtp_only_config()\n", __func__);
			}
		}
		else if(enable == USBSTATUS_VTP) 
		{
			printk("[enable_askon]USBSTATUS_VTP\n");
                        inaskonstatus = 1;		
#ifdef VTP_MODE
		mtp_mode_on = 0;
		UsbIndicator(1);
		UmsCDEnable=1;
		ret = usb_change_config(dev->cdev, &ums_only_config);
		if (ret) {
			printk("[%s] Fail to ums_only_config()\n", __func__);
				}
#else
		mtp_mode_on = 0;
		UsbIndicator(1);
             ret = usb_change_config(dev->cdev, &rndis_only_config);
			if (ret) {
				printk("[%s] Fail to rndis_only_config()\n", __func__);
			}
#endif
		}
		else if(enable == USBSTATUS_UMS)
		{
			printk("[enable_askon]USBSTATUS_UMS\n");
			mtp_mode_on = 0;
			inaskonstatus = 1;			
			UmsCDEnable=0;
			UsbIndicator(1);

			ret = usb_change_config(dev->cdev, &ums_only_config);
			if (ret) {
				printk("[%s] Fail to ums_only_config()\n", __func__);
			}
		}
/*
	for reenumeration in case of booting-up connected with host
	because if connected, host don't enumerate 
*/
	if(!mtp_mode_on)
		ap_usb_power_on(1);
	
		if (dev->cdev && dev->cdev->gadget ) {
             usb_gadget_connect(dev->cdev->gadget);
           	}
		}

void askon_gadget_disconnect(void)
{
	struct android_dev *dev = _android_dev;

	if(askonstatus)
		usb_gadget_disconnect(dev->cdev->gadget);

}

static int adb_enable_open(struct inode *ip, struct file *fp)
{

	if (atomic_inc_return(&adb_enable_excl) != 1) {
		atomic_dec(&adb_enable_excl);
		return -EBUSY;
	}
        printk(KERN_INFO "enabling adb\n");
	enable_adb(_android_dev, USBSTATUS_ADB);
	atomic_dec(&adb_enable_excl);

	return 0;
}

static int adb_enable_release(struct inode *ip, struct file *fp)
{
	if (atomic_inc_return(&adb_enable_excl) != 1) {
	atomic_dec(&adb_enable_excl);
	return -EBUSY;
	}
	enable_adb(_android_dev, 0);
	atomic_dec(&adb_enable_excl);
	return 0;
}

int usb_mtp_select(int disable)
{
	if (atomic_inc_return(&adb_enable_excl) != 1) {
	atomic_dec(&adb_enable_excl);
	return -EBUSY;
	}

	disable_mtp(_android_dev);
	atomic_dec(&adb_enable_excl);
	return 0;
}

int usb_switch_select(int enable)
{
	if (atomic_inc_return(&adb_enable_excl) != 1) {
	atomic_dec(&adb_enable_excl);
	return -EBUSY;
	}

	enable_adb(_android_dev, enable);
	atomic_dec(&adb_enable_excl);
	return 0;
}

int askon_switch_select(int enable)
{
	if (atomic_inc_return(&adb_enable_excl) != 1) {
	atomic_dec(&adb_enable_excl);
	return -EBUSY;
	}

	enable_askon(_android_dev, enable);
	atomic_dec(&adb_enable_excl);
	return 0;
}


static const struct file_operations adb_enable_fops = {
	.owner =   THIS_MODULE,
	.open =    adb_enable_open,
	.release = adb_enable_release,
};

static struct miscdevice adb_enable_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "android_adb_enable",
	.fops = &adb_enable_fops,
};


#ifdef DM_PORT_ACTIVE
static int dm_enable_open(struct inode *ip, struct file *fp)
{
	if (atomic_inc_return(&dm_enable_excl) != 1) {
		atomic_dec(&dm_enable_excl);
		return -EBUSY;
	}

	printk(KERN_INFO "enabling dm\n");
	enable_adb(_android_dev, 1);

	return 0;
}

static int dm_enable_release(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "disabling dm\n");
	enable_adb(_android_dev, 0);
	atomic_dec(&dm_enable_excl);
	return 0;
}

static const struct file_operations dm_enable_fops = {
	.owner =   THIS_MODULE,
	.open =    dm_enable_open,
	.release = dm_enable_release,
};

static struct miscdevice dm_enable_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "dm_port_enable",
	.fops = &dm_enable_fops,
};
#endif

static int __init android_probe(struct platform_device *pdev)
{
	struct android_usb_platform_data *pdata = pdev->dev.platform_data;
	struct android_dev *dev = _android_dev;

	if (pdata) {
		if (pdata->vendor_id)
			device_desc.idVendor =
				__constant_cpu_to_le16(pdata->vendor_id);
		if (pdata->product_id) {
			dev->product_id = pdata->product_id;
			device_desc.idProduct =
				__constant_cpu_to_le16(pdata->product_id);
		}
		if (pdata->adb_product_id)
			dev->adb_product_id = pdata->adb_product_id;

		if (pdata->mtp_product_id)
			dev->mtp_product_id = pdata->mtp_product_id;
		
		if (pdata->kies_product_id)
			dev->kies_product_id = pdata->kies_product_id;

		if (pdata->rndis_product_id)
			dev->rndis_product_id = pdata->rndis_product_id;

		if (pdata->version)
			dev->version = pdata->version;

		if (pdata->product_name)
			strings_dev[STRING_PRODUCT_IDX].s = pdata->product_name;
		if (pdata->manufacturer_name)
			strings_dev[STRING_MANUFACTURER_IDX].s =
					pdata->manufacturer_name;
		if (pdata->serial_number)
			strings_dev[STRING_SERIAL_IDX].s = pdata->serial_number;
		dev->nluns = pdata->nluns;
	}

	return 0;
}

static struct platform_driver android_platform_driver = {
	.driver = { .name = "C110 Android USB", },
	.probe = android_probe,
};

static int __init init(void)
{
	struct android_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* set default values, which should be overridden by platform data */
	dev->product_id = PRODUCT_ID;
	dev->adb_product_id = ADB_PRODUCT_ID;
	dev->mtp_product_id = MTP_PRODUCT_ID;
	dev->kies_product_id = KIES_PRODUCT_ID;
	dev->rndis_product_id = RNDIS_PRODUCT_ID;

	_android_dev = dev;

	ret = platform_driver_register(&android_platform_driver);
	if (ret)
		return ret;
	ret = misc_register(&adb_enable_device);
	if (ret) {
		platform_driver_unregister(&android_platform_driver);
		return ret;
	}
#ifdef DM_PORT_ACTIVE
	ret = misc_register(&dm_enable_device);
	if (ret) {
		platform_driver_unregister(&android_platform_driver);
		return ret;
	}	
#endif
	ret = usb_composite_register(&android_usb_driver);
	if (ret) {
		misc_deregister(&adb_enable_device);
#ifdef DM_PORT_ACTIVE
		misc_deregister(&dm_enable_device);
#endif
		platform_driver_unregister(&android_platform_driver);
	}
	return ret;
}
module_init(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&android_usb_driver);
	misc_deregister(&adb_enable_device);
#ifdef DM_PORT_ACTIVE
	misc_deregister(&dm_enable_device);	
#endif
	platform_driver_unregister(&android_platform_driver);
	kfree(_android_dev);
	_android_dev = NULL;
	gserial_cleanup();
}
module_exit(cleanup);
