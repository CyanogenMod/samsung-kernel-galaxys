/*
 * linux/drivers/power/s3c6410_battery.h
 *
 * Battery measurement code for S3C6410 platform.
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define DRIVER_NAME	"jupiter-battery"

/*
 * Spica Rev00 board Battery Table
 */
#define BATT_CAL				2447	/* 3.60V */

#define BATT_MAXIMUM			406		/* 4.176V */
#define BATT_FULL				353		/* 4.10V  */
#define BATT_SAFE_RECHARGE	353		/* 4.10V */
#define BATT_ALMOST_FULL		188		/* 3.8641V */		//322	/* 4.066V */
#define BATT_HIGH				112		/* 3.7554V */		//221	/* 3.919V */
#define BATT_MED				66		/* 3.6907V */		//146	/* 3.811V */
#define BATT_LOW				43		/* 3.6566V */		//112	/* 3.763V */
#define BATT_CRITICAL			8		/* 3.6037V */ 	//(74)	/* 3.707V */
#define BATT_MINIMUM			(-28)	/* 3.554V */		//(38)	/* 3.655V */
#define BATT_OFF				(-128)	/* 3.4029V */		//(-103)	/* 3.45V  */

/*
 * AriesQ Rev00 board Temperature Table
 */
#if defined(CONFIG_S5PC110_KEPLER_BOARD)|| defined(CONFIG_S5PC110_FLEMING_BOARD)
const int temper_table[][2] =  {
	/* ADC, Temperature (C) */
	{ 1731,		-100}, // 0
	{ 1711,		-90	}, 
	{ 1691,		-80	}, 
	{ 1671,		-70	}, 
	{ 1654,		-60	}, 
	{ 1653,		-50	}, 
	{ 1633,		-40	}, 
	{ 1613,		-30	}, 
	{ 1592,		-20	}, 
	{ 1569,		-10	}, 
	{ 1567,		0	}, // 10
	{ 1543,		10	}, // +10
	{ 1526,		20	}, 
	{ 1504,		30	}, 
	{ 1484,		40	}, 
	{ 1465,		50	},	
	{ 1442,		60	},
	{ 1419,		70	},
	{ 1396,		80	},
	{ 1373,		90	},
	{ 1350,		100	},
	{ 1327,		110	},
	{ 1304,		120	},
	{ 1281,		130	},
	{ 1258,		140	},
	{ 1235,		150	},
	{ 1212,		160	},
	{ 1189,		170	},
	{ 1166,		180	},
	{ 1143,		190	},
	{ 1120,		200	},
	{ 1097,		210	},
	{ 1074,		220	},
	{ 1051,		230	},
	{ 1028,		240	},
	{ 1005,		250	},
	{ 982,		260	},
	{ 959,		270	},
	{ 936,		280	},
	{ 913,		290	},
	{ 890,		300	},
	{ 867,		310	},
	{ 844,		320	},
	{ 821,		330	},
	{ 798,		340	},
	{ 775,		350	},
	{ 752,		360	},
	{ 729,		370	},
	{ 702,		380	},
	{ 682,		390	},
	{ 665,		400	},
	{ 643,		410	},
	{ 621,		420	},
	{ 602,		430	},
	{ 584,		440	},
	{ 564,		450	},
	{ 544,		460	},
	{ 528,		470	},
	{ 513,		480	},
	{ 495,		490	},
	{ 478,		500	},
	{ 465,		510	},
	{ 453,		520	},
	{ 438,		530	},
	{ 423,		540	},
	{ 408,		550	},
	{ 395,		560	},
	{ 379,		570	},
	{ 364,		580	},
	{ 351,		590	},
	{ 337,		600	},
	{ 323,		610	},
	{ 314,		620	},
	{ 304,		630	},
	{ 290,		640	},
	{ 276,		650	},
	{ 262,		660	},
	{ 248,		670	},
	{ 234,		680	},	
	{ 220,		690	},
	{ 206,		700	},	
};
#elif (defined CONFIG_S5PC110_T959_BOARD)
const int temper_table[][2] =  {
	/* ADC, Temperature (C) */
	{ 1859,		-200}, 
	{ 1846,		-190}, 
	{ 1832,		-180}, 
	{ 1818,		-170}, 
	{ 1804,		-160}, 
	{ 1790,		-150}, 
	{ 1773,		-140}, 
	{ 1756,		-130}, 
	{ 1739,		-120}, 
	{ 1722,		-110}, 
	{ 1705,		-100}, // 10
	{ 1691,		-90 }, 
	{ 1677,		-80 }, 	
	{ 1663,		-70	},
	{ 1649,		-60	},
	{ 1635,		-50	},
	{ 1610,		-40	},
	{ 1586,		-30	},
	{ 1562,		-20	},
	{ 1530,		-10	}, // [[junghyunseok edit temperature table 20100531
	{ 1514,		0	}, // 20
	{ 1497,		10	},
	{ 1479,		20	},
	{ 1465,		30	}, 
	{ 1450,		40	}, // [[junghyunseok edit temperature table 20100531
	{ 1420,		50	}, 
	{ 1406,		60	},
	{ 1386,		70	},
	{ 1366,		80	},
	{ 1346,		90	},
	{ 1326,		100	},
	{ 1302,		110	},
	{ 1278,		120	},
	{ 1254,		130	},
	{ 1230,		140	},
	{ 1206,		150	},
	{ 1182,		160	},
	{ 1158,		170	},
	{ 1134,		180	},
	{ 1110,		190	},
	{ 1086,		200	},
	{ 1059,		210	},
	{ 1035,		220	},
	{ 1011,		230	},
	{ 987,		240	},
	{ 963,		250	},
	{ 937,		260	},
	{ 913,		270	},
	{ 889,		280	},
	{ 865,		290	},
	{ 841,		300	},
	{ 816,		310	},
	{ 794,		320	},
	{ 772,		330	},
	{ 750,		340	},
	{ 728,		350	},
	{ 708,		360	},
	{ 690,		370	},
	{ 672,		380	},
	{ 654,		390	},
	{ 636,		400	},
	{ 616,		410	},
	{ 599,		420	},
	{ 580,		430	}, // [[junghyunseok edit temperature table 20100531
	{ 565,		440	},
	{ 548,		450	},
	{ 529,		460	},
	{ 512,		470	},
	{ 495,		480	},
	{ 478,		490	},
	{ 461,		500	},
	{ 440,		510	}, // [[junghyunseok edit temperature table 20100531
	{ 431,		520	},
	{ 416,		530	},
	{ 401,		540	},
	{ 386,		550	},
	{ 373,		560	},
	{ 360,		570	},
	{ 347,		580	},
	{ 334,		590	},
	{ 321,		600	},
	{ 311,		610	},
	{ 303,		620	},
	{ 296,		630	},
	{ 290 ,		640	}, // [[junghyunseok edit temperature table 20100531
	{ 279,		650	},
	{ 262,		660	},
	{ 248,		670	},
	{ 230,		680	}, // [[junghyunseok edit temperature table 20100531
	{ 220,		690	},
	{ 206,		700	},		
};
#else
const int temper_table[][2] =  {
	/* ADC, Temperature (C) */
	{ 1667,		-70	},
	{ 1658,		-60	},
	{ 1632,		-50	},
	{ 1619,		-40	},
	{ 1614,		-30	},
	{ 1596,		-20	},
	{ 1577,		-10	},
	{ 1559,		0	},
	{ 1536,		10	},
	{ 1513,		20	},
	{ 1491,		30	},
	{ 1468,		40	},
	{ 1445,		50	},
	{ 1421,		60	},
	{ 1396,		70	},
	{ 1372,		80	},
	{ 1348,		90	},
	{ 1324,		100	},
	{ 1299,		110	},
	{ 1275,		120	},
	{ 1251,		130	},
	{ 1226,		140	},
	{ 1202,		150	},
	{ 1178,		160	},
	{ 1155,		170	},
	{ 1131,		180	},
	{ 1108,		190	},
	{ 1084,		200	},
	{ 1060,		210	},
	{ 1037,		220	},
	{ 1013,		230	},
	{ 990,		240	},
	{ 966,		250	},
	{ 943,		260	},
	{ 920,		270	},
	{ 898,		280	},
	{ 875,		290	},
	{ 852,		300	},
	{ 829,		310	},
	{ 806,		320	},
	{ 784,		330	},
	{ 761,		340	},
	{ 738,		350	},
	{ 718,		360	},
	{ 697,		370	},
	{ 677,		380	},
	{ 656,		390	},
	{ 636,		400	},
	{ 615,		410	},
	{ 595,		420	},
	{ 574,		430	},
	{ 554,		440	},
	{ 533,		450	},
	{ 518,		460	},
	{ 503,		470	},
	{ 487,		480	},
	{ 472,		490	},
	{ 457,		500	},
	{ 442,		510	},
	{ 427,		520	},
	{ 411,		530	},
	{ 396,		540	},
	{ 381,		550	},
	{ 368,		560	},
	{ 354,		570	},
	{ 341,		580	},
	{ 324,		590	},
	{ 306,		600	},
	{ 299,		610	},
	{ 293,		620	},
	{ 286,		630	},
	{ 275,		640	},
	{ 264,		650	},
};
#endif

#if defined(CONFIG_S5PC110_KEPLER_BOARD)|| defined(CONFIG_S5PC110_FLEMING_BOARD)
#define EVENT_TEMP_HIGH_BLOCK	temper_table[74][0]		// 64 ¢®E
#define EVENT_TEMP_HIGH_RECOVER	temper_table[67][0]		// 57 ¢®E
// [[ junghyunseok edit for charging block 20100503
#define TEMP_HIGH_BLOCK	temper_table[60][0]		// 50 ¢®E
#define TEMP_HIGH_RECOVER	temper_table[53][0]		// 43 ¢®E
#define TEMP_LOW_BLOCK		temper_table[11][0]		// 1  ¢®E
#define TEMP_LOW_RECOVER	temper_table[16][0]		// 6 ¢®E
// ]] junghyunseok edit for charging block 20100503
#elif (defined CONFIG_S5PC110_T959_BOARD)
// [[junghyunseok edit temperature table 20100531
#define EVENT_TEMP_HIGH_BLOCK	temper_table[88][0]		// 68 ¢®E
#define EVENT_TEMP_HIGH_RECOVER	temper_table[84][0]		// 64 ¢®E
#define TEMP_HIGH_BLOCK	temper_table[71][0]		// 51 ¢®E
#define TEMP_HIGH_RECOVER	temper_table[63][0]		// 43 ¢®E
#define TEMP_LOW_BLOCK		temper_table[19][0]		// -1 temp
#define TEMP_LOW_RECOVER	temper_table[24][0]		// 4 temp
// ]]junghyunseok edit temperature table 20100531
#else
#define TEMP_IDX_ZERO_CELSIUS	7
#define TEMP_HIGH_BLOCK		temper_table[TEMP_IDX_ZERO_CELSIUS+63][0]
#define TEMP_HIGH_RECOVER		temper_table[TEMP_IDX_ZERO_CELSIUS+58][0]
#define TEMP_LOW_BLOCK			temper_table[TEMP_IDX_ZERO_CELSIUS-4][0]
#define TEMP_LOW_RECOVER		temper_table[TEMP_IDX_ZERO_CELSIUS+1][0]
#endif
/*
 * AriesQ Rev00 board ADC channel
 */
typedef enum s3c_adc_channel {
	S3C_ADC_CHG_CURRENT = 2,
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_T959_VER_B0) || (defined CONFIG_T959_VER_B5) || defined(CONFIG_S5PC110_FLEMING_BOARD)	
	S3C_ADC_EAR = 3,
	S3C_ADC_V_F = 4,
	S3C_ADC_TEMPERATURE = 6,
	S3C_ADC_VOLTAGE,
#else
	S3C_ADC_EAR = 3,
	S3C_ADC_TEMPERATURE = 6,
	S3C_ADC_VOLTAGE,
	S3C_ADC_V_F,
#endif	
	ENDOFADC
} adc_channel_type;

#define IRQ_TA_CONNECTED_N	IRQ_EINT(19)
#define IRQ_TA_CHG_N			IRQ_EINT(25)
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#define LOW_BATTERY_IRQ		IRQ_EINT(27)

/******************************************************************************
 * Battery driver features
 * ***************************************************************************/
/* #define __TEMP_ADC_VALUE__ */
/* #define __USE_EGPIO__ */
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_T959_VER_B0) || (defined CONFIG_T959_VER_B5) || defined(CONFIG_S5PC110_FLEMING_BOARD)
#define __CHECK_BATTERY_V_F__
#else
#define __PMIC_V_F__
#endif
// [[ junghyunseok edit for current charging control of T959 20100429
#if defined(CONFIG_KEPLER_VER_B2)||  defined(CONFIG_S5PC110_FLEMING_BOARD) || (defined CONFIG_T959_VER_B5)
#define __CHECK_CHG_CURRENT__
#define __ADJUST_RECHARGE_ADC__
#ifdef __CHECK_CHG_CURRENT__
#define CURRENT_OF_FULL_CHG		279		//150mA
#define CURRENT_OF_TOPOFF_CHG		100		//??mA
#endif
#endif

#define __BATTERY_V_F__
#define __BATTERY_COMPENSATION__
/* #define __CHECK_BOARD_REV__ */
/* #define __BOARD_REV_ADC__ */
#define __TEST_DEVICE_DRIVER__
/* #define __ALWAYS_AWAKE_DEVICE__  */
#define __TEST_MODE_INTERFACE__
#define __FUEL_GAUGES_IC__ 

/*****************************************************************************/
#ifdef __CHECK_BATTERY_V_F__
#define BATT_VF_MAX	750 // 0.55
#define BATT_VF_MIN	450 // 0.45
#endif	

#define TOTAL_CHARGING_TIME	(6*60*60*1000)	/* 6 hours */
#define TOTAL_RECHARGING_TIME	(1*60*60*1000+30*60*1000)	/* 1.5 hours */
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)
#define TOTAL_EVENT_TIME  (40*60*1000)  /* 40 minites */
#endif

#define COMPENSATE_VIBRATOR		19
#define COMPENSATE_CAMERA			25
#define COMPENSATE_MP3				17
#define COMPENSATE_VIDEO			28
#define COMPENSATE_VOICE_CALL_2G	13
#define COMPENSATE_VOICE_CALL_3G	14
#define COMPENSATE_DATA_CALL		25
#define COMPENSATE_LCD				0
#define COMPENSATE_TA				0
#define COMPENSATE_CAM_FALSH		0
#define COMPENSATE_BOOTING		52
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)
#define COMPENSATE_WIFI			0
#define COMPENSATE_GPS				0
#endif

#define SOC_LB_FOR_POWER_OFF		27

#if defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)
#define FULL_CHARGE_COND_VOLTAGE	4100
#define RECHARGE_COND_VOLTAGE		4140
#define RECHARGE_COND_TIME		(30*1000)	/* 30 seconds */
#else
#define FULL_CHARGE_COND_VOLTAGE	4000
#define RECHARGE_COND_VOLTAGE	4130
#define RECHARGE_COND_TIME		(30*1000)	/* 30 seconds */
#endif

#define MIN_SOC		0
#define FULL_SOC	92

// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#define FUEL_INT_1ST	0
#define FUEL_INT_2ND	1
#define FUEL_INT_3RD	2
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504

#define LPM_MODE

#ifdef LPM_MODE
void charging_mode_set(unsigned int val);
unsigned int charging_mode_get(void);
#endif


static int s3c_bat_get_property(struct power_supply *bat_ps, enum power_supply_property psp, union power_supply_propval *val);
static int s3c_power_get_property(struct power_supply *bat_ps,  enum power_supply_property psp, union power_supply_propval *val);
static int s3c_bat_create_attrs(struct device * dev);
static ssize_t s3c_bat_show_property(struct device *dev, struct device_attribute *attr, char *buf);
static void s3c_bat_set_compesation(int mode, int offset, int compensate_value);
static void s3c_bat_set_vol_cal(int batt_cal);
static ssize_t s3c_bat_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
void s3c_bat_set_compensation_for_drv(int mode, int offset);
void low_battery_power_off(void);
static int get_usb_power_state(void);
static int s3c_bat_get_adc_data(adc_channel_type adc_ch);
static unsigned long calculate_average_adc(adc_channel_type channel, int adc);
static unsigned long s3c_read_temp(void);
static int is_over_abs_time(void);

#ifdef __CHECK_CHG_CURRENT__
static void check_chg_current(void);
#endif /* __CHECK_CHG_CURRENT__ */

static u32 s3c_get_bat_health(void);
static void s3c_set_bat_health(u32 batt_health);
static void s3c_set_time_for_charging(int mode);
static void s3c_set_chg_en(int enable);
static void s3c_temp_control(int mode);
static int s3c_bat_get_charging_status(void);
static void s3c_cable_check_status(void);
void s3c_cable_changed(void);
void s3c_cable_charging(void);
static int s3c_cable_status_update(void);
static int s3c_get_bat_temp(void);
static int s3c_get_bat_vol(void);
static int s3c_get_bat_level(void);
static int s3c_bat_need_recharging(void);
static int s3c_bat_is_full_charged(void);
static void s3c_bat_charging_control(void);
static void s3c_bat_status_update(void);

#if defined(__CHECK_BATTERY_V_F__) || defined(__PMIC_V_F__)
static unsigned int s3c_bat_check_v_f(void);
#endif /* __CHECK_BATTERY_V_F__ */
static void polling_timer_func(unsigned long unused);
static void s3c_store_bat_old_data(void);
static void s3c_bat_work(struct work_struct *work);
static void battery_early_suspend(struct early_suspend *h);
static void battery_late_resume(struct early_suspend *h);
static int __devinit s3c_bat_probe(struct platform_device *pdev);

#ifdef CONFIG_PM
static int s3c_bat_suspend(struct platform_device *pdev, pm_message_t state);
static int s3c_bat_resume(struct platform_device *pdev);
#endif /* CONFIG_PM */

static int __devexit s3c_bat_remove(struct platform_device *pdev);
static int __init s3c_bat_init(void);
static void __exit s3c_bat_exit(void);

