/*
 * linux/drivers/power/s3c6410_battery.c
 *
 * Battery measurement code for S3C6410 platform.
 *
 * based on palmtx_battery.c
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/battery.h>
#include <plat/gpio-cfg.h>
#include <linux/earlysuspend.h>
#include <linux/io.h>
#include <plat/regs-clock.h>
#include <plat/regs-power.h>
#include <mach/map.h>
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#include <plat/s5pc110.h>
#include <plat/regs-gpio.h>

#include "s5pc110_battery.h"

#include <mach/max8998_function.h>
static struct wake_lock vbus_wake_lock;
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
static struct wake_lock low_battery_wake_lock;

#include <linux/i2c.h>
#include "fuel_gauge.c"

//NAGSM_Android_SEL_Kernel_Aakash_20100312
struct class *battery_class;
struct device *s5pc110bat_dev;
//NAGSM_Android_SEL_Kernel_Aakash_20100312

/* Prototypes */
extern int s3c_adc_get_adc_data(int channel);
extern void MAX8998_IRQ_init(void);
extern void maxim_charging_control(unsigned int dev_type  , unsigned int cmd, int uicharging);

// [[ junghyunseok edit for stepcharging 20100506
extern void stepcharging_Timer_setup();

extern void maxim_topoff_change(void);
extern unsigned char maxim_chg_status(void);
extern unsigned char maxim_charging_enable_status(void);
#ifdef defined(__PMIC_V_F__)
extern unsigned char maxim_vf_status(void);
#endif
extern u8 FSA9480_Get_JIG_Status(void);
extern void set_low_bat_interrupt(int on);
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
irqreturn_t low_battery_isr(int irq, void *dev_id);

//#define BATTERY_DEBUG
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || (defined CONFIG_S5PC110_FLEMING_BOARD)
#ifdef BATTERY_DEBUG
void PMIC_dump(void);
#endif
#endif

#ifdef __TEST_DEVICE_DRIVER__
extern int amp_enable(int);
extern int audio_power(int);

static ssize_t s3c_test_show_property(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t s3c_test_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static int bat_temper_state = 0;
static struct wake_lock wake_lock_for_dev;
#endif /* __TEST_DEVICE_DRIVER__ */


//#define LPM_MODE

#define TRUE		1
#define FALSE	0

#define ADC_DATA_ARR_SIZE	6
#define ADC_TOTAL_COUNT		10
#define POLLING_INTERVAL	5000

#define POLLING_INTERVAL_TEST	1000

/* Offset Bit Value */
#define OFFSET_VIBRATOR_ON		(0x1 << 0)
#define OFFSET_CAMERA_ON		(0x1 << 1)
#define OFFSET_MP3_PLAY			(0x1 << 2)
#define OFFSET_VIDEO_PLAY		(0x1 << 3)
#define OFFSET_VOICE_CALL_2G		(0x1 << 4)
#define OFFSET_VOICE_CALL_3G		(0x1 << 5)
#define OFFSET_DATA_CALL		(0x1 << 6)
#define OFFSET_LCD_ON			(0x1 << 7)
#define OFFSET_TA_ATTACHED		(0x1 << 8)
#define OFFSET_CAM_FLASH		(0x1 << 9)
#define OFFSET_BOOTING			(0x1 << 10)
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || (defined CONFIG_S5PC110_FLEMING_BOARD)
#define OFFSET_WIFI				(0x1 << 11)
#define OFFSET_GPS				(0x1 << 12)
#endif

#define INVALID_VOL_ADC		160

// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
static struct work_struct low_bat_work;

typedef enum {
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC,
	CHARGER_DISCHARGE
} charger_type_t;


/*
static char *status_text[] = {
	[POWER_SUPPLY_STATUS_UNKNOWN] 		=	"Unknown",
	[POWER_SUPPLY_STATUS_CHARGING] 		=	"Charging",
	[POWER_SUPPLY_STATUS_DISCHARGING]		=	"Discharging",
	[POWER_SUPPLY_STATUS_NOT_CHARGING]	=	"Not Charging",
	[POWER_SUPPLY_STATUS_FULL] 				=	"Full",
};
*/

struct battery_info {
	u32 batt_id;		/* Battery ID from ADC */
	s32 batt_vol;		/* Battery voltage from ADC */
	s32 batt_vol_adc;	/* Battery ADC value */
	s32 batt_vol_adc_cal;	/* Battery ADC value (calibrated)*/
	s32 batt_temp;		/* Battery Temperature (C) from ADC */
	s32 batt_temp_adc;	/* Battery Temperature ADC value */
	s32 batt_temp_adc_cal;	/* Battery Temperature ADC value (calibrated) */
	s32 batt_current;	/* Battery current from ADC */
	u32 level;		/* formula */
	u32 charging_source;	/* 0: no cable, 1:usb, 2:AC */
	u32 charging_enabled;	/* 0: Disable, 1: Enable */
	u32 batt_health;	/* Battery Health (Authority) */
	u32 batt_is_full;       /* 0 : Not full 1: Full */
	u32 batt_is_recharging; /* 0 : Not recharging 1: Recharging */
	s32 batt_vol_adc_aver;	/* batt vol adc average */
	u32 batt_test_mode;	/* test mode */
	s32 batt_vol_aver;	/* batt vol average */
	s32 batt_temp_aver;	/* batt temp average */
	s32 batt_temp_adc_aver;	/* batt temp adc average */
	s32 batt_v_f_adc;	/* batt V_F adc */
};

struct s3c_battery_info {
	int present;
	int polling;
	unsigned int polling_interval;
	unsigned int device_state;

	struct battery_info bat_info;
#ifdef LPM_MODE
	unsigned int charging_mode_booting;
#endif
};

static enum power_supply_property s3c_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property s3c_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

static struct power_supply s3c_power_supplies[] = {
	{
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = s3c_battery_properties,
		.num_properties = ARRAY_SIZE(s3c_battery_properties),
		.get_property = s3c_bat_get_property,
	},
	{
		.name = "usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
	{
		.name = "ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
};

#define SEC_BATTERY_ATTR(_name)								\
{											\
        .attr = { .name = #_name, .mode = S_IRUGO | S_IWUGO, .owner = THIS_MODULE },	\
        .show = s3c_bat_show_property,							\
        .store = s3c_bat_store,								\
}

static struct device_attribute s3c_battery_attrs[] = {
        SEC_BATTERY_ATTR(batt_vol),
        SEC_BATTERY_ATTR(batt_vol_adc),
        SEC_BATTERY_ATTR(batt_vol_adc_cal),
        SEC_BATTERY_ATTR(batt_temp),
        SEC_BATTERY_ATTR(batt_temp_adc),
        SEC_BATTERY_ATTR(batt_temp_adc_cal),
	SEC_BATTERY_ATTR(batt_vol_adc_aver),
	/* test mode */
	SEC_BATTERY_ATTR(batt_test_mode),
	/* average */
	SEC_BATTERY_ATTR(batt_vol_aver),
	SEC_BATTERY_ATTR(batt_temp_aver),
	SEC_BATTERY_ATTR(batt_temp_adc_aver),
	SEC_BATTERY_ATTR(batt_v_f_adc),
#ifdef __CHECK_CHG_CURRENT__
	SEC_BATTERY_ATTR(batt_chg_current),
#endif /* __CHECK_CHG_CURRENT__ */
	SEC_BATTERY_ATTR(charging_source),
	SEC_BATTERY_ATTR(vibrator),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(mp3),
	SEC_BATTERY_ATTR(video),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(data_call),
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD)	 || (defined CONFIG_S5PC110_FLEMING_BOARD)
	SEC_BATTERY_ATTR(wifi),
	SEC_BATTERY_ATTR(gps),	
#endif	
	SEC_BATTERY_ATTR(device_state),
	SEC_BATTERY_ATTR(batt_compensation),
	SEC_BATTERY_ATTR(is_booting),
	SEC_BATTERY_ATTR(fg_soc),
	SEC_BATTERY_ATTR(reset_soc),
#ifdef LPM_MODE
	SEC_BATTERY_ATTR(charging_mode_booting),
	SEC_BATTERY_ATTR(batt_temp_check),
	SEC_BATTERY_ATTR(batt_full_check),
#endif
};

enum {
        BATT_VOL = 0,
        BATT_VOL_ADC,
        BATT_VOL_ADC_CAL,
        BATT_TEMP,
        BATT_TEMP_ADC,
        BATT_TEMP_ADC_CAL,
	BATT_VOL_ADC_AVER,
	BATT_TEST_MODE,
	BATT_VOL_AVER,
	BATT_TEMP_AVER,
	BATT_TEMP_ADC_AVER,
	BATT_V_F_ADC,
#ifdef __CHECK_CHG_CURRENT__
	BATT_CHG_CURRENT,	
#endif /* __CHECK_CHG_CURRENT__ */
	BATT_CHARGING_SOURCE,
	BATT_VIBRATOR,
	BATT_CAMERA,
	BATT_MP3,
	BATT_VIDEO,
	BATT_VOICE_CALL_2G,
	BATT_VOICE_CALL_3G,
	BATT_DATA_CALL,
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD)	 || (defined CONFIG_S5PC110_FLEMING_BOARD)
	BATT_WIFI,
	BATT_GPS,		
#endif	
	BATT_DEV_STATE,
	BATT_COMPENSATION,
	BATT_BOOTING,
	BATT_FG_SOC,
	BATT_RESET_SOC,
#ifdef LPM_MODE
	CHARGING_MODE_BOOTING,
	BATT_TEMP_CHECK,
	BATT_FULL_CHECK,
#endif
};

struct adc_sample_info {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_TOTAL_COUNT];
	int index;
};
static struct adc_sample_info adc_sample[ENDOFADC];


struct battery_driver 
{
	struct early_suspend	early_suspend;
};
struct battery_driver *battery = NULL;

static int batt_chg_full_1st=0;

/* lock to protect the battery info */
static DEFINE_MUTEX(work_lock);


static struct work_struct bat_work;
static struct device *dev;
static struct timer_list polling_timer;
static int s3c_battery_initial;
static int force_update, force_log;
static int old_level, old_temp, old_is_full, old_is_recharging, old_health, new_temp_level;
static charger_type_t cable_status = CHARGER_BATTERY;


static int batt_max;
static int batt_full;
static int batt_safe_rech;
static int batt_almost;
static int batt_high;
static int batt_medium;
static int batt_low;
static int batt_critical;
static int batt_min;
static int batt_off;
static int batt_compensation;

#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || (defined CONFIG_S5PC110_FLEMING_BOARD)
static int count_check_chg_current;
static int count_check_recharging_bat;
#endif

static unsigned int start_time_msec;
static unsigned int total_time_msec;
static unsigned int end_time_msec;
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD)  || (defined CONFIG_S5PC110_FLEMING_BOARD)
// [junghyunseok edit for CTIA of behold3 20100413
static int event_occur = 0;	
static unsigned int event_start_time_msec;
static unsigned int event_total_time_msec;
#endif

static struct s3c_battery_info s3c_bat_info;

extern charging_device_type curent_device_type;

static int full_charge_flag;

unsigned int is_calling_or_playing = 0;

#ifdef __TEST_DEVICE_DRIVER__
#define SEC_TEST_ATTR(_name)								\
{											\
        .attr = { .name = #_name, .mode = S_IRUGO | S_IWUGO, .owner = THIS_MODULE },	\
        .show = s3c_test_show_property,							\
        .store = s3c_test_store,							\
}

static struct device_attribute s3c_test_attrs[] = {
        SEC_TEST_ATTR(pm),
        SEC_TEST_ATTR(usb),
        SEC_TEST_ATTR(bt_wl),
        SEC_TEST_ATTR(tflash),
        SEC_TEST_ATTR(audio),
        SEC_TEST_ATTR(lcd),
        SEC_TEST_ATTR(suspend_lock),
        SEC_TEST_ATTR(control_tmp),
};

enum {
        TEST_PM = 0,
	USB_OFF,
	BT_WL_OFF,
	TFLASH_OFF,
	AUDIO_OFF,
	LCD_CHECK,
	SUSPEND_LOCK,
	CTRL_TMP,
};

static int s3c_test_create_attrs(struct device * dev)
{
        int i, rc;
        
        for (i = 0; i < ARRAY_SIZE(s3c_test_attrs); i++) {
                rc = device_create_file(dev, &s3c_test_attrs[i]);
                if (rc)
                        goto s3c_attrs_failed;
        }
        goto succeed;
        
s3c_attrs_failed:
        while (i--)
                device_remove_file(dev, &s3c_test_attrs[i]);
succeed:        
        return rc;
}

static void s3c_lcd_check(void)
{
/*
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO6, &reg_buff)) {
		pr_info("%s: VLCD 1.8V (%d)\n", __func__, reg_buff);
	}
	if ((Get_MAX8698_PM_REG(ELDO7, &reg_buff))) {
		pr_info("%s: VLCD 2.8V (%d)\n", __func__, reg_buff);
	}
*/
}

static void s3c_usb_off(void)
{
/*
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO3, &reg_buff)) {
		pr_info("%s: OTGI 1.2V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO3, 0);
	}
	if ((Get_MAX8698_PM_REG(ELDO8, &reg_buff))) {
		pr_info("%s: OTG 3.3V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO8, 0);
	}
*/	
}

static void s3c_bt_wl_off(void)
{
/*
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO4, &reg_buff)) {
		pr_info("%s: BT_WL 2.6V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO4, 0);
	}
*/	
}

static void s3c_tflash_off(void)
{
/*
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("%s: TF 3.0V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 0);
	}
*/	
}

static void s3c_audio_off(void)
{
/*
	pr_info("%s: Turn off audio power, amp\n", __func__);
	amp_enable(0);
	audio_power(0);
*/	
}

static void s3c_test_pm(void)
{
#if 0
	/* PMIC */
	s3c_usb_off();
	s3c_bt_wl_off();
	s3c_tflash_off();
	s3c_lcd_check();

	/* AUDIO */
	s3c_audio_off();

	/* GPIO */
#endif	
}

static ssize_t s3c_test_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
        int i = 0;
        const ptrdiff_t off = attr - s3c_test_attrs;

        switch (off) {
        case TEST_PM:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		s3c_test_pm();
                break;
        case USB_OFF:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 1);
		s3c_usb_off();
                break;
        case BT_WL_OFF:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 2);
		s3c_bt_wl_off();
                break;
        case TFLASH_OFF:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 3);
		s3c_tflash_off();
                break;
        case AUDIO_OFF:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 4);
		s3c_audio_off();
                break;
        case LCD_CHECK:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 5);
		s3c_lcd_check();
                break;
        default:
                i = -EINVAL;
        }       
        
        return i;
}

static ssize_t s3c_test_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int mode = 0;
	int ret = 0;
	const ptrdiff_t off = attr - s3c_test_attrs;

        switch (off) {
        case SUSPEND_LOCK:
		if (sscanf(buf, "%d\n", &mode) == 1) {
			//dev_dbg(dev, "%s: suspend_lock(%d)\n", __func__, mode);
			if (mode) 
                		wake_lock(&wake_lock_for_dev);
			else
                		wake_lock_timeout(
						&wake_lock_for_dev, HZ / 2);
			ret = count;
		}
                break;
	case CTRL_TMP:
		if (sscanf(buf, "%d\n", &mode) == 1) {
			//dev_info(dev, "%s: control tmp(%d)\n", __func__, mode);
			bat_temper_state = mode;
			ret = count;
		}
		break;
        default:
                ret = -EINVAL;
        }       

	return ret;
}
#endif /* __TEST_DEVICE_DRIVER__ */


#ifdef LPM_MODE
void charging_mode_set(unsigned int val)
{
	s3c_bat_info.charging_mode_booting=val;
}
unsigned int charging_mode_get(void)
{
	return s3c_bat_info.charging_mode_booting;
}
#endif


//NAGSM_Android_SEL_Kernel_Aakash_20100503
static int charging_connect;
static int charging_control_overide = 1;
int is_under_at_sleep_cmd = 0;

int get_wakelock_for_AT_SLEEP(void)
{
	wake_lock(&vbus_wake_lock);
}

int release_wakelock_for_AT_SLEEP(void)
{
	wake_lock_timeout(&vbus_wake_lock,HZ * 5);
}

static ssize_t charging_status_show(struct device *dev, struct device_attribute *attr, char *sysfsbuf)
{	
	charging_connect = s3c_bat_info.bat_info.charging_enabled;
	return sprintf(sysfsbuf, "%d\n", charging_connect);
}

static ssize_t charging_con_store(struct device *dev, struct device_attribute *attr,const char *sysfsbuf, size_t size)
{
	sscanf(sysfsbuf, "%d", &charging_connect);

	mutex_lock(&work_lock);		

	printk("+++++ charging_connect = %d +++++\n", charging_connect);
	
	charging_control_overide = charging_connect;
	s3c_set_chg_en(charging_connect);

	release_wakelock_for_AT_SLEEP();	
	is_under_at_sleep_cmd = !charging_control_overide;
	
	mutex_unlock(&work_lock);	

	return size;
}

static DEVICE_ATTR(usbchargingcon, S_IRUGO | S_IWUSR, charging_status_show, charging_con_store);
//NAGSM_Android_SEL_Kernel_Aakash_20100503

static int s3c_bat_get_property(struct power_supply *bat_ps, 
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	//pr_info("[BAT]:%s\n", __func__);

	switch (psp)
	{
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = s3c_bat_get_charging_status();
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = s3c_get_bat_health();
			//pr_info("[BAT]:%s:HEALTH=%d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = s3c_bat_info.present;
			break;
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval = s3c_bat_info.bat_info.level;
			//pr_info("[BAT]:%s:LEVEL=%d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_TEMP:
			val->intval = s3c_bat_info.bat_info.batt_temp;
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int s3c_power_get_property(struct power_supply *bat_ps, 
		enum power_supply_property psp, 
		union power_supply_propval *val)
{
	
	//pr_info("[BAT]:%s\n", __func__);

	switch (psp)
	{
		case POWER_SUPPLY_PROP_ONLINE:
			if (bat_ps->type == POWER_SUPPLY_TYPE_MAINS)
			{
				val->intval = (curent_device_type == PM_CHARGER_TA ? 1 : 0);
			}
			else if (bat_ps->type == POWER_SUPPLY_TYPE_USB)
			{
				val->intval = (curent_device_type == PM_CHARGER_USB_INSERT ? 1 : 0);
			}
			else
			{
				val->intval = 0;
			}
			break;
		default:
			return -EINVAL;
	}
	
	return 0;
}



static ssize_t s3c_bat_show_property(struct device *dev, struct device_attribute *attr, char *buf)
{
        int i = 0;
        const ptrdiff_t off = attr - s3c_battery_attrs;

	//pr_info("[BAT]:%s\n", __func__);

        switch (off)
	{
		case BATT_VOL:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",( s3c_bat_info.bat_info.batt_vol+50));
			break;
		case BATT_VOL_ADC:
			s3c_bat_info.bat_info.batt_vol_adc = 0;
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_vol_adc);
			break;
		case BATT_VOL_ADC_CAL:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_vol_adc_cal);
			break;
		case BATT_TEMP:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_temp);
			break;
		case BATT_TEMP_ADC:
			s3c_bat_info.bat_info.batt_temp_adc = s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_temp_adc);
			break;	
#ifdef __TEST_MODE_INTERFACE__
		case BATT_TEST_MODE:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_test_mode);
			break;
#endif /* __TEST_MODE_INTERFACE__ */
		case BATT_TEMP_ADC_CAL:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_temp_adc_cal);
			break;
		case BATT_VOL_ADC_AVER:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_vol_adc_aver);
			break;
#ifdef __TEST_MODE_INTERFACE__
		case BATT_VOL_AVER:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_vol_aver);
			break;
		case BATT_TEMP_AVER:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_temp_aver);
			break;
		case BATT_TEMP_ADC_AVER:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_temp_adc_aver);
			break;
		case BATT_V_F_ADC:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_v_f_adc);
			break;
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __CHECK_CHG_CURRENT__
		case BATT_CHG_CURRENT:
			s3c_bat_info.bat_info.batt_current = s3c_bat_get_adc_data(S3C_ADC_CHG_CURRENT);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_current);
			break;
#endif /* __CHECK_CHG_CURRENT__ */
		case BATT_CHARGING_SOURCE:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.charging_source);
			break;
#ifdef __BATTERY_COMPENSATION__
		case BATT_DEV_STATE:
			i += scnprintf(buf + i, PAGE_SIZE - i, "0x%08x\n", s3c_bat_info.device_state);
			break;
		case BATT_COMPENSATION:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", batt_compensation);
			break;
#endif /* __BATTERY_COMPENSATION__ */
		case BATT_FG_SOC:
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD)	 || (defined CONFIG_S5PC110_FLEMING_BOARD)
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",  s3c_bat_info.bat_info.level);	
#else				
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",  fg_read_soc());
#endif			
			break;
#ifdef LPM_MODE
		case CHARGING_MODE_BOOTING:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", charging_mode_get());
			break;		
		case BATT_TEMP_CHECK:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_health);
			break;		
		case BATT_FULL_CHECK:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", s3c_bat_info.bat_info.batt_is_full );
			break;			
#endif

		default:
			i = -EINVAL;
        }       
        
	return i;
}


static void s3c_bat_set_compesation(int mode, int offset, int compensate_value)
{

	//pr_info("[BAT]:%s\n", __func__);

	if (mode)
	{
		if (!(s3c_bat_info.device_state & offset))
		{
			s3c_bat_info.device_state |= offset;
			batt_compensation += compensate_value;
		}
	}
	else
	{
		if (s3c_bat_info.device_state & offset)
		{
			s3c_bat_info.device_state &= ~offset;
			batt_compensation -= compensate_value;
		}
	}

	is_calling_or_playing = s3c_bat_info.device_state;
	//pr_info("[BAT]:%s: device_state=0x%x, compensation=%d\n", __func__, s3c_bat_info.device_state, batt_compensation);
	
}


static void s3c_bat_set_vol_cal(int batt_cal)
{
	int max_cal = 4096;

	//pr_info("[BAT]:%s\n", __func__);

	if (!batt_cal)
	{
		return;
	}

	if (batt_cal >= max_cal)
	{
		dev_err(dev, "%s: invalid battery_cal(%d)\n", __func__, batt_cal);
		return;
	}

	batt_max = batt_cal + BATT_MAXIMUM;
	batt_full = batt_cal + BATT_FULL;
	batt_safe_rech = batt_cal + BATT_SAFE_RECHARGE;
	batt_almost = batt_cal + BATT_ALMOST_FULL;
	batt_high = batt_cal + BATT_HIGH;
	batt_medium = batt_cal + BATT_MED;
	batt_low = batt_cal + BATT_LOW;
	batt_critical = batt_cal + BATT_CRITICAL;
	batt_min = batt_cal + BATT_MINIMUM;
	batt_off = batt_cal + BATT_OFF;
}

static int s3c_bat_get_charging_status(void)
{
        charger_type_t charger = CHARGER_BATTERY; 
        int ret = 0;

	//pr_info("[BAT]:%s\n", __func__);
        
        charger = s3c_bat_info.bat_info.charging_source;
        
        switch (charger)
	{
		case CHARGER_BATTERY:
			ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		case CHARGER_USB:
		case CHARGER_AC:
			if (s3c_bat_info.bat_info.batt_is_full)
			{
				ret = POWER_SUPPLY_STATUS_FULL;
			}
			else
			{
				ret = POWER_SUPPLY_STATUS_CHARGING;
			}
			break;
		case CHARGER_DISCHARGE:
			ret = POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		default:
		ret = POWER_SUPPLY_STATUS_UNKNOWN;
        }

        return ret;
}

static ssize_t s3c_bat_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - s3c_battery_attrs;

	//pr_info("[BAT]:%s\n", __func__);

        switch (off)
	{
	        case BATT_VOL_ADC_CAL:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_info.bat_info.batt_vol_adc_cal = x;
				s3c_bat_set_vol_cal(x);
				ret = count;
			}
			//pr_info("[BAT]:%s: batt_vol_adc_cal = %d\n", __func__, x);
			break;
	        case BATT_TEMP_ADC_CAL:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_info.bat_info.batt_temp_adc_cal = x;
				ret = count;
			}
			//pr_info("[BAT]:%s: batt_temp_adc_cal = %d\n", __func__, x);
			break;
#ifdef __TEST_MODE_INTERFACE__
		case BATT_TEST_MODE:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_info.bat_info.batt_test_mode = x;
				ret = count;
			}
			if (s3c_bat_info.bat_info.batt_test_mode)
			{
				s3c_bat_info.polling_interval = POLLING_INTERVAL_TEST;			
/*				if (s3c_bat_info.polling)
				{
					del_timer_sync(&polling_timer);
				}
				mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
				s3c_bat_status_update();
*/			}
			else
			{
				s3c_bat_info.polling_interval = POLLING_INTERVAL;
/*				if (s3c_bat_info.polling)
				{
					del_timer_sync(&polling_timer);
				}
				mod_timer(&polling_timer,jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
				s3c_bat_status_update();
*/			}
			//pr_info("[BAT]:%s: batt_test_mode = %d\n", __func__, x);
			break;
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __BATTERY_COMPENSATION__
		case BATT_VIBRATOR:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_VIBRATOR_ON,	COMPENSATE_VIBRATOR);
				ret = count;
			}
			//pr_info("[BAT]:%s: vibrator = %d\n", __func__, x);
			break;
		case BATT_CAMERA:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_CAMERA_ON, COMPENSATE_CAMERA);
				ret = count;
			}
			//pr_info("[BAT]:%s: camera = %d\n", __func__, x);
			break;
		case BATT_MP3:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_MP3_PLAY,	COMPENSATE_MP3);
				ret = count;
			}
			//pr_info("[BAT]:%s: mp3 = %d\n", __func__, x);
			break;
		case BATT_VIDEO:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_VIDEO_PLAY, COMPENSATE_VIDEO);
				ret = count;
			}
			//pr_info("[BAT]:%s: video = %d\n", __func__, x);
			break;
		case BATT_VOICE_CALL_2G:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_VOICE_CALL_2G, COMPENSATE_VOICE_CALL_2G);
				ret = count;
			}
			//pr_info("[BAT]:%s: voice call 2G = %d\n", __func__, x);
			break;
		case BATT_VOICE_CALL_3G:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_VOICE_CALL_3G, COMPENSATE_VOICE_CALL_3G);
				ret = count;
			}
			//pr_info("[BAT]:%s: voice call 3G = %d\n", __func__, x);
			break;
		case BATT_DATA_CALL:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_DATA_CALL, COMPENSATE_DATA_CALL);
				ret = count;
			}
			//pr_info("[BAT]:%s: data call = %d\n", __func__, x);
			break;
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD)	 || (defined CONFIG_S5PC110_FLEMING_BOARD)		
		case BATT_WIFI:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_WIFI, COMPENSATE_WIFI);
				ret = count;
			}
			//pr_info("[BAT]:%s: wifi = %d\n", __func__, x);
			break;
		case BATT_GPS:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_GPS, COMPENSATE_GPS);
				ret = count;
			}
			//pr_info("[BAT]:%s: gps = %d\n", __func__, x);
			break;						
#endif			
#ifdef COMPENSATE_BOOTING
		case BATT_BOOTING:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				s3c_bat_set_compesation(x, OFFSET_BOOTING, COMPENSATE_BOOTING);
				ret = count;
			}
			//pr_info("[BAT]:%s: boot complete = %d\n", __func__, x);
			break;
#endif /* COMPENSATE_BOOTING */
#endif /* __BATTERY_COMPENSATION__ */
		case BATT_RESET_SOC:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				if (x == 1)
				{
					fg_reset_soc();
				}
				ret = count;
			}
			//pr_info("[BAT]:%s: Reset SOC:%d\n", __func__, x);
			break;
#ifdef LPM_MODE
		case CHARGING_MODE_BOOTING:
			if (sscanf(buf, "%d\n", &x) == 1)
			{
				charging_mode_set(x);
				ret = count;
			}
			//pr_info("[BAT]:%s: CHARGING_MODE_BOOTING:%d\n", __func__, x);
			break;		
#endif
	        default:
	                ret = -EINVAL;
        }       

	return ret;
}


void s3c_bat_set_compensation_for_drv(int mode, int offset)
{

	//pr_info("[BAT]:%s\n", __func__);

	switch(offset)
	{
		case OFFSET_VIBRATOR_ON:
			//pr_info("[BAT]:%s: vibrator = %d\n", __func__, mode);
			s3c_bat_set_compesation(mode, offset, COMPENSATE_VIBRATOR);
			break;
		case OFFSET_LCD_ON:
			//pr_info("[BAT]:%s: LCD On = %d\n", __func__, mode);
			s3c_bat_set_compesation(mode, offset, COMPENSATE_LCD);
			break;
		case OFFSET_CAM_FLASH:
			//pr_info("[BAT]:%s: flash = %d\n", __func__, mode);
			s3c_bat_set_compesation(mode, offset, COMPENSATE_CAM_FALSH);
			break;
		default:
			break;
	}

}

EXPORT_SYMBOL(s3c_bat_set_compensation_for_drv);




void low_battery_power_off(void)
{
	s3c_bat_info.bat_info.level = 0;
	
	schedule_work(&bat_work);
	mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}


static int get_usb_power_state(void)
{

	//pr_info("[BAT]:%s\n", __func__);

	if(curent_device_type==PM_CHARGER_USB_INSERT)
		return 1;
	else
		return 0;
}	


static int s3c_bat_get_adc_data(adc_channel_type adc_ch)
{
	int adc_arr[ADC_DATA_ARR_SIZE];
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i;

	//pr_info("[BAT]:%s\n", __func__);

	for (i = 0; i < ADC_DATA_ARR_SIZE; i++)
	{
		adc_arr[i] = s3c_adc_get_adc_data(adc_ch);
//		pr_info("[BAT]:%s: adc_arr = %d\n", __func__, adc_arr[i]);
		if ( i != 0)
		{
			if (adc_arr[i] > adc_max)
			{
				adc_max = adc_arr[i];
			}
			else if (adc_arr[i] < adc_min)
			{
				adc_min = adc_arr[i];
			}
		}
		else
		{
			adc_max = adc_arr[0];
			adc_min = adc_arr[0];
		}
		adc_total += adc_arr[i];
	}

//	pr_info("[BAT]:%s: adc_max = %d, adc_min = %d\n",	__func__, adc_max, adc_min);
	return (adc_total - adc_max - adc_min) / (ADC_DATA_ARR_SIZE - 2);
}


static unsigned long calculate_average_adc(adc_channel_type channel, int adc)
{
	unsigned int cnt = 0;
	int total_adc = 0;
	int average_adc = 0;
	int index = 0;

	cnt = adc_sample[channel].cnt;
	total_adc = adc_sample[channel].total_adc;

	if (adc < 0 || adc == 0) {
		dev_err(dev, "%s: invalid adc : %d\n", __func__, adc);
		adc = adc_sample[channel].average_adc;
	}

	if( cnt < ADC_TOTAL_COUNT ) {
		adc_sample[channel].adc_arr[cnt] = adc;
		adc_sample[channel].index = cnt;
		adc_sample[channel].cnt = ++cnt;

		total_adc += adc;
		average_adc = total_adc / cnt;
	} else {
		index = adc_sample[channel].index;
		if (++index >= ADC_TOTAL_COUNT)
			index = 0;

		total_adc = (total_adc - adc_sample[channel].adc_arr[index]) + adc;
		average_adc = total_adc / ADC_TOTAL_COUNT;

		adc_sample[channel].adc_arr[index] = adc;
		adc_sample[channel].index = index;
	}

	adc_sample[channel].total_adc = total_adc;
	adc_sample[channel].average_adc = average_adc;

	//pr_info("[BAT]:%s: ch:%d adc=%d, avg_adc=%d\n", __func__, channel, adc, average_adc);
	
	return average_adc;
}

static unsigned long s3c_read_temp(void)
{
	int adc = 0;

	//pr_info("[BAT]:%s\n", __func__);

	adc = s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);

	s3c_bat_info.bat_info.batt_temp_adc = adc;

	return adc;
}

static int is_over_abs_time(void)
{
	unsigned int total_time=0;

	//pr_info("[BAT]:%s\n", __func__);

	if(!start_time_msec)
	{
		return 0;
	}
		
	if (s3c_bat_info.bat_info.batt_is_recharging)
	{
		total_time = TOTAL_RECHARGING_TIME;
	}
	else
	{
		total_time = TOTAL_CHARGING_TIME;
	}

	if(jiffies_to_msecs(jiffies) >= start_time_msec)
	{
		total_time_msec = jiffies_to_msecs(jiffies) - start_time_msec;
	}
	else
	{
		total_time_msec = 0xFFFFFFFF - start_time_msec + jiffies_to_msecs(jiffies);
	}

	if (total_time_msec > total_time && start_time_msec)
	{
		pr_info("[BAT]:%s:abs time is over.:start_time=%u, total_time_msec=%u\n", __func__, start_time_msec, total_time_msec);
		return 1;
	}	
	else
	{
		return 0;
	}
}

#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || (defined CONFIG_S5PC110_FLEMING_BOARD)
static int is_over_event_time(void)
{
	unsigned int total_time=0;

	//pr_info("[BAT]:%s\n", __func__);

	if(!event_start_time_msec)
	{
// [junghyunseok edit for CTIA of behold3 20100413	
		return 1;
	}
		
		total_time = TOTAL_EVENT_TIME;

	if(jiffies_to_msecs(jiffies) >= event_start_time_msec)
	{
		event_total_time_msec = jiffies_to_msecs(jiffies) - event_start_time_msec;
	}
	else
	{
		event_total_time_msec = 0xFFFFFFFF - event_start_time_msec + jiffies_to_msecs(jiffies);
	}

	if (event_total_time_msec > total_time && event_start_time_msec)
	{
		pr_info("[BAT]:%s:abs time is over.:event_start_time_msec=%u, event_total_time_msec=%u\n", __func__, event_start_time_msec, event_total_time_msec);
		return 1;
	}	
	else
	{
		return 0;
	}
}
#endif

#ifdef __CHECK_CHG_CURRENT__

static void check_chg_current(void)
{
	unsigned long chg_current = 0; 

	chg_current = s3c_bat_get_adc_data(S3C_ADC_CHG_CURRENT);
#ifdef BATTERY_DEBUG
	printk("[chg_current] %s:  chg_current = %d , count_check_chg_current = %d\n", __func__, chg_current, count_check_chg_current);
#endif	
	s3c_bat_info.bat_info.batt_current = chg_current;

	if ((!s3c_bat_info.bat_info.batt_is_full)  && (CURRENT_OF_TOPOFF_CHG < chg_current) &&  (chg_current<= CURRENT_OF_FULL_CHG)) 
	{
		count_check_chg_current++;
		if (count_check_chg_current >= 10) {
			dev_info(dev, "%s: battery full\n", __func__);
#ifdef BATTERY_DEBUG
			printk("[FULL Display] %s: battery full, count_check_chg_current = %d\n", __func__, count_check_chg_current);
       		PMIC_dump();
#endif			
			s3c_bat_info.bat_info.batt_is_full = 1;
			force_update = 1;
// [junghyunseok edit to prevent wrong charging disable 20100414
//			full_charge_flag = 1;
			count_check_chg_current = 0;
		}
	}  
// [[junghyunseok edit to remove topoff 20100510
       else if( s3c_bat_info.bat_info.batt_is_full  && (chg_current < CURRENT_OF_TOPOFF_CHG) && (chg_current != 0) )
       {
              count_check_chg_current++;
       	if (count_check_chg_current >= 10) 
			{
#ifdef BATTERY_DEBUG			
				printk("[FULL : CHG CUT OFF] %s: battery full\n", __func__);     
	       		PMIC_dump();
#endif		
// [junghyunseok edit to prevent wrong charging disable 20100414		
				force_update = 1;
				full_charge_flag = 1;
				count_check_chg_current = 0;				
       		}	
       }
	else {
		count_check_chg_current = 0;
	}
}
#endif /* __CHECK_CHG_CURRENT__ */

#ifdef __ADJUST_RECHARGE_ADC__
static void check_recharging_bat(int bat_vol)
{
// [junghyunseok edit to remove too many log 20100414
	//printk("[ %s]   count_check_chg_current = %d\n", __func__,  count_check_recharging_bat);
// [[junghyunseok edit to remove topoff 20100510
	if ( bat_vol < RECHARGE_COND_VOLTAGE) 
	{
// ]]junghyunseok edit to remove topoff 20100510
		if (++count_check_recharging_bat >= 10) {
			dev_info(dev, "%s: recharging\n", __func__);
#ifdef BATTERY_DEBUG			
			printk( "%s: recharging , count_check_recharging_bat = %d\n", __func__, count_check_recharging_bat);
#endif			
			s3c_set_chg_en(1);	
			s3c_bat_info.bat_info.batt_is_recharging = 1;
			full_charge_flag = 0;					
			count_check_recharging_bat = 0;
		}
	} else {
		count_check_recharging_bat = 0;
	}
}
#endif /* __ADJUST_RECHARGE_ADC__ */


static u32 s3c_get_bat_health(void)
{
	//pr_info("[BAT]:%s\n", __func__);

	return s3c_bat_info.bat_info.batt_health;
}

static void s3c_set_bat_health(u32 batt_health)
{
	//pr_info("[BAT]:%s\n", __func__);

	s3c_bat_info.bat_info.batt_health = batt_health;
}

static void s3c_set_time_for_charging(int mode)
{

	//pr_info("[BAT]:%s\n", __func__);

	if (mode)
	{
		/* record start time for abs timer */
		start_time_msec = jiffies_to_msecs(jiffies);
		end_time_msec = 0;
		//pr_info("[BAT]:%s: start_time(%u)\n", __func__, start_time_msec);
	}
	else
	{
		/* initialize start time for abs timer */
		start_time_msec = 0;
		total_time_msec = 0;
		end_time_msec = jiffies_to_msecs(jiffies);
		//pr_info("[BAT]:%s: start_time_msec(%u)\n", __func__, start_time_msec);
	}
}

#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || (defined CONFIG_S5PC110_FLEMING_BOARD)
static void s3c_set_time_for_event(int mode)
{

	//pr_info("[BAT]:%s\n", __func__);

	if (mode)
	{
		/* record start time for abs timer */
		event_start_time_msec = jiffies_to_msecs(jiffies);
		//pr_info("[BAT]:%s: start_time(%u)\n", __func__, event_start_time_msec);
	}
	else
	{
		/* initialize start time for abs timer */
		event_start_time_msec = 0;
		event_total_time_msec = 0;
		//pr_info("[BAT]:%s: start_time_msec(%u)\n", __func__, event_start_time_msec);
	}
}
#endif

static int charging_state;
static void s3c_set_chg_en(int enable)
{

	int chg_en_val;

	//pr_info("[BAT]:%s\n", __func__);

	if(charging_state != enable)
	{
		chg_en_val = maxim_chg_status();

		if (enable && chg_en_val)
		{

			if(curent_device_type==PM_CHARGER_TA)
			{
				maxim_charging_control(PM_CHARGER_TA, TRUE, batt_chg_full_1st);
				s3c_set_time_for_charging(1);
				charging_state=1;
			}
			else if (curent_device_type==PM_CHARGER_USB_INSERT)
			{
				maxim_charging_control(PM_CHARGER_USB_INSERT, TRUE, batt_chg_full_1st);
				s3c_set_time_for_charging(1);
				charging_state=1;
			}
			else
			{
				maxim_charging_control(PM_CHARGER_DEFAULT, FALSE, batt_chg_full_1st);
				s3c_set_time_for_charging(0);
				s3c_bat_info.bat_info.batt_is_recharging = 0;
				charging_state=0;
				//pr_info("[BAT]:%s:unknown charger!!\n", __func__);
			}
		}
		else
		{
			maxim_charging_control(PM_CHARGER_DEFAULT, FALSE, batt_chg_full_1st);
			s3c_set_time_for_charging(0);
			s3c_bat_info.bat_info.batt_is_recharging = 0;
			charging_state = 0;
		}
		s3c_bat_info.bat_info.charging_enabled = charging_state;
		ce_for_fuelgauge = charging_state;
		
		//pr_info("[BAT]:%s: charging_state = %d\n", __func__, charging_state);

	}
	
}

static void s3c_temp_control(int mode)
{

	//pr_info("[BAT]:%s\n", __func__);

	switch (mode)
	{
		case POWER_SUPPLY_HEALTH_GOOD:
			pr_info("[BAT]:%s: GOOD\n", __func__);
			s3c_set_bat_health(mode);
			break;
		case POWER_SUPPLY_HEALTH_OVERHEAT:
			pr_info("[BAT]:%s: OVERHEAT\n", __func__);
			s3c_set_bat_health(mode);
			break;
		case POWER_SUPPLY_HEALTH_FREEZE:
			pr_info("[BAT]:%s: FREEZE\n", __func__);
			s3c_set_bat_health(mode);
			break;
		default:
			break;
	}

	s3c_cable_check_status();
	
}



static void s3c_cable_check_status(void)
{

	//pr_info("[BAT]:%s\n", __func__);


	if (maxim_chg_status())
	{
		if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD)
		{
			pr_info("[BAT]:%s:Unhealth battery state!\n", __func__);
			cable_status = CHARGER_DISCHARGE;
		}
		else if (get_usb_power_state())
		{
			cable_status = CHARGER_USB;
			//pr_info("[BAT]:%s: status : USB\n", __func__);
		}
		else
		{
			cable_status = CHARGER_AC;
			//pr_info("[BAT]:%s: status : AC\n", __func__);
		}

	} 
	else
	{
		cable_status = CHARGER_BATTERY;
		//pr_info("[BAT]:%s: status : BATTERY\n", __func__);
	}

}


void s3c_cable_changed(void)
{
	//pr_info("[BAT]:%s\n", __func__);

	if (!s3c_battery_initial)
	{
		return;
	}

	s3c_bat_info.bat_info.batt_is_full = 0;
	batt_chg_full_1st=0;
	force_log=1;
// [junghyunseok edit for CTIA of behold3 20100413	
	if(s3c_bat_info.bat_info.batt_health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
	s3c_set_bat_health(POWER_SUPPLY_HEALTH_GOOD);	

	s3c_cable_check_status();

	schedule_work(&bat_work);

	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}



void s3c_cable_charging(void)
{

	//pr_info("[BAT]:%s\n", __func__);

	if (!s3c_battery_initial)
	{
		return ;
	}

	if (s3c_bat_info.bat_info.charging_enabled && s3c_get_bat_health() == POWER_SUPPLY_HEALTH_GOOD)
	{
		if(batt_chg_full_1st == 1)
		{
			pr_info("[BAT]:%s:2nd fully charged interrupt.\n",__func__);
			full_charge_flag = 1;			
			force_update = 1;
		}
		else
		{
			pr_info("[BAT]:%s:1st fully charged interrupt.\n",__func__);
			s3c_bat_info.bat_info.batt_is_full = 1;
			batt_chg_full_1st=1;
			force_update = 1;
			maxim_topoff_change();
		}
	}

	schedule_work(&bat_work);
	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}

static charger_type_t old_cable_status;
static int s3c_cable_status_update(void)
{

	//pr_info("[BAT]:%s\n", __func__);


	if(!s3c_battery_initial)
	{
		return -EPERM;
	}

	if(old_cable_status != cable_status)
	{
		
		if (curent_device_type != PM_CHARGER_NULL)
		{
			wake_lock(&vbus_wake_lock);
			//pr_info("[BAT]:%s:suspend wake lock.\n", __func__);
		}
		else
		{
			/* give userspace some time to see the uevent and update
			 * LED state or whatnot...
			 */
			if(!maxim_chg_status())
			{
				wake_lock_timeout(&vbus_wake_lock, 5 * HZ);
				//pr_info("[BAT]:%s:suspend wake unlock.\n", __func__);
			}
		}

		s3c_bat_info.bat_info.charging_source = cable_status;
		old_cable_status = cable_status;
		
	        /* if the power source changes, all power supplies may change state */
		power_supply_changed(&s3c_power_supplies[CHARGER_BATTERY]);

		//pr_info("[BAT]:%s: call power_supply_changed\n", __func__);

	}

	return 0;
	
}

// [[ junghyunseok edit for recharging and full charging 20100414
#ifdef BATTERY_DEBUG
void PMIC_dump(void)
{
    int count,kcount;
    unsigned char reg_buff[0x40]; 	 

	
    for(count = 0; count<0x40 ; count++)
    	{
            reg_buff[count] = 0;
	     pmic_read(0xCC, count, &reg_buff[count], (byte)1);
    	}

     printk(" PMIC dump =========================== \n ");

    for(kcount = 0; kcount < 8 ; kcount++)
    	{
    	     for(count = 0;count < 8 ; count++)
	     printk(" Reg [%x] = %x ", count+(8*kcount), reg_buff[count+(8*kcount)]);
	     printk("  \n ");
			 
    	}
}
#endif
// ]] junghyunseok edit for recharging and full charging 20100414

static int s3c_get_bat_temp(void)
{
	int temp = 0;
	int array_size = 0;
	int i = 0;
	int temp_adc_aver=0;
	int temp_adc = s3c_read_temp();
	int health = s3c_get_bat_health();
	unsigned int ex_case = 0;

	//pr_info("[BAT]:%s\n", __func__);

	temp_adc_aver = calculate_average_adc(S3C_ADC_TEMPERATURE, temp_adc);

	
#ifdef __TEST_DEVICE_DRIVER__
		switch (bat_temper_state) {
		case 0:
			break;
		case 1:
			temp_adc_aver = TEMP_HIGH_BLOCK;
			break;
		case 2:
			temp_adc_aver = TEMP_LOW_BLOCK;
			break;
		default:
			break;
		}
#endif /* __TEST_DEVICE_DRIVER__ */

	s3c_bat_info.bat_info.batt_temp_adc_aver=temp_adc_aver;
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || (defined CONFIG_S5PC110_FLEMING_BOARD)
	ex_case = OFFSET_MP3_PLAY | OFFSET_VOICE_CALL_2G | OFFSET_VOICE_CALL_3G | OFFSET_DATA_CALL | OFFSET_VIDEO_PLAY |
                       OFFSET_CAMERA_ON | OFFSET_WIFI | OFFSET_GPS;
  	if (s3c_bat_info.device_state & ex_case) {
  	  if (temp_adc_aver <= EVENT_TEMP_HIGH_BLOCK) {
  	 	 	if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
  			s3c_temp_control(POWER_SUPPLY_HEALTH_OVERHEAT);
	  } else if (temp_adc_aver >= EVENT_TEMP_HIGH_RECOVER) {
  			if (health == POWER_SUPPLY_HEALTH_OVERHEAT &&
  				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
  			s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);  	
  	  }
	  if(event_occur == 0){	
	    event_occur = 1;
	  }    
		goto __map_temperature__;
	}
       if(event_occur == 1){
	      s3c_set_time_for_event(1);
	      event_occur = 0;
	}
#else					   
	ex_case = OFFSET_MP3_PLAY | OFFSET_VOICE_CALL_2G | OFFSET_VOICE_CALL_3G	| OFFSET_DATA_CALL | OFFSET_VIDEO_PLAY;	
	if (s3c_bat_info.device_state & ex_case)
	{
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT || health == POWER_SUPPLY_HEALTH_FREEZE)
		{
			s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);
			pr_info("[BAT]:%s : temp exception case. : device_state=%d \n", __func__, s3c_bat_info.device_state);
		}
		goto __map_temperature__;
	}
#endif
	if (temp_adc_aver <= TEMP_HIGH_BLOCK)
	{
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD)	 || (defined CONFIG_S5PC110_FLEMING_BOARD)
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT && health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE && is_over_event_time())
#else		
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT && health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)		
#endif		
		{
			s3c_temp_control(POWER_SUPPLY_HEALTH_OVERHEAT);
			s3c_set_time_for_event(0);			
		}
	}
	else if (temp_adc_aver >= TEMP_HIGH_RECOVER && temp_adc_aver <= TEMP_LOW_RECOVER)
	{
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT || health == POWER_SUPPLY_HEALTH_FREEZE)
		{
			s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);
		}
	}
	else if (temp_adc_aver >= TEMP_LOW_BLOCK)
	{
		if (health != POWER_SUPPLY_HEALTH_FREEZE && health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
		{
			s3c_temp_control(POWER_SUPPLY_HEALTH_FREEZE);
		}
	}


__map_temperature__:	
	array_size = ARRAY_SIZE(temper_table);
	for (i = 0; i < (array_size - 1); i++)
	{
		if (i == 0)
		{
			if (temp_adc_aver >= temper_table[0][0])
			{
				temp = temper_table[0][1];
				break;
			}
			else if (temp_adc_aver <= temper_table[array_size-1][0])
			{
				temp = temper_table[array_size-1][1];
				break;
			}
		}

		if (temper_table[i][0] > temp_adc_aver && temper_table[i+1][0] <= temp_adc_aver)
		{
			temp = temper_table[i+1][1];
		}
	}

	//pr_info("[BAT]:%s: temp = %d, temp_adc = %d, temp_adc_aver = %d, batt_health=%d\n",	__func__, temp, temp_adc, temp_adc_aver, s3c_bat_info.bat_info.batt_health);

	s3c_bat_info.bat_info.batt_temp=temp;


	return temp;
}

static int s3c_get_bat_vol(void)
{
	int fg_vcell = -1;

	//pr_info("[BAT]:%s\n", __func__);

	if ((fg_vcell = fg_read_vcell()) < 0)
	{
		dev_err(dev, "%s: Can't read vcell!!!\n", __func__);
		fg_vcell = s3c_bat_info.bat_info.batt_vol;
	}
	else
	{
		s3c_bat_info.bat_info.batt_vol = fg_vcell;
	}

//	pr_info("[BAT]:%s: fg_vcell = %d\n", __func__, fg_vcell);
	
	return fg_vcell;
}


static int s3c_get_bat_level(void)
{
	int fg_soc = -1;

	//pr_info("[BAT]:%s\n", __func__);

	if ((fg_soc = fg_read_soc()) < 0)
	{
		dev_err(dev, "%s: Can't read soc!!!\n", __func__);
		fg_soc = s3c_bat_info.bat_info.level;
	}

// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
	if (low_battery_flag && (fg_soc <= 1))
		fg_soc = 0;  
	else
		low_battery_flag = 0;
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504

	new_temp_level=fg_soc;
	
//	pr_info("[BAT]:%s: fg_soc = %d \n",	__func__, fg_soc);

	return fg_soc;
}

static int s3c_bat_need_recharging(void)
{
	unsigned int  charging_end_total_time=0;
		
	//pr_info("[BAT]:%s\n", __func__);


	if(end_time_msec)
	{
		if(jiffies_to_msecs(jiffies) >= end_time_msec)
		{
			charging_end_total_time = jiffies_to_msecs(jiffies) - end_time_msec;
		}
		else
		{
			charging_end_total_time = 0xFFFFFFFF - end_time_msec + jiffies_to_msecs(jiffies);
		}
	}

//	pr_info("[BAT]:%s:end_time_msec=%d, total_time=%d \n", __func__, end_time_msec, charging_end_total_time);

	if(s3c_bat_info.bat_info.batt_is_full != 1 || s3c_bat_info.bat_info.batt_is_recharging == 1 || s3c_bat_info.bat_info.batt_vol > RECHARGE_COND_VOLTAGE
		|| charging_end_total_time < RECHARGE_COND_TIME)
	{
		//pr_info("[BAT]:%s : not need recharge. \n", __func__);
		return 0;
	}
	else
	{
		//pr_info("[BAT]:%s : need recharge. \n", __func__);
		return 1;
	}

}

static int s3c_bat_is_full_charged(void)
{
	//pr_info("[BAT]:%s\n", __func__);

	if(s3c_bat_info.bat_info.batt_is_full == 1 && full_charge_flag == 1)
	{
		//pr_info("[BAT]:%s : battery is fully charged\n", __func__);
		return 1;
	}
	else
	{
		//pr_info("[BAT]:%s : battery is not fully charged\n", __func__);
		return 0;
	}

}


static void s3c_bat_charging_control(void)
{
	//pr_info("[BAT]:%s\n", __func__);
	

	if(cable_status==CHARGER_BATTERY || cable_status==CHARGER_DISCHARGE)
	{
		s3c_set_chg_en(0);
//		pr_info("[BAT]:%s:no charging.\n", __func__);
	}
#ifndef __ADJUST_RECHARGE_ADC__	
	else if(s3c_bat_need_recharging())
	{
		pr_info("[BAT]:%s:need recharging.\n", __func__);
		s3c_set_chg_en(1);
		s3c_bat_info.bat_info.batt_is_recharging = 1;
		full_charge_flag = 0;
	}
#endif	
	else if(s3c_bat_is_full_charged())
	{
		s3c_set_chg_en(0);
	}
	else if(is_over_abs_time())
	{
		s3c_set_chg_en(0);
		s3c_bat_info.bat_info.batt_is_full = 1;
		full_charge_flag = 1;
	}
	else
	{
		s3c_set_chg_en(1);
	}
	
}

static void s3c_bat_status_update(void)
{

	//pr_info("[BAT]:%s\n", __func__);

	if(s3c_bat_info.bat_info.batt_is_full || s3c_bat_info.bat_info.batt_is_recharging)
	{
		s3c_bat_info.bat_info.level = 100;
	}
	else if (!s3c_bat_info.bat_info.charging_enabled && !s3c_bat_info.bat_info.batt_is_full && new_temp_level > old_level)
	{
		s3c_bat_info.bat_info.level=old_level;
	}
	else
	{
		s3c_bat_info.bat_info.level=new_temp_level;		
	}

// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100506
#if defined(CONFIG_KEPLER_VER_B2)
		if((s3c_bat_info.bat_info.level>=6) && (rcomp_status!=1))
			fuel_gauge_rcomp(FUEL_INT_2ND);
		else if((s3c_bat_info.bat_info.level<5) && (rcomp_status!=2))
			fuel_gauge_rcomp(FUEL_INT_3RD);		
#elif defined(CONFIG_T959_VER_B5)
		if((s3c_bat_info.bat_info.level>=16) && (rcomp_status!=0))
			fuel_gauge_rcomp(FUEL_INT_1ST);
		else if((s3c_bat_info.bat_info.level<15) && (s3c_bat_info.bat_info.level>=6) && (rcomp_status!=1))
			fuel_gauge_rcomp(FUEL_INT_2ND);
		else if((s3c_bat_info.bat_info.level<5) && (rcomp_status!=2))
			fuel_gauge_rcomp(FUEL_INT_3RD);		
#endif
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100506

	if (old_level != s3c_bat_info.bat_info.level || old_temp != s3c_bat_info.bat_info.batt_temp ||	old_is_full != s3c_bat_info.bat_info.batt_is_full
		|| force_update || old_health != s3c_bat_info.bat_info.batt_health)
	{
		//pr_info("[BAT]:update\n");
		force_update = 0;
		power_supply_changed(&s3c_power_supplies[CHARGER_BATTERY]);
	}
// [[ NAGSM_Kernel_HDLNC_junghyunseok edit for adding log about device state 20100513
	if(old_level != s3c_bat_info.bat_info.level || old_is_full != s3c_bat_info.bat_info.batt_is_full || 
		old_is_recharging!= s3c_bat_info.bat_info.batt_is_recharging || old_health != s3c_bat_info.bat_info.batt_health ||force_log==1)
	{
		pr_info("[BAT]:Vol=%d, Temp=%d, SOC=%d, Lv=%d, ST=%u, TT=%u, CS=%d, CE=%d, RC=%d, FC=%d, Hlth=%d, DS=0x%x\n", 
			s3c_bat_info.bat_info.batt_vol, s3c_bat_info.bat_info.batt_temp, new_temp_level, s3c_bat_info.bat_info.level, start_time_msec, total_time_msec, 
			cable_status, s3c_bat_info.bat_info.charging_enabled, s3c_bat_info.bat_info.batt_is_recharging, s3c_bat_info.bat_info.batt_is_full, s3c_bat_info.bat_info.batt_health, s3c_bat_info.device_state);
		force_log=0;
	}
// ]] NAGSM_Kernel_HDLNC_junghyunseok edit for adding log about device state 20100513
}


#if defined(__CHECK_BATTERY_V_F__)
static unsigned int s3c_bat_check_v_f(void)
{
	unsigned int rc = 0;
	int adc = 0;

	//pr_info("[BAT]:%s\n", __func__);
	
	adc = s3c_bat_get_adc_data(S3C_ADC_V_F);

	s3c_bat_info.bat_info.batt_v_f_adc=adc;
// [junghyunseok edit to remove too many log 20100414
#ifdef BATTERY_DEBUG
	pr_info("[BAT]:%s: V_F ADC = %d\n", __func__, adc);
#endif
	if ((adc <= BATT_VF_MAX && adc >= BATT_VF_MIN) ||  FSA9480_Get_JIG_Status())
	{
		rc = 1;
	}
	else
	{
		pr_info("[BAT]:%s: Unauthorized battery!\n", __func__);
		s3c_set_bat_health(POWER_SUPPLY_HEALTH_UNSPEC_FAILURE);
		rc = 0;
	}
	return rc;
}
#elif defined(__PMIC_V_F__)
static unsigned int s3c_bat_check_v_f(void)
{
	unsigned int ret = 0;
	
	if( maxim_vf_status() || FSA9480_Get_JIG_Status() )	// bat detected
	{
		ret = 1;
	}
	else
	{
		dev_err(dev, "%s: VF error!\n", __func__);
		s3c_set_bat_health(POWER_SUPPLY_HEALTH_UNSPEC_FAILURE);
		ret = 0;
	}
	return ret;
}
#endif /* __CHECK_BATTERY_V_F__ */



static void polling_timer_func(unsigned long unused)
{
	//pr_info("[BAT]:%s\n", __func__);

	schedule_work(&bat_work);

	mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}

static void s3c_store_bat_old_data(void)
{
	//pr_info("[BAT]:%s\n", __func__);

	old_temp = s3c_bat_info.bat_info.batt_temp;
	old_level = s3c_bat_info.bat_info.level; 
	old_is_full = s3c_bat_info.bat_info.batt_is_full;
	old_is_recharging = s3c_bat_info.bat_info.batt_is_recharging;
	old_health = s3c_bat_info.bat_info.batt_health;
	
//	pr_info("[BAT]:%s : old_temp=%d, old_level=%d, old_is_full=%d\n", __func__, old_temp, old_level, old_is_full);
}

// [junghyunseok add to clear temp_adc_data 20100503
static void s3c_temp_data_clear(void)
{
	int tmp_erase =0;

	for(tmp_erase =0; tmp_erase < 10; tmp_erase++){
	adc_sample[S3C_ADC_TEMPERATURE].adc_arr[tmp_erase]=0;
		}
	adc_sample[S3C_ADC_TEMPERATURE].average_adc=0;
	adc_sample[S3C_ADC_TEMPERATURE].total_adc=0;
	adc_sample[S3C_ADC_TEMPERATURE].cnt=0;
	adc_sample[S3C_ADC_TEMPERATURE].index=0;
}
// ]junghyunseok add to clear temp_adc_data 20100503

static void s3c_bat_work(struct work_struct *work)
{

	//pr_info("[BAT]:%s\n", __func__);

	if(!s3c_battery_initial)
	{
		return;
	}
	//NAGSM_Android_SEL_Kernel_Aakash_20100503
	if (!charging_control_overide)
	{
		return;	
	}
	//NAGSM_Android_SEL_Kernel_Aakash_20100503
	mutex_lock(&work_lock);

	s3c_store_bat_old_data();
	s3c_get_bat_temp();
	s3c_get_bat_vol();
	s3c_get_bat_level();
// [[junghyunseok edit to remove topoff 20100510	
#ifdef __CHECK_CHG_CURRENT__	
	if ( (s3c_bat_info.bat_info.batt_vol >= FULL_CHARGE_COND_VOLTAGE)  && s3c_bat_info.bat_info.charging_enabled)
		{
			check_chg_current();	
   	 	}
#endif
#ifdef __ADJUST_RECHARGE_ADC__
	if (s3c_bat_info.bat_info.batt_is_full &&  (!s3c_bat_info.bat_info.charging_enabled))
		{
			check_recharging_bat(s3c_bat_info.bat_info.batt_vol);
		}	
#endif
// ]]junghyunseok edit to remove topoff 20100510
	s3c_bat_charging_control();
	s3c_cable_status_update();
#if defined(__CHECK_BATTERY_V_F__) || defined(__PMIC_V_F__)
	if (s3c_bat_check_v_f() == 0)
	{
		// bat_det error -> phone power off?? reset??
// [[ junghyunseok edit for test pass 20100531
		if (curent_device_type != PM_CHARGER_NULL)
		{
			if (pm_power_off)
				pm_power_off();
		}
// ]] junghyunseok edit for test pass 20100531
	}
#endif	
	s3c_bat_status_update();

	mutex_unlock(&work_lock);

	
}

static int s3c_bat_create_attrs(struct device * dev)
{
        int i, rc;

	//pr_info("[BAT]:%s\n", __func__);

        for (i = 0; i < ARRAY_SIZE(s3c_battery_attrs); i++)
	{
                rc = device_create_file(dev, &s3c_battery_attrs[i]);
                if (rc)
                {
                        goto s3c_attrs_failed;
                }
        }
        goto succeed;
        
s3c_attrs_failed:
        while (i--)
                device_remove_file(dev, &s3c_battery_attrs[i]);
succeed:        
        return rc;
}

// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
irqreturn_t low_battery_isr(int irq, void *dev_id)
{
 	schedule_work(&low_bat_work);
 	
	return IRQ_HANDLED;
}

static void s3c_low_bat_work(struct work_struct *work)
{
  int temp_soc;
  wake_lock(&low_battery_wake_lock);
// [[ junghyunseok edit for exception code 20100511
  if ((temp_soc = fg_read_soc()) < 0)
  {
	dev_err(dev, "%s: Can't read soc!!!\n", __func__);
	temp_soc = s3c_bat_info.bat_info.level;
  }
// ]] junghyunseok edit for exception code 20100511  
  printk("[LBW] RCS= %d, SOC= %d\n", rcomp_status, temp_soc);

  switch (rcomp_status){
  	case 0:
		if(temp_soc<=18){
			fuel_gauge_rcomp(FUEL_INT_2ND);
			wake_lock_timeout(&low_battery_wake_lock, HZ * 5);
			}
		else{
			fuel_gauge_rcomp(FUEL_INT_1ST);		
			wake_lock_timeout(&low_battery_wake_lock, HZ * 5);		
			}
		break;
	case 1:
		if(temp_soc<=8){		
			fuel_gauge_rcomp(FUEL_INT_3RD);
			wake_lock_timeout(&low_battery_wake_lock, HZ * 5);
			}
		else{
			fuel_gauge_rcomp(FUEL_INT_2ND);			
			wake_lock_timeout(&low_battery_wake_lock, HZ * 5);		
			}
		break;
	case 2:
		if(temp_soc<=3){			
			low_battery_flag = 1;
			wake_lock_timeout(&low_battery_wake_lock, HZ * 30);		
			}
		else{
			fuel_gauge_rcomp(FUEL_INT_3RD);			
			wake_lock_timeout(&low_battery_wake_lock, HZ * 5);		
			}
		break;
	default:
		dev_err(dev, "%s: wrong rcomp_status value!!!\n", __func__);	// [[ junghyunseok edit for exception code 20100511	
		break;		
  }
}
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504

static void battery_early_suspend(struct early_suspend *h)
{
	u32 con;

	/*hsmmc clock disable*/
	con = readl(S5P_CLKGATE_IP2);
	con &= ~(S5P_CLKGATE_IP2_HSMMC3|S5P_CLKGATE_IP2_HSMMC2|S5P_CLKGATE_IP2_HSMMC1 \
		|S5P_CLKGATE_IP2_HSMMC0);
	writel(con, S5P_CLKGATE_IP2);

	/*g3d clock disable*/
	con = readl(S5P_CLKGATE_IP0);
	con &= ~S5P_CLKGATE_IP0_G3D;
	writel(con, S5P_CLKGATE_IP0);

	/*power gating*/
    s5p_power_gating(S5PC110_POWER_DOMAIN_G3D, DOMAIN_LP_MODE);
    s5p_power_gating(S5PC110_POWER_DOMAIN_MFC, DOMAIN_LP_MODE);
    s5p_power_gating(S5PC110_POWER_DOMAIN_TV, DOMAIN_LP_MODE);
    s5p_power_gating(S5PC110_POWER_DOMAIN_CAM, DOMAIN_LP_MODE);
    s5p_power_gating(S5PC110_POWER_DOMAIN_AUDIO, DOMAIN_LP_MODE);
//	con = readl(S5P_NORMAL_CFG);
//	con &= ~(S5PC110_POWER_DOMAIN_G3D|S5PC110_POWER_DOMAIN_MFC|S5PC110_POWER_DOMAIN_TV \
//		|S5PC110_POWER_DOMAIN_CAM|S5PC110_POWER_DOMAIN_AUDIO);
//	writel(con , S5P_NORMAL_CFG);

	/*usb clock disable*/
//	s3c_usb_cable(0);

	return;
}

static void battery_late_resume(struct early_suspend *h)
{

	return;
}

static ssize_t set_fuel_gauge_read_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int fg_soc = -1;

	if ((fg_soc = fg_read_soc()) < 0)
	{
		fg_soc = s3c_bat_info.bat_info.level;
	}
	else 
	{
		s3c_bat_info.bat_info.level = fg_soc;
	}
	printk("%s: soc is  %d!!!\n", __func__, fg_soc);
	return sprintf(buf, "%d\n", fg_soc);	

}

static ssize_t set_fuel_gauge_reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	int ret_value=0;
	int fg_soc = -1;

	ret_value=fg_reset_soc();	

	if ((fg_soc = fg_read_soc()) < 0)
	{
		fg_soc = s3c_bat_info.bat_info.level;
	}
	else 
	{
		s3c_bat_info.bat_info.level = fg_soc;
	}

	force_update = 1;
	force_log=1;

	printk("Enter set_fuel_gauge_reset_show by AT command return vlaue is %d \n", ret_value);
	ret_value=0;

	return sprintf(buf,"%d\n", ret_value);

}

extern struct class *sec_class;
struct device *fg_atcom_test;
static DEVICE_ATTR(set_fuel_gauage_read, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_fuel_gauge_read_show, NULL);
static DEVICE_ATTR(set_fuel_gauage_reset, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_fuel_gauge_reset_show, NULL);


static int __devinit s3c_bat_probe(struct platform_device *pdev)
{
	int i;
	int ret = 0;
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100506	
	int temp_soc_in_probe;	

	dev = &pdev->dev;
	//pr_info("[BAT]:%s\n", __func__);

	s3c_bat_info.present = 1;
	s3c_bat_info.polling = 1;
	s3c_bat_info.polling_interval = POLLING_INTERVAL;
	s3c_bat_info.device_state = 0;
	s3c_bat_info.bat_info.batt_id = 0;
	s3c_bat_info.bat_info.batt_vol = 0;
	s3c_bat_info.bat_info.batt_vol_adc = 0;
	s3c_bat_info.bat_info.batt_vol_adc_cal = 0;
	s3c_bat_info.bat_info.batt_temp = 0;
	s3c_bat_info.bat_info.batt_temp_adc = 0;
	s3c_bat_info.bat_info.batt_temp_adc_cal = 0;
	s3c_bat_info.bat_info.batt_current = 0;
	s3c_bat_info.bat_info.level = 100;
	s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
	s3c_bat_info.bat_info.charging_enabled = 0;
	s3c_bat_info.bat_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
	s3c_bat_info.bat_info.batt_is_full = 0;
	s3c_bat_info.bat_info.batt_is_recharging= 0;
	s3c_bat_info.bat_info.batt_vol_adc_aver = 0;
	s3c_bat_info.bat_info.batt_test_mode = 0;
	s3c_bat_info.bat_info.batt_vol_aver = 0;
	s3c_bat_info.bat_info.batt_temp_aver = 0;
	s3c_bat_info.bat_info.batt_temp_adc_aver = 0;
	s3c_bat_info.bat_info.batt_v_f_adc = 0;

	ce_for_fuelgauge = 0;
		
	batt_compensation = 0;
// [junghyunseok edit for charging block at the bootting time 20100414
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD)	 || (defined CONFIG_S5PC110_FLEMING_BOARD)
	count_check_chg_current = 0;
	count_check_recharging_bat = 0;
	
	/* initialize start time for abs timer */
	event_start_time_msec = 0;
	event_total_time_msec = 0;
#endif

	old_cable_status = CHARGER_BATTERY;
	charging_state = -1;
// ]junghyunseok edit for charging block at the bootting time 20100414
	INIT_WORK(&bat_work, s3c_bat_work);
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)
	INIT_WORK(&low_bat_work, s3c_low_bat_work);	
#endif
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) 
	{
		ret = power_supply_register(&pdev->dev, &s3c_power_supplies[i]);
		if (ret) 
		{
			dev_err(dev, "Failed to register power supply %d,%d\n", i, ret);
			goto __end__;
		}
	}

	/* create sec detail attributes */
	s3c_bat_create_attrs(s3c_power_supplies[CHARGER_BATTERY].dev);

#ifdef __TEST_DEVICE_DRIVER__
	s3c_test_create_attrs(s3c_power_supplies[CHARGER_AC].dev);
	wake_lock_init(&wake_lock_for_dev, WAKE_LOCK_SUSPEND, "wake_lock_dev");
#endif /* __TEST_DEVICE_DRIVER__ */

	if (s3c_bat_info.polling)
	{
		setup_timer(&polling_timer, polling_timer_func, 0);
		mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
	}

// [[ junghyunseok edit for stepcharging 20100506
      stepcharging_Timer_setup();

	s3c_battery_initial = 1;
	force_update = 0;
	force_log=0;
	full_charge_flag = 0;
	
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100506
// [[ junghyunseok edit for exception code 20100511
	if ((temp_soc_in_probe = fg_read_soc()) < 0)
	{
		dev_err(dev, "%s: Can't read soc!!!\n", __func__);
		temp_soc_in_probe = s3c_bat_info.bat_info.level;
	}
// ]] junghyunseok edit for exception code 20100511	
#if defined(CONFIG_KEPLER_VER_B2) 
	if(temp_soc_in_probe>=6){
		fuel_gauge_rcomp(FUEL_INT_2ND);	
		}
	else{
		fuel_gauge_rcomp(FUEL_INT_3RD);	
		}
#elif defined(CONFIG_T959_VER_B5)
	if(temp_soc_in_probe>=16){
		fuel_gauge_rcomp(FUEL_INT_1ST);
		}
	else if(temp_soc_in_probe>=6){
		fuel_gauge_rcomp(FUEL_INT_2ND);	
		}
	else{
		fuel_gauge_rcomp(FUEL_INT_3RD);	
		}
#else
		fuel_gauge_rcomp(FUEL_INT_3RD);	
#endif
#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)
	    s3c_gpio_cfgpin(GPIO_KBR3, S5PC11X_GPH3_3_EXT_INT33_3);
		s3c_gpio_setpull(GPIO_KBR3, S3C_GPIO_PULL_NONE);

		set_irq_type(LOW_BATTERY_IRQ, IRQ_TYPE_EDGE_FALLING);
		ret = request_irq(LOW_BATTERY_IRQ, low_battery_isr, IRQF_SAMPLE_RANDOM, "low battery irq", (void *) pdev);

		if (ret == 0) {
			printk("low battery interrupt registered \n");
		}
		else {
			printk("request_irq failed\n");
			return;
		}
#endif

	s3c_cable_check_status();

	s3c_bat_work(&bat_work);

	/* Request IRQ */ 
	MAX8998_IRQ_init();

	if(charging_mode_get())
	{
		battery = kzalloc(sizeof(struct battery_driver), GFP_KERNEL);
		battery->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
		battery->early_suspend.suspend = battery_early_suspend;
		battery->early_suspend.resume = battery_late_resume;
		register_early_suspend(&battery->early_suspend);
	}

	fg_atcom_test = device_create(sec_class, NULL, 0, NULL, "fg_atcom_test");
	if (IS_ERR(fg_atcom_test))
	{
		//printk("Failed to create device(fg_atcom_test)!\n");
	}

	if (device_create_file(fg_atcom_test, &dev_attr_set_fuel_gauage_read)< 0)
	{
		//printk("Failed to create device file(%s)!\n", dev_attr_set_fuel_gauage_read.attr.name);
	}
	if (device_create_file(fg_atcom_test, &dev_attr_set_fuel_gauage_reset)< 0)
	{
		//printk("Failed to create device file(%s)!\n", dev_attr_set_fuel_gauage_reset.attr.name);
	}


__end__:
	return ret;
}

#ifdef CONFIG_PM
static int s3c_bat_suspend(struct platform_device *pdev, 
		pm_message_t state)
{
	//pr_info("[BAT]:%s\n", __func__);
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504	
#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)	
#else
	set_low_bat_interrupt(1);
#endif	
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504

	if (s3c_bat_info.polling)
	{
		del_timer_sync(&polling_timer);
	}

	flush_scheduled_work();
	return 0;
}

static int s3c_bat_resume(struct platform_device *pdev)
{
	//pr_info("[BAT]:%s\n", __func__);
	//wake_lock(&vbus_wake_lock);
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504	
#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)	
#else
	set_low_bat_interrupt(0);
#endif	
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504	
// [junghyunseok add to clear temp_adc_data 20100503	
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD)	 || (defined CONFIG_S5PC110_FLEMING_BOARD)
	s3c_temp_data_clear();
#endif
// ]junghyunseok add to clear temp_adc_data 20100503		
	schedule_work(&bat_work);

	if (s3c_bat_info.polling)
	{
		mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
	}
	return 0;
}
#endif /* CONFIG_PM */

static int __devexit s3c_bat_remove(struct platform_device *pdev)
{
	int i;
	//pr_info("[BAT]:%s\n", __func__);

	if (s3c_bat_info.polling)
	{
		del_timer_sync(&polling_timer);
	}

	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) 
	{
		power_supply_unregister(&s3c_power_supplies[i]);
	}
 
	return 0;
}

static struct platform_driver s3c_bat_driver = {
	.driver.name	= DRIVER_NAME,
	.driver.owner	= THIS_MODULE,
	.probe		= s3c_bat_probe,
	.remove		= __devexit_p(s3c_bat_remove),
	.suspend		= s3c_bat_suspend,
	.resume		= s3c_bat_resume,
};


static int __init s3c_bat_init(void)
{
	//pr_info("[BAT]:%s\n", __func__);

	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)
    wake_lock_init(&low_battery_wake_lock, WAKE_LOCK_SUSPEND, "low_battery_wake_lock");
#endif
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
	if (i2c_add_driver(&fg_i2c_driver))
	{
		pr_err("%s: Can't add fg i2c drv\n", __func__);
	}

//NAGSM_Android_SEL_Kernel_Aakash_20100312
//This FD is implemented to control battery charging via DFTA
	battery_class = class_create(THIS_MODULE, "batterychrgcntrl");
	if (IS_ERR(battery_class))
		pr_err("Failed to create class(batterychrgcntrl)!\n");

	s5pc110bat_dev= device_create(battery_class, NULL, 0, NULL, "charging_control");
	if (IS_ERR(s5pc110bat_dev))
		pr_err("Failed to create device(charging_control)!\n");
	if (device_create_file(s5pc110bat_dev, &dev_attr_usbchargingcon) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_usbchargingcon.attr.name);
//NAGSM_Android_SEL_Kernel_Aakash_20100312

	return platform_driver_register(&s3c_bat_driver);
}

static void __exit s3c_bat_exit(void)
{
	//pr_info("[BAT]:%s\n", __func__);
	i2c_del_driver(&fg_i2c_driver);
	platform_driver_unregister(&s3c_bat_driver);
}

late_initcall(s3c_bat_init);
module_exit(s3c_bat_exit);

MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("S3C6410 battery driver");
MODULE_LICENSE("GPL");
