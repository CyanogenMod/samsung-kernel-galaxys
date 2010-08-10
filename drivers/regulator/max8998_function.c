// Maxim 8998 Command Module: Interface Window
// Firmware Group
// 06/17/2009  initialize
// (C) 2004 Maxim Integrated Products
//---------------------------------------------------------------------------
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <mach/hardware.h>
#include <mach/gpio-jupiter.h>
#include <plat/regs-clock.h>
#include <mach/gpio.h>
#include <mach/battery.h>
#include <mach/max8998_function.h>
#include <linux/hrtimer.h>
#include <linux/wakelock.h>


#define PMIC_IRQ		IRQ_EINT7
#define	EXT_INT_0_CON	0xe0200e00
#define PMIC_SYS_START	0x0
#define PMIC_SYS_DONE	0x5a5a	
#define PMIC_INT_PEND 	0xE0200F40

extern u8 FSA9480_Get_I2C_USB_Status(void);
irqreturn_t pmic_irq(int irq, void *dev_id);
void MAX8998_PM_IRQ_isr();
void s3c_cable_changed(void);
void s3c_cable_charging(void);
void low_battery_power_off(void);
static enum hrtimer_restart pm_sys_init_done(struct hrtimer *timer);
int pm_get_sys_init_status(void);
void charging_mode_set(unsigned int val);
unsigned int charging_mode_get(void);
unsigned char maxim_lpm_chg_status(void);
void maxim_batt_check(void);

extern u8 MicroTAstatus;
charging_device_type curent_device_type=PM_CHARGER_NULL;
static maxim_rtc_display_type  pm_app_rtc_current_display_mode = MAXIM_RTC_24HR_MODE;
static	void	__iomem		*pmic_int_mem;
struct workqueue_struct *pmic_int_wq;
struct work_struct pmic_int_work;
static struct hrtimer charger_timer;
static struct hrtimer pm_sys_init_timer;
static int pm_sys_start=PMIC_SYS_START;
static	void	__iomem		*pmic_pend_mask_mem;
static struct wake_lock pmic_wake_lock;
static int low_bat1_int_status=0;

extern spinlock_t pmic_access_lock;


// [[ junghyunseok edit for stepcharging workqueue function 20100517
static struct work_struct stepcharging_work;
int stepchargingCount;
byte stepchargingreg_buff[2];
#define CHARGINGSTEP_INTERVAL	50
static struct timer_list chargingstep_timer;
// ]] junghyunseok edit for stepcharging 20100506

max8998_reg_type  max8998pm[ENDOFPM+1] =
{
    /* addr       mask        clear       shift   */
    {  0x00,      0x80,       0x7F,       0x07 }, // PWRONR
    {  0x00,      0x40,       0xBF,       0x06 }, // PWRONF    
    {  0x00,      0x20,       0xDF,       0x05 }, // JIGR    
    {  0x00,      0x10,       0xEF,       0x04 }, // JIGF  
    {  0x00,      0x08,       0xF7,       0x03 }, // DCINR  
    {  0x00,      0x04,       0xFB,       0x02 }, // DCINF  

    {  0x01,      0x08,       0xF7,       0x03 }, // ALARM0
    {  0x01,      0x04,       0xFB,       0x02 }, // ALARM1
    {  0x01,      0x02,       0xFD,       0x01 }, // SMPLEVNT
    {  0x01,      0x01,       0xFE,       0x00 }, // WTSREVNT

    {  0x02,      0x80,       0x7F,       0x07 }, // CHGFAULT
    {  0x02,      0x20,       0xDF,       0x05 }, // DONER
    {  0x02,      0x10,       0xEF,       0x04 }, // CHGRSTF
    {  0x02,      0x08,       0xF7,       0x03 }, // DCINOVPR
    {  0x02,      0x04,       0xFB,       0x02 }, // TOPOFFR
    {  0x02,      0x01,       0xFE,       0x00 }, // ONKEY1S

    {  0x03,      0x02,       0xFD,       0x01 }, // LOBAT2
    {  0x03,      0x01,       0xFE,       0x00 }, // LOBAT1

    {  0x04,      0x80,       0x7F,       0x07 }, // PWRONRM
    {  0x04,      0x40,       0xBF,       0x06 }, // PWRONFM
    {  0x04,      0x20,       0xDF,       0x05 }, // JIGRM
    {  0x04,      0x10,       0xEF,       0x04 }, // JIGFM
    {  0x04,      0x08,       0xF7,       0x03 }, // DCINRM
    {  0x04,      0x04,       0xFB,       0x02 }, // DCINFM

    {  0x05,      0x08,       0xF7,       0x03 }, // ALARM0M
    {  0x05,      0x04,       0xFB,       0x02 }, // ALARM1M
    {  0x05,      0x02,       0xFD,       0x01 }, // SMPLEVNTM
    {  0x05,      0x01,       0xFE,       0x00 }, // WTSREVNTM

    {  0x06,      0x80,       0x7F,       0x07 }, // CHGFAULTM
    {  0x06,      0x20,       0xDF,       0x05 }, // DONERM
    {  0x06,      0x10,       0xEF,       0x04 }, // CHGRSTFM
    {  0x06,      0x08,       0xF7,       0x03 }, // DCINOVPRM
    {  0x06,      0x04,       0xFB,       0x02 }, // TOPOFFRM
    {  0x06,      0x01,       0xFE,       0x00 }, // ONKEY1SM

    {  0x07,      0x02,       0xFD,       0x01 }, // LOBAT2M
    {  0x07,      0x01,       0xFE,       0x00 }, // LOBAT1M

    {  0x08,      0x80,       0x7F,       0x07 }, // PWRON_status
    {  0x08,      0x40,       0xBF,       0x06 }, // JIG_status
    {  0x08,      0x20,       0xDF,       0x05 }, // ALARM0_status
    {  0x08,      0x10,       0xEF,       0x04 }, // ALARM1_status
    {  0x08,      0x08,       0xF7,       0x03 }, // SMPLEVNT_status
    {  0x08,      0x04,       0xFB,       0x02 }, // WTSREVNT_status
    {  0x08,      0x02,       0xFD,       0x01 }, // LOBAT2_status
    {  0x08,      0x01,       0xFE,       0x00 }, // LOBAT1_status

    {  0x09,      0x40,       0xBF,       0x06 }, // DONE_status
    {  0x09,      0x20,       0xDF,       0x05 }, // VDCINOK_status
    {  0x09,      0x10,       0xEF,       0x04 }, // DETBAT_status
    {  0x09,      0x08,       0xF7,       0x03 }, // CHGON_status
    {  0x09,      0x04,       0xFB,       0x02 }, // FCHG_status
    {  0x09,      0x02,       0xFD,       0x01 }, // PQL_status

    {  0x0A,      0x80,       0x7F,       0x07 }, // PWRONM_status
    {  0x0A,      0x40,       0xBF,       0x06 }, // JIGM_status
    {  0x0A,      0x20,       0xDF,       0x05 }, // ALARM0M_status
    {  0x0A,      0x10,       0xEF,       0x04 }, // ALARM1M_status
    {  0x0A,      0x08,       0xF7,       0x03 }, // SMPLEVNTM_status
    {  0x0A,      0x04,       0xFB,       0x02 }, // WTSREVNTM_status
    {  0x0A,      0x02,       0xFD,       0x01 }, // LOBAT2M_status
    {  0x0A,      0x01,       0xFE,       0x00 }, // LOBAT1M_status

    {  0x0B,      0x40,       0xBF,       0x06 }, // DONEM_status
    {  0x0B,      0x20,       0xDF,       0x05 }, // VDCINOKM_status
    {  0x0B,      0x10,       0xEF,       0x04 }, // DETBATM_status
    {  0x0B,      0x08,       0xF7,       0x03 }, // CHGONM_status
    {  0x0B,      0x04,       0xFB,       0x02 }, // FCHGM_status
    {  0x0B,      0x02,       0xFD,       0x01 }, // PQLM_status

    {  0x0C,      0xE0,       0x1F,       0x05 }, // TP
    {  0x0C,      0x18,       0xE7,       0x03 }, // RSTR
    {  0x0C,      0x07,       0xF8,       0x00 }, // ICH

    {  0x0D,      0xC0,       0x3F,       0x06 }, // ESAFEOUT
    {  0x0D,      0x30,       0xCF,       0x04 }, // FT
    {  0x0D,      0x08,       0xF7,       0x03 }, // BATTSL
    {  0x0D,      0x06,       0xF9,       0x01 }, // TMP
    {  0x0D,      0x01,       0xFE,       0x00 }, // CHGENB

    {  0x0E,      0x80,       0x7F,       0x07 }, // LDO2DIS
    {  0x0E,      0x40,       0xBF,       0x06 }, // LDO3DIS
    {  0x0E,      0x20,       0xDF,       0x05 }, // LDO4DIS
    {  0x0E,      0x10,       0xEF,       0x04 }, // LDO5DIS
    {  0x0E,      0x08,       0xF7,       0x03 }, // LDO6DIS
    {  0x0E,      0x04,       0xFB,       0x02 }, // LDO7DIS
    {  0x0E,      0x02,       0xFD,       0x01 }, // LDO8DIS
    {  0x0E,      0x01,       0xFE,       0x00 }, // LDO9DIS

    {  0x0F,      0x80,       0x7F,       0x07 }, // LDO10DIS
    {  0x0F,      0x40,       0xBF,       0x06 }, // LDO11DIS
    {  0x0F,      0x20,       0xDF,       0x05 }, // LDO12DIS
    {  0x0F,      0x10,       0xEF,       0x04 }, // LDO13DIS
    {  0x0F,      0x08,       0xF7,       0x03 }, // LDO14DIS
    {  0x0F,      0x04,       0xFB,       0x02 }, // LDO15DIS
    {  0x0F,      0x02,       0xFD,       0x01 }, // LDO16DIS
    {  0x0F,      0x01,       0xFE,       0x00 }, // LDO17DIS

    {  0x10,      0x20,       0xDF,       0x05 }, // SAFEOUT1DIS
    {  0x10,      0x10,       0xEF,       0x04 }, // SAFEOUT2DIS
    {  0x10,      0x08,       0xF7,       0x03 }, // SD1DIS
    {  0x10,      0x04,       0xFB,       0x02 }, // SD2DIS
    {  0x10,      0x02,       0xFD,       0x01 }, // SD3DIS
    {  0x10,      0x01,       0xFE,       0x00 }, // SD4DIS

    {  0x11,      0x80,       0x7F,       0x07 }, // EN1
    {  0x11,      0x40,       0xBF,       0x06 }, // EN2
    {  0x11,      0x20,       0xDF,       0x05 }, // EN3
    {  0x11,      0x10,       0xEF,       0x04 }, // EN4
    {  0x11,      0x08,       0xF7,       0x03 }, // ENLDO2
    {  0x11,      0x04,       0xFB,       0x02 }, // ENLDO3
    {  0x11,      0x02,       0xFD,       0x01 }, // ENLDO4
    {  0x11,      0x01,       0xFE,       0x00 }, // ENLDO5

    {  0x12,      0x80,       0x7F,       0x07 }, // ELDO6
    {  0x12,      0x40,       0xBF,       0x06 }, // ELDO7
    {  0x12,      0x20,       0xDF,       0x05 }, // ELDO8
    {  0x12,      0x10,       0xEF,       0x04 }, // ELDO9
    {  0x12,      0x08,       0xF7,       0x03 }, // ELDO10
    {  0x12,      0x04,       0xFB,       0x02 }, // ELDO11
    {  0x12,      0x02,       0xFD,       0x01 }, // ELDO12
    {  0x12,      0x01,       0xFE,       0x00 }, // ELDO13

    {  0x13,      0x80,       0x7F,       0x07 }, // ELDO14
    {  0x13,      0x40,       0xBF,       0x06 }, // ELDO15
    {  0x13,      0x20,       0xDF,       0x05 }, // ELDO16
    {  0x13,      0x10,       0xEF,       0x04 }, // ELDO17
    {  0x13,      0x08,       0xF7,       0x03 }, // EPWRHOLD
    {  0x13,      0x04,       0xFB,       0x02 }, // ENBATTMON
    {  0x13,      0x02,       0xFD,       0x01 }, // ELBCNFG2
    {  0x13,      0x01,       0xFE,       0x00 }, // ELBCNFG1

    {  0x14,      0x80,       0x7F,       0x07 }, // EN32KHZAP
    {  0x14,      0x40,       0xBF,       0x06 }, // EN32KHZCP
    {  0x14,      0x20,       0xDF,       0x05 }, // ENVICHG
    {  0x14,      0x0F,       0xF0,       0x00 }, // RAMP

    {  0x15,      0x1F,       0xE0,       0x00 }, // DVSARM1

    {  0x16,      0x1F,       0xE0,       0x00 }, // DVSARM2

    {  0x17,      0x1F,       0xE0,       0x00 }, // DVSARM3

    {  0x18,      0x1F,       0xE0,       0x00 }, // DVSARM4

    {  0x19,      0x1F,       0xE0,       0x00 }, // DVSINT1

    {  0x1A,      0x1F,       0xE0,       0x00 }, // DVSINT2

    {  0x1B,      0x1F,       0xE0,       0x00 }, // BUCK3

    {  0x1C,      0x0F,       0xF0,       0x00 }, // BUCK4

    {  0x1D,      0xF0,       0x0F,       0x04 }, // LDO2
    {  0x1D,      0x0F,       0xF0,       0x00 }, // LDO3

    {  0x1E,      0x1F,       0xE0,       0x00 }, // LDO4

    {  0x1F,      0x1F,       0xE0,       0x00 }, // LDO5

    {  0x20,      0x1F,       0xE0,       0x00 }, // LDO6

    {  0x21,      0x1F,       0xE0,       0x00 }, // LDO7

    {  0x22,      0x70,       0x8F,       0x04 }, // LDO8
    {  0x22,      0x02,       0xFC,       0x00 }, // LDO9

    {  0x23,      0xE0,       0x1F,       0x05 }, // LDO10
    {  0x23,      0x1F,       0xE0,       0x00 }, // LDO11

    {  0x24,      0x1F,       0xE0,       0x00 }, // LDO12

    {  0x25,      0x1F,       0xE0,       0x00 }, // LDO13

    {  0x26,      0x1F,       0xE0,       0x00 }, // LDO14

    {  0x27,      0x1F,       0xE0,       0x00 }, // LDO15

    {  0x28,      0x1F,       0xE0,       0x00 }, // LDO16

    {  0x29,      0x1F,       0xE0,       0x00 }, // LDO17

    {  0x2A,      0x07,       0xF8,       0x00 }, // BKCHR

    {  0x2B,      0x30,       0xCF,       0x04 }, // LBHYST1
    {  0x2B,      0x07,       0xF8,       0x00 }, // LBTH1

    {  0x2C,      0x30,       0xCF,       0x04 }, // LBHYST2
    {  0x2C,      0x07,       0xF8,       0x00 }, // LBTH2

    {  0x3F,      0xF0,       0x0F,       0x04 }, // DASH
    {  0x3F,      0x0F,       0xF0,       0x00 }, // MASKREV
};


max8998_reg_type  max8998rtc[ENDOFRTC] =
{
    /* addr       mask        clear       shift   */
    {  0x00,      0x0F,       0xF0,       0x00 }, // RTC_1SEC,
    {  0x00,      0x70,       0x8F,       0x04 }, // RTC_10SEC,
    {  0x01,      0x0F,       0xF0,       0x00 }, // RTC_1MIN,
    {  0x01,      0x70,       0x8F,       0x04 }, // RTC_10MIN,
    {  0x02,      0x0F,       0xF0,       0x00 }, // RTC_1HR,
    {  0x02,      0x30,       0xCF,       0x04 }, // RTC_10HR_FOR_24HR,
    {  0x02,      0x10,       0xEF,       0x04 }, // RTC_10HR_FOR_12HR,
    {  0x02,      0x20,       0xDF,       0x05 }, // RTC_AM_PM_FOR_12HR,
    {  0x02,      0x80,       0x7F,       0x07 }, // RTC_HR_SELECT,
    {  0x03,      0x07,       0xF8,       0x00 }, // RTC_WEEKDAY,
    {  0x04,      0x0F,       0xF0,       0x00 }, // RTC_1DATE,
    {  0x04,      0x30,       0xCF,       0x04 }, // RTC_10DATE,
    {  0x05,      0x0F,       0xF0,       0x00 }, // RTC_1MONTH,
    {  0x05,      0x10,       0xEF,       0x04 }, // RTC_10MONTH,
    {  0x06,      0x0F,       0xF0,       0x00 }, // RTC_1YEAR,
    {  0x06,      0xF0,       0x0F,       0x04 }, // RTC_10YEAR,
    {  0x07,      0x0F,       0xF0,       0x00 }, // RTC_100YEAR,
    {  0x07,      0xF0,       0x0F,       0x04 }, // RTC_1000YEAR,

    {  0x08,      0x0F,       0xF0,       0x00 }, // ALARM0_1SEC,
    {  0x08,      0x70,       0x8F,       0x04 }, // ALARM0_10SEC,
    {  0x09,      0x0F,       0xF0,       0x00 }, // ALARM0_1MIN,
    {  0x09,      0x70,       0x8F,       0x04 }, // ALARM0_10MIN,
    {  0x0A,      0x0F,       0xF0,       0x00 }, // ALARM0_1HR,
    {  0x0A,      0x30,       0xCF,       0x04 }, // ALARM0_10HR_FOR_24HR,
    {  0x0A,      0x10,       0xEF,       0x04 }, // ALARM0_10HR_FOR_12HR,
    {  0x0A,      0x20,       0xDF,       0x05 }, // ALARM0_AM_PM_FOR_12HR,
    {  0x0A,      0x80,       0x7F,       0x07 }, // ALARM0_HR_SELECT,
    {  0x0B,      0x07,       0xF8,       0x00 }, // ALARM0_WEEKDAY,
    {  0x0C,      0x0F,       0xF0,       0x00 }, // ALARM0_1DATE,
    {  0x0C,      0x30,       0xCF,       0x04 }, // ALARM0_10DATE,
    {  0x0D,      0x0F,       0xF0,       0x00 }, // ALARM0_1MONTH,
    {  0x0D,      0x10,       0xEF,       0x04 }, // ALARM0_10MONTH,
    {  0x0E,      0x0F,       0xF0,       0x00 }, // ALARM0_1YEAR,
    {  0x0E,      0xF0,       0x0F,       0x04 }, // ALARM0_10YEAR,
    {  0x0F,      0x0F,       0xF0,       0x00 }, // ALARM0_100YEAR,
    {  0x0F,      0xF0,       0x0F,       0x04 }, // ALARM0_1000YEAR,

    {  0x10,      0x0F,       0xF0,       0x00 }, // ALARM1_1SEC,
    {  0x10,      0x70,       0x8F,       0x04 }, // ALARM1_10SEC,
    {  0x11,      0x0F,       0xF0,       0x00 }, // ALARM1_1MIN,
    {  0x11,      0x70,       0x8F,       0x04 }, // ALARM1_10MIN,
    {  0x12,      0x0F,       0xF0,       0x00 }, // ALARM1_1HR,
    {  0x12,      0x30,       0xCF,       0x04 }, // ALARM1_10HR_FOR_24HR,
    {  0x12,      0x10,       0xEF,       0x04 }, // ALARM1_10HR_FOR_12HR,
    {  0x12,      0x20,       0xDF,       0x05 }, // ALARM1_AM_PM_FOR_12HR,
    {  0x12,      0x80,       0x7F,       0x07 }, // ALARM1_HR_SELECT,
    {  0x13,      0x07,       0xF8,       0x00 }, // ALARM1_WEEKDAY,
    {  0x14,      0x0F,       0xF0,       0x00 }, // ALARM1_1DATE,
    {  0x14,      0x30,       0xCF,       0x04 }, // ALARM1_10DATE,
    {  0x15,      0x0F,       0xF0,       0x00 }, // ALARM1_1MONTH,
    {  0x15,      0x10,       0xEF,       0x04 }, // ALARM1_10MONTH,
    {  0x16,      0x0F,       0xF0,       0x00 }, // ALARM1_1YEAR,
    {  0x16,      0xF0,       0x0F,       0x04 }, // ALARM1_10YEAR,
    {  0x17,      0x0F,       0xF0,       0x00 }, // ALARM1_100YEAR,
    {  0x17,      0xF0,       0x0F,       0x04 }, // ALARM1_1000YEAR,

    {  0x18,      0x80,       0x7F,       0x07 }, // ALARM0_ONESEC,
    {  0x18,      0x40,       0xBF,       0x06 }, // ALARM0_YEARS,
    {  0x18,      0x20,       0xDF,       0x05 }, // ALARM0_MONTH,
    {  0x18,      0x10,       0xEF,       0x04 }, // ALARM0_DATE,
    {  0x18,      0x08,       0xF7,       0x03 }, // ALARM0_DAY,
    {  0x18,      0x04,       0xFB,       0x02 }, // ALARM0_HOUR,
    {  0x18,      0x02,       0xFD,       0x01 }, // ALARM0_MIN,
    {  0x18,      0x01,       0xFE,       0x00 }, // ALARM0_SEC,

    {  0x19,      0x80,       0x7F,       0x07 }, // ALARM1_ONESEC,
    {  0x19,      0x40,       0xBF,       0x06 }, // ALARM1_YEARS,
    {  0x19,      0x20,       0xDF,       0x05 }, // ALARM1_MONTH,
    {  0x19,      0x10,       0xEF,       0x04 }, // ALARM1_DATE,
    {  0x19,      0x08,       0xF7,       0x03 }, // ALARM1_DAY,
    {  0x19,      0x04,       0xFB,       0x02 }, // ALARM1_HOUR,
    {  0x19,      0x02,       0xFD,       0x01 }, // ALARM1_MIN,
    {  0x19,      0x01,       0xFE,       0x00 }, // ALARM1_SEC,

    {  0x1A,      0x40,       0xBF,       0x06 }, // RTC_STATUS_LEAP_OK,      
    {  0x1A,      0x20,       0xDF,       0x05 }, // RTC_STATUS_MON_OK,       
    {  0x1A,      0x10,       0xEF,       0x04 }, // RTC_STATUS_CARY_OK,      
    {  0x1A,      0x08,       0xF7,       0x03 }, // RTC_STATUS_REG_OK,       
    {  0x1A,      0x04,       0xFB,       0x02 }, // RTC_STATUS_ALARM1,       
    {  0x1A,      0x02,       0xFD,       0x01 }, // RTC_STATUS_ALARM0,       
    {  0x1A,      0x01,       0xFE,       0x00 }, // RTC_STATUS_ATAL_FLT,     
                                                                              
    {  0x1B,      0x80,       0x7F,       0x07 }, // WTSR_SMPL_CNTL_EN_SMPL,  
    {  0x1B,      0x40,       0xBF,       0x06 }, // WTSR_SMPL_CNTL_EN_WTSR,  
    {  0x1B,      0x0C,       0xF3,       0x02 }, // WTSR_SMPL_CNTL_TIME_SMPL,
    {  0x1B,      0x03,       0xFC,       0x00 }, // WTSR_SMPL_CNTL_TIME_WTSR,
};

#ifndef BOOTLOADER

void maxim_pwr_press(void);
void maxim_pwr_release(void);
void maxim_vac_connect(void);
void maxim_vac_disconnect(void);
void maxim_charging_topoff(void);
void maxim_low_battery_2nd(void); // 3.3v
void maxim_low_battery_1st(void); // 3.57V

max8998_irq_table_type max8998_irq_table[ENDOFIRQ+1] = {
    maxim_pwr_press, // PWRONR
    maxim_pwr_release, // PWRONF
    NULL, // JIGR
    NULL, // JIGF
    maxim_vac_connect, // DCINR
    maxim_vac_disconnect, // DCINF
    NULL, // ALARM0
    NULL, // ALARM1
    NULL, // SMPLEVNT
    NULL, // WTSREVNT
    NULL, // CHGFAULT
    NULL, // DONER
    NULL, // CHGRSTF
    NULL, // DCINOVPR
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)
    NULL, // TOPOFFR
    NULL, // ONKEY1S
    NULL, // LOBAT2
    NULL// LOBAT1   
#else
    maxim_charging_topoff, // TOPOFFR
    NULL, // ONKEY1S
    maxim_low_battery_2nd, // LOBAT2
    maxim_low_battery_1st  // LOBAT1
#endif
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
};
#endif

const byte buck4_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    BUCK4_0p800,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    BUCK4_0p900,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    BUCK4_1p000,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    BUCK4_1p100,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    BUCK4_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    BUCK4_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    BUCK4_1p400,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    BUCK4_1p500,   // VCC_1p500,
    BUCK4_1p600,   // VCC_1p600,
    BUCK4_1p700,   // VCC_1p700,
    BUCK4_1p800,   // VCC_1p800,
    BUCK4_1p900,   // VCC_1p900,
    BUCK4_2p000,   // VCC_2p000,
    BUCK4_2p100,   // VCC_2p100,
    BUCK4_2p200,   // VCC_2p200,
    BUCK4_2p300,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    0xFF,    // VCC_2p800,
    0xFF,    // VCC_2p900,
    0xFF,    // VCC_3p000,
    0xFF,    // VCC_3p100,
    0xFF,    // VCC_3p200,
    0xFF,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo2_3_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    LDO2_3_0p800,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    LDO2_3_0p850,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    LDO2_3_0p900,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    LDO2_3_0p950,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    LDO2_3_1p000,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    LDO2_3_1p050,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    LDO2_3_1p100,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    LDO2_3_1p150,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    LDO2_3_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    LDO2_3_1p250,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    LDO2_3_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    0xFF,   // VCC_1p600,
    0xFF,   // VCC_1p700,
    0xFF,   // VCC_1p800,
    0xFF,   // VCC_1p900,
    0xFF,   // VCC_2p000,
    0xFF,   // VCC_2p100,
    0xFF,   // VCC_2p200,
    0xFF,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    0xFF,    // VCC_2p800,
    0xFF,    // VCC_2p900,
    0xFF,    // VCC_3p000,
    0xFF,    // VCC_3p100,
    0xFF,    // VCC_3p200,
    0xFF,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte buck3_ldo4to7_11_17_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    0xFF,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    0xFF,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    BUCK3_LDO4TO7_11_17_1p600,   // VCC_1p600,
    BUCK3_LDO4TO7_11_17_1p700,   // VCC_1p700,
    BUCK3_LDO4TO7_11_17_1p800,   // VCC_1p800,
    BUCK3_LDO4TO7_11_17_1p900,   // VCC_1p900,
    BUCK3_LDO4TO7_11_17_2p000,   // VCC_2p000,
    BUCK3_LDO4TO7_11_17_2p100,   // VCC_2p100,
    BUCK3_LDO4TO7_11_17_2p200,   // VCC_2p200,
    BUCK3_LDO4TO7_11_17_2p300,   // VCC_2p300,
    BUCK3_LDO4TO7_11_17_2p400,    // VCC_2p400,
    BUCK3_LDO4TO7_11_17_2p500,    // VCC_2p500,
    BUCK3_LDO4TO7_11_17_2p600,    // VCC_2p600,
    BUCK3_LDO4TO7_11_17_2p700,    // VCC_2p700,
    BUCK3_LDO4TO7_11_17_2p800,    // VCC_2p800,
    BUCK3_LDO4TO7_11_17_2p900,    // VCC_2p900,
    BUCK3_LDO4TO7_11_17_3p000,    // VCC_3p000,
    BUCK3_LDO4TO7_11_17_3p100,    // VCC_3p100,
    BUCK3_LDO4TO7_11_17_3p200,    // VCC_3p200,
    BUCK3_LDO4TO7_11_17_3p300,    // VCC_3p300,
    BUCK3_LDO4TO7_11_17_3p400,    // VCC_3p400,
    BUCK3_LDO4TO7_11_17_3p500,    // VCC_3p500,
    BUCK3_LDO4TO7_11_17_3p600    // VCC_3p600,
};

const byte ldo8_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    0xFF,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    0xFF,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    0xFF,   // VCC_1p600,
    0xFF,   // VCC_1p700,
    0xFF,   // VCC_1p800,
    0xFF,   // VCC_1p900,
    0xFF,   // VCC_2p000,
    0xFF,   // VCC_2p100,
    0xFF,   // VCC_2p200,
    0xFF,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    0xFF,    // VCC_2p800,
    0xFF,    // VCC_2p900,
    LDO8_3p000,    // VCC_3p000,
    LDO8_3p100,    // VCC_3p100,
    LDO8_3p200,    // VCC_3p200,
    LDO8_3p300,    // VCC_3p300,
    LDO8_3p400,    // VCC_3p400,
    LDO8_3p500,    // VCC_3p500,
    LDO8_3p600    // VCC_3p600,
};

const byte ldo9_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    0xFF,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    0xFF,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    0xFF,   // VCC_1p600,
    0xFF,   // VCC_1p700,
    0xFF,   // VCC_1p800,
    0xFF,   // VCC_1p900,
    0xFF,   // VCC_2p000,
    0xFF,   // VCC_2p100,
    0xFF,   // VCC_2p200,
    0xFF,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    LDO9_2p800,    // VCC_2p800,
    LDO9_2p900,    // VCC_2p900,
    LDO9_3p000,    // VCC_3p000,
    LDO9_3p100,    // VCC_3p100,
    0xFF,    // VCC_3p200,
    0xFF,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo10_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    LDO10_0p950,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    LDO10_1p000,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    LDO10_1p050,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    LDO10_1p100,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    LDO10_1p150,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    LDO10_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    LDO10_1p250,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    LDO10_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    0xFF,   // VCC_1p600,
    0xFF,   // VCC_1p700,
    0xFF,   // VCC_1p800,
    0xFF,   // VCC_1p900,
    0xFF,   // VCC_2p000,
    0xFF,   // VCC_2p100,
    0xFF,   // VCC_2p200,
    0xFF,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    0xFF,    // VCC_2p800,
    0xFF,    // VCC_2p900,
    0xFF,    // VCC_3p000,
    0xFF,    // VCC_3p100,
    0xFF,    // VCC_3p200,
    0xFF,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo12_13_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    LDO12_13_0p800,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    LDO12_13_0p900,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    LDO12_13_1p000,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    LDO12_13_1p100,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    LDO12_13_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    LDO12_13_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    LDO12_13_1p400,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    LDO12_13_1p500,   // VCC_1p500,
    LDO12_13_1p600,   // VCC_1p600,
    LDO12_13_1p700,   // VCC_1p700,
    LDO12_13_1p800,   // VCC_1p800,
    LDO12_13_1p900,   // VCC_1p900,
    LDO12_13_2p000,   // VCC_2p000,
    LDO12_13_2p100,   // VCC_2p100,
    LDO12_13_2p200,   // VCC_2p200,
    LDO12_13_2p300,   // VCC_2p300,
    LDO12_13_2p400,    // VCC_2p400,
    LDO12_13_2p500,    // VCC_2p500,
    LDO12_13_2p600,    // VCC_2p600,
    LDO12_13_2p700,    // VCC_2p700,
    LDO12_13_2p800,    // VCC_2p800,
    LDO12_13_2p900,    // VCC_2p900,
    LDO12_13_3p000,    // VCC_3p000,
    LDO12_13_3p100,    // VCC_3p100,
    LDO12_13_3p200,    // VCC_3p200,
    LDO12_13_3p300,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo14_15_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    LDO14_15_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    LDO14_15_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    LDO14_15_1p400,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    LDO14_15_1p500,   // VCC_1p500,
    LDO14_15_1p600,   // VCC_1p600,
    LDO14_15_1p700,   // VCC_1p700,
    LDO14_15_1p800,   // VCC_1p800,
    LDO14_15_1p900,   // VCC_1p900,
    LDO14_15_2p000,   // VCC_2p000,
    LDO14_15_2p100,   // VCC_2p100,
    LDO14_15_2p200,   // VCC_2p200,
    LDO14_15_2p300,   // VCC_2p300,
    LDO14_15_2p400,    // VCC_2p400,
    LDO14_15_2p500,    // VCC_2p500,
    LDO14_15_2p600,    // VCC_2p600,
    LDO14_15_2p700,    // VCC_2p700,
    LDO14_15_2p800,    // VCC_2p800,
    LDO14_15_2p900,    // VCC_2p900,
    LDO14_15_3p000,    // VCC_3p000,
    LDO14_15_3p100,    // VCC_3p100,
    LDO14_15_3p200,    // VCC_3p200,
    LDO14_15_3p300,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo16_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    0xFF,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    0xFF,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    LDO16_1p600,   // VCC_1p600,
    LDO16_1p700,   // VCC_1p700,
    LDO16_1p800,   // VCC_1p800,
    LDO16_1p900,   // VCC_1p900,
    LDO16_2p000,   // VCC_2p000,
    LDO16_2p100,   // VCC_2p100,
    LDO16_2p200,   // VCC_2p200,
    LDO16_2p300,   // VCC_2p300,
    LDO16_2p400,    // VCC_2p400,
    LDO16_2p500,    // VCC_2p500,
    LDO16_2p600,    // VCC_2p600,
    LDO16_2p700,    // VCC_2p700,
    LDO16_2p800,    // VCC_2p800,
    LDO16_2p900,    // VCC_2p900,
    LDO16_3p000,    // VCC_3p000,
    LDO16_3p100,    // VCC_3p100,
    LDO16_3p200,    // VCC_3p200,
    LDO16_3p300,    // VCC_3p300,
    LDO16_3p400,    // VCC_3p400,
    LDO16_3p500,    // VCC_3p500,
    LDO16_3p600    // VCC_3p600,
};


/*===========================================================================

      P O W E R     M A N A G E M E N T     S E C T I O N

===========================================================================*/
/*===========================================================================

FUNCTION pmic_read                                

DESCRIPTION
    It does the following: When we need to read a specific register in Power management section, This is used
INPUT PARAMETERS
    byte slave : slave address
 	byte reg : Register address 
 	byte data : data 
 	byte length : the number of the register 
RETURN VALUE
	PMIC_PASS : Operation was successful
	PMIC_FAIL : Read operation failed
DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
	pmic_read(SLAVE_ADDR_PM, IRQ1_ADDR, &irq1_reg);
===========================================================================*/
pmic_status_type pmic_read(byte slave, byte reg, byte *data, byte length)
{
	int i=0;
	int ret;

	if(1)//!pm_get_sys_init_status())
	{
		if(length>I2C_SMBUS_BLOCK_MAX)
		{
			printk("read fail: try to read more than 32 byte \n");
			return (PMIC_FAIL);
		}

		ret = i2c_smbus_read_i2c_block_data(max8998_client, reg, length, data);

		if (ret != length){
			printk("read fail: read length unmatch \n");
			return (PMIC_FAIL);
		}

	}
	else
	{
		do
		{
			data[i] = i2c_smbus_read_byte_data(max8998_client, reg);
			msleep(5);
			if (data[i] < 0)
			return (PMIC_FAIL);
			reg++;
			i++;
			length--;
		}while(length>0);	
	}

	return (PMIC_PASS);
}

/*===========================================================================

FUNCTION pmic_write                                

DESCRIPTION
    It does the following: When we need to write a specific register in Power management section, This is used.
INPUT PARAMETERS
    byte slave : slave address
 	byte reg : Register address 
 	byte data : data 
 	byte length : the number of the register 
RETURN VALUE
	PMIC_PASS : Operation was successful
	PMIC_FAIL : Write operation failed
DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
	reg = ONOFF1_ADDR;
	pmic_onoff_buf[i] = (( pmic_onoff_buf[i] & ~(mask))|(mask & data));
	
	if (pmic_write(SLAVE_ADDR_PM, reg, &pmic_onoff_buf[i]) != PMIC_PASS) {
		MSG_HIGH("Write Vreg control failed, reg 0x%x", reg, 0, 0);
	}
===========================================================================*/
pmic_status_type pmic_write(byte slave, byte reg, byte *data, byte length)
{
	int ret;
	int i=0;
	extern u8 max8998_cache_regs[];

	if(1)//!pm_get_sys_init_status())
	{
		if(length>I2C_SMBUS_BLOCK_MAX)
		{
			printk("read fail: try to read more than 32 byte \n");
			return (PMIC_FAIL);
		}

		ret = i2c_smbus_write_i2c_block_data(max8998_client, reg, length, data);

		if (ret != 0){
			printk("write fail \n");
			return (PMIC_FAIL);
		}
	}
	else
	{
		do
		{
			ret = i2c_smbus_write_byte_data(max8998_client, reg, data[i]);
			msleep(5);
			if (ret != 0)
				return (PMIC_FAIL);
			reg++;
			i++;
			length--;
		}while(length>0);
	}

	do
	{
		max8998_cache_regs[reg] = data[i];
		reg++;
		i++;
		length--;
	}while(length>0);	

	return (PMIC_PASS);
}

/*===========================================================================

FUNCTION Set_MAX8998_PM_REG                                

DESCRIPTION
    This function write the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    value   :   the value for reg_num.

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8998_PM_REG(CHGENB, onoff);

===========================================================================*/
boolean Set_MAX8998_PM_REG(max8998_pm_section_type reg_num, byte value)
{
    byte reg_buff;

    if(reg_num >= ENDOFPM)
    {
        // Error - Invalid register
        printk("Set_MAX8998_PM_REG Invalid register\n");
	 return FALSE;
    }

    spin_lock(&pmic_access_lock);

    if(pmic_read(SLAVE_ADDR_PM, max8998pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Error - I2C read error
        spin_unlock(&pmic_access_lock);
        printk("Set_MAX8998_PM_REG Read failed\n");
	return FALSE; // return error
    }

    reg_buff = (reg_buff & max8998pm[reg_num].clear) | (value << max8998pm[reg_num].shift);
    if(pmic_write(SLAVE_ADDR_PM, max8998pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Error - I2C write error
        spin_unlock(&pmic_access_lock);
        printk("Set_MAX8998_PM_REG Write failed\n");
        return FALSE;
    }

    spin_unlock(&pmic_access_lock);
    return TRUE;
}
/*===========================================================================

FUNCTION Get_MAX8998_PM_REG                                

DESCRIPTION
    This function read the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.

RETURN VALUE
    reg_buff :  the value of selected register.
                reg_buff is aligned to the right side of the return value.

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
byte Get_MAX8998_PM_REG(max8998_pm_section_type reg_num)
{
    byte reg_buff;

    if(pmic_read(SLAVE_ADDR_PM, max8998pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Error - I2C read error
        return (byte)(-1); // return error
    }

    reg_buff = (reg_buff & max8998pm[reg_num].mask) >> max8998pm[reg_num].shift;
    return reg_buff;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_ADDR                                

DESCRIPTION
    This function write the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    byte reg_addr    : the register address.
    byte *reg_buff   : the array for data of register to write.
 	byte length      : the number of the register 

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_ADDR(byte reg_addr, byte *reg_buff, byte length)
{
    if(reg_addr > ONOFF4_REG)
    {
		// Error - Invalid Write Register
		return FALSE;
    }

    spin_lock(&pmic_access_lock);
	
    if(pmic_write(SLAVE_ADDR_PM, (byte)reg_addr, reg_buff, length) != PMIC_PASS)
    {
        // Error - I2C write error
	       spin_unlock(&pmic_access_lock);        
		return FALSE;
    }

    spin_unlock(&pmic_access_lock);
    return TRUE;
}

/*===========================================================================

FUNCTION Get_MAX8998_PM_ADDR                                

DESCRIPTION
    This function read the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    byte reg_addr   : the register address.
    byte *reg_buff  : the array for data of register to write.
 	byte length     : the number of the register 

RETURN VALUE
    byte *reg_buff : the pointer parameter for data of sequential registers
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8998_PM_ADDR(byte reg_addr, byte *reg_buff, byte length)
{
    if(reg_addr > ONOFF4_REG)
    {
        // Error - Invalid Write Register
		return FALSE; // return error;
    }
    if(pmic_read(SLAVE_ADDR_PM, (byte)reg_addr, reg_buff, length) != PMIC_PASS)
    {
        // Error - I2C write error
		return FALSE; // return error
    }

	return TRUE;
}

/*===========================================================================

FUNCTION Set_MAX8998_PM_ONOFF_CNTL                                

DESCRIPTION
    Control ON/OFF register for Bucks, LDOs, PWRHOLD, BATTMON, LBCNFGs,
    32kHzAP, 32kHzCP and VICHG

INPUT PARAMETERS
    byte onoff_reg  : selected register (ONOFF1_REG, ONOFF2_REG, ONOFF3_REG or ONOFF4_REG)
    byte cntl_mask  : mask bit, selected LDO and BUCK
    byte status     : turn on or off (0 - off, 1 - on)

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Turn on LDO5 and BUCK1:

    Set_MAX8998_PM_ONOFF_CNTL(ONOFF1_REG, (ONOFF1_EN1_M | ONOFF1_ELDO5_M), 1);

===========================================================================*/
void Set_MAX8998_PM_ONOFF_CNTL(byte onoff_reg, byte cntl_item, byte status)
{
    byte reg_value;
    byte set_value;

    if( (onoff_reg < ONOFF1_REG) || (onoff_reg > ONOFF4_REG) )
    {
        // Error - Invalid onoff control
        return; // return error
    }

    if (status == 0)
    {
        // off condition
        set_value = 0;
    }
    else if (status == 1)
    {
        // on condition
        set_value = cntl_item;
    }
    else
    {
      // Error - this condition is not defined
      return;
    }

    spin_lock(&pmic_access_lock);

    Get_MAX8998_PM_ADDR(onoff_reg, &reg_value, 1);
    reg_value = ((reg_value & ~cntl_item) | set_value);
    Set_MAX8998_PM_ADDR(onoff_reg, &reg_value, 1);

    spin_unlock(&pmic_access_lock);
}

/*===========================================================================

FUNCTION Set_MAX8998_PM_ADISCHG_EN                                

DESCRIPTION
    Control Active discharge for BUCKs, LDOs, and SAFEOUTs

INPUT PARAMETERS
    byte onoff_reg  : selected register (ADISCHG_EN1_REG, ADISCHG_EN2_REG or ADISCHG_EN3_REG)
    byte cntl_mask  : mask bit, selected LDO and BUCK
    byte status     : turn on or off (0 - disable, 1 - enable)
  
RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8998_PM_ADISCHG_EN(SD1DIS_M, 1);

===========================================================================*/
void Set_MAX8998_PM_ADISCHG_EN(byte adischg_en_reg, byte cntl_item, byte status)
{
    byte reg_value;
    byte set_value;

	if( (adischg_en_reg < ADISCHG_EN1_REG) || (adischg_en_reg > ADISCHG_EN3_REG) )
    {
        // Error - Invalid onoff control
        return; // return error
    }

    if (status == 0)
    {
        // off condition
        set_value = 0;
    }
    else if (status == 1)
    {
        // on condition
        set_value = cntl_item;
    }
    else
    {
      // Error - this condition is not defined
      return;
    }

    spin_lock(&pmic_access_lock);

	Get_MAX8998_PM_ADDR(adischg_en_reg, &reg_value, 1);
    reg_value = ((reg_value & ~cntl_item) | set_value);
	Set_MAX8998_PM_ADDR(adischg_en_reg, &reg_value, 1);

    spin_unlock(&pmic_access_lock);
}

/*===========================================================================

FUNCTION Set_MAX8998_PM_RAMP_RATE

DESCRIPTION
    Control ramp rate when buck uses DVS.

INPUT PARAMETERS
    ramp_rate_type ramp_rate : ramp rate (from RAMP_1mVpus to RAMP_12mVpus)
  
RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8998_PM_SET_RAMP_RATE(RAMP_11mVpus);

===========================================================================*/
void Set_MAX8998_PM_RAMP_RATE(ramp_rate_type ramp_rate)
{
    if (ramp_rate > RAMP_MAX)
    {
        // Error - Invalid value
        return;
    }

    Set_MAX8998_PM_REG(RAMP, (byte)ramp_rate);
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_BUCK1_2_Voltage                                

DESCRIPTION
    Control Output program register for BUCK1,2

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_BUCK1_2_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if ((byte)set_value > (byte)VCC_1p500) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)set_value);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_BUCK4_Voltage                                

DESCRIPTION
    Control Output program register for BUCK4

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_BUCK4_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (buck4_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)buck4_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO2_3_Voltage                                

DESCRIPTION
    Control Output program register for LDO2,3

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO2_3_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo2_3_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo2_3_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage                                

DESCRIPTION
    Control Output program register for BUCK3, LDO4~7,11,17

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (buck3_ldo4to7_11_17_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)buck3_ldo4to7_11_17_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO8_Voltage                                

DESCRIPTION
    Control Output program register for LDO8

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO8_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo8_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo8_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO9_Voltage                                

DESCRIPTION
    Control Output program register for LDO9

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO9_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo9_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo9_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO10_Voltage                                

DESCRIPTION
    Control Output program register for LDO10

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO10_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo10_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo10_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO12_13_Voltage                                

DESCRIPTION
    Control Output program register for LDO12,13

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO12_13_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo12_13_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo12_13_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO14_15_Voltage                                

DESCRIPTION
    Control Output program register for LDO14, 15

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO14_15_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo14_15_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo14_15_voltage[set_value]);
    return ret_val;
}

/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO16_Voltage                                

DESCRIPTION
    Control Output program register for LDO16

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO16_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo16_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo16_voltage[set_value]);
    return ret_val;
}



/*===========================================================================

FUNCTION Set_MAX8998_PM_OUTPUT_Voltage                                

DESCRIPTION
    Control Output program register for OUT1 - OUT19 except OUT12 and OUT13

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_OUTPUT_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if( (ldo_num < DVSARM1) || (ldo_num > LDO17) )
    {
        // Invalid buck or ldo register
        return FALSE; // return error
    }

    switch(ldo_num) {
    case DVSARM1:
    case DVSARM2:
    case DVSARM3:
    case DVSARM4:
    case DVSINT1:
    case DVSINT2:
        ret_val = Set_MAX8998_PM_BUCK1_2_Voltage(ldo_num, set_value);
        break;
    case BUCK4:
        ret_val = Set_MAX8998_PM_BUCK4_Voltage(ldo_num, set_value);
        break;
    case LDO2:
    case LDO3:
        ret_val = Set_MAX8998_PM_LDO2_3_Voltage(ldo_num, set_value);
        break;
    case BUCK3:
    case LDO4:
    case LDO5:
    case LDO6:
    case LDO7:
    case LDO11:
    case LDO17:
        ret_val = Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage(ldo_num, set_value);
        break;
    case LDO8:
        ret_val = Set_MAX8998_PM_LDO8_Voltage(ldo_num, set_value);
        break;
    case LDO9:
        ret_val = Set_MAX8998_PM_LDO9_Voltage(ldo_num, set_value);
        break;
    case LDO10:
        ret_val = Set_MAX8998_PM_LDO10_Voltage(ldo_num, set_value);
        break;
    case LDO12:
    case LDO13:
        ret_val = Set_MAX8998_PM_LDO12_13_Voltage(ldo_num, set_value);
        break;
    case LDO14:
    case LDO15:
        ret_val = Set_MAX8998_PM_LDO14_15_Voltage(ldo_num, set_value);
        break;
	case LDO16:
        ret_val = Set_MAX8998_PM_LDO16_Voltage(ldo_num, set_value);
        break;
    default:
        ret_val = FALSE;
        // Invalid BUCK or LDO number
        break;
    }
    return ret_val;
}

/*===========================================================================

FUNCTION Set_MAX8998_PM_LBCNFG

DESCRIPTION
    Control low battery configuration.

INPUT PARAMETERS
    max8998_pm_section_type cntl_item : control item (LBHYST1,2 or LBTH1,2)
    lbcnfg_type             lbcnfg    : control value
                                        (LBHYST_xxx or LBTH_xxx)
RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    

===========================================================================*/
void Set_MAX8998_PM_LBCNFG(max8998_pm_section_type cntl_item, lbcnfg_type lbcnfg)
{
    byte set_value;

    if ((cntl_item < LBHYST1) || (cntl_item > LBTH2))
    {
        // Error - Invalid register
        return;
    }

    if ( (cntl_item == LBTH1) || (cntl_item == LBTH2) )
    {
        set_value = (byte) lbcnfg;
    }
    else
    {
        set_value = (byte) lbcnfg - LBTH_2p9V;
    }

    Set_MAX8998_PM_REG(cntl_item, set_value);
}


#if 0
/*===========================================================================

      R T C     S E C T I O N

===========================================================================*/

/*===========================================================================

FUNCTION Set_MAX8998_RTC_REG                                

DESCRIPTION
    This function write the value at the selected register in the RTC section.

INPUT PARAMETERS
    max8998_rtc_section_type reg_num : Register number
    byte value                       : the value for reg_num

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_RTC_REG(max8998_rtc_section_type reg_num, byte value)
{
    byte reg_buff;

    if(pmic_read(SLAVE_ADDR_RTC, max8998rtc[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        #ifndef BOOTLOADER
        //status = NU_Release_Semaphore(&Maxim_PM_Semaphore);
        #endif
        printk("Set_MAX8998_RTC_REG Read failed\n");
        // Write Vreg control failed
        return FALSE; // return error
    }

    reg_buff = (reg_buff & max8998rtc[reg_num].clear) | (value << max8998rtc[reg_num].shift);
    if(pmic_write(SLAVE_ADDR_RTC, max8998rtc[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        #ifndef BOOTLOADER
       // status = NU_Release_Semaphore(&Maxim_PM_Semaphore);
        #endif
        printk("Set_MAX8998_RTC_REG Write failed\n");
        // Write Vreg control failed
        return FALSE;
    }
    
    return TRUE;
}


/*===========================================================================

FUNCTION Get_MAX8998_RTC_REG                                

DESCRIPTION
    This function read the value at the selected register in the RTC section.

INPUT PARAMETERS
    max8998_rtc_section_type reg_num :   selected register in the register address.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE
    reg_buff :  the value of selected register.
                reg_buff is aligned to the right side of the return value.

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8998_RTC_REG(max8998_rtc_section_type reg_num, byte *reg_buff)
{
    if(pmic_read(SLAVE_ADDR_RTC, max8998rtc[reg_num].addr, reg_buff, (byte)1) != PMIC_PASS)
    {
        // Write Vreg control failed
        return FALSE; // return error
    }
    *reg_buff = (*reg_buff & max8998rtc[reg_num].mask) >> max8998rtc[reg_num].shift;

    return TRUE;
}


/*===========================================================================

FUNCTION Set_MAX8998_RTC_ADDR                                

DESCRIPTION
    This function write the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8998_rtc_reg_type reg_addr : the register address.
    byte value                    : the value for register address.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_RTC_ADDR(max8998_rtc_reg_type reg_addr, byte value)
{
    if(pmic_write(SLAVE_ADDR_RTC, (byte)reg_addr, &value, (byte)1) != PMIC_PASS)
    {
        // Write Vreg control failed
        return FALSE;
    }
    return TRUE;
}


/*===========================================================================

FUNCTION Get_MAX8998_RTC_ADDR                                

DESCRIPTION
    This function read the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8998_rtc_reg_type reg_addr   : the register address.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE
    reg_buff : the value for register address.

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8998_RTC_ADDR(max8998_rtc_reg_type reg_addr, byte *reg_buff)
{
    if(pmic_read(SLAVE_ADDR_RTC, (byte)reg_addr, reg_buff, (byte)1) != PMIC_PASS)
    {
        // Write Vreg control failed
        return FALSE; // return error
    }
    return TRUE;
}


/*===========================================================================

FUNCTION Set_MAX8998_SMPL_Ctrl                                

DESCRIPTION
    Select SMPL function disable/enable

INPUT PARAMETERS
    onoff_status : 0 = disable
                   1 = enable

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_SMPL_Ctrl(byte onoff_status)
{
    if((onoff_status == 0) || (onoff_status == 1))
    {
        return Set_MAX8998_RTC_REG(WTSR_SMPL_CNTL_EN_SMPL, (byte)onoff_status);
    }
    else
    {
        // Invalid parameter
        return FALSE;
    }
}

/*===========================================================================

FUNCTION Set_MAX8998_SMPL_Timer                                

DESCRIPTION
    Select SMPL function timer

INPUT PARAMETERS
    timer = SMPLT_500mS
            SMPLT_1000mS
            SMPLT_1500mS
            SMPLT_2000mS

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_SMPL_Timer(max8998_rtc_smplt_type timer)
{
    if(timer > SMPLT_2000ms)
    {
        timer = SMPLT_2000ms;
    }
    return Set_MAX8998_RTC_REG(WTSR_SMPL_CNTL_TIME_SMPL, (byte)timer);
}

/*===========================================================================

FUNCTION Set_MAX8998_WTSR_Ctrl                                

DESCRIPTION
    Select WTSR function disable/enable

INPUT PARAMETERS
    onoff_status : 0 = disable
                   1 = enable

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_WTSR_Ctrl(byte onoff_status)
{
    if((onoff_status == 0) || (onoff_status == 1))
    {
        return Set_MAX8998_RTC_REG(WTSR_SMPL_CNTL_EN_WTSR, (byte)onoff_status);
    }
    else
    {
        // Invalid parameter 
        return FALSE;
    }
}

/*===========================================================================

FUNCTION Set_MAX8998_WTSR_Timer                                

DESCRIPTION
    Select WTSR function timer

INPUT PARAMETERS
    timer =  WTSRT_62p5ms
             WTSRT_78p125ms

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_WTSR_Timer(max8998_rtc_wtsrt_type timer)
{
    if(timer < WTSRT_78p12ms)
    {
        timer = WTSRT_62p5ms;
    }
    return Set_MAX8998_RTC_REG(WTSR_SMPL_CNTL_TIME_WTSR, (byte)timer);
}


/*===========================================================================

FUNCTION Set_MAX8998_RTC                                

DESCRIPTION
    This function write the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8998_rtc_cmd_type :     TIMEKEEPER = timekeeper register 0x0~0x7
                               ALARM0     = alarm0 register 0x8~0xF
                               ALARM1     = alarm1 register 0x10~0x18

    byte* max8998_rtc_ptr : the write value for registers.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_RTC(max8998_rtc_cmd_type rtc_cmd,byte *max8998_rtc_ptr)
{
    byte reg;

    reg = (byte)rtc_cmd * 8;

    if(pmic_write(SLAVE_ADDR_RTC, reg, max8998_rtc_ptr, (byte)8) != PMIC_PASS)
    {
        // Write RTC failed
        return FALSE;
    }
    return TRUE;
}

/*===========================================================================

FUNCTION Get_MAX8998_RTC                                

DESCRIPTION
    This function read the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8998_rtc_cmd_type :     TIMEKEEPER = timekeeper register 0x0~0x7
                               ALARM0     = alarm0 register 0x8~0xF
                               ALARM1     = alarm1 register 0x10~0x18

    byte* max8998_rtc_ptr : the return value for registers.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8998_RTC(max8998_rtc_cmd_type rtc_cmd, byte *max8998_rtc_ptr)
{
    byte reg;

    reg = (byte)rtc_cmd * 8;

    if(pmic_read(SLAVE_ADDR_RTC, reg, max8998_rtc_ptr, (byte)8) != PMIC_PASS)
    {
        // Read RTC failed
        return FALSE;
    }
    return TRUE;
}
#endif

#ifndef BOOTLOADER
/*===========================================================================

      I R Q    R O U T I N E

===========================================================================*/
irqreturn_t pmic_irq(int irq, void *dev_id)
{
	//wake_lock(&pmic_wake_lock);
	wake_lock_timeout(&pmic_wake_lock, 2 * HZ);
	disable_irq(PMIC_IRQ);

	hrtimer_cancel(&charger_timer);
	hrtimer_start(&charger_timer,
					ktime_set(500 / 1000, (500 % 1000) * 1000000),
					HRTIMER_MODE_REL);

	return IRQ_HANDLED;
}

irqreturn_t pmic_isr(void)
{	
//	maxim_batt_check();
	MAX8998_PM_IRQ_isr();
	if(readl(pmic_pend_mask_mem)&(0x1<<7))
		writel(readl(pmic_pend_mask_mem)|(0x1<<7), pmic_pend_mask_mem); 
	enable_irq(PMIC_IRQ);
	//wake_unlock(&pmic_wake_lock);
}

static enum hrtimer_restart charger_timer_func(struct hrtimer *timer)
{
	queue_work(pmic_int_wq, &pmic_int_work);
	return HRTIMER_NORESTART;
}

int get_boot_charger_info(void)
{
	return readl(S5P_INFORM5);
}

void lpm_mode_check(void)
{
	if(get_boot_charger_info())
	{
		if(maxim_lpm_chg_status())
			{
			charging_mode_set(1);
			}
		else{
			if (pm_power_off)
				pm_power_off();
		}
	}
	else
	{
		charging_mode_set(0);
	}
}

void set_low_bat_interrupt(int on)
{
	if(on)
	{
		if(!low_bat1_int_status)
		{
			Set_MAX8998_PM_REG(ELBCNFG1, 1);
		}
		Set_MAX8998_PM_REG(ELBCNFG2, 1);
	}
	else
	{
		Set_MAX8998_PM_REG(ELBCNFG1, 0);
		Set_MAX8998_PM_REG(ELBCNFG2, 0);
	}
}





/*===========================================================================

FUNCTION MAX8998_IRQ_init                                

DESCRIPTION
    Initialize the IRQ Mask register for the IRQ handler.

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8998_IRQ_init(void)
{
	byte irq_mask[4];
	int ret;

	pmic_pend_mask_mem = ioremap(PMIC_INT_PEND, 0x1);

// [[ junghyunseok edit for stepcharging 20100506
	stepchargingCount = 0;

	irq_mask[0] = (byte)IRQ1_M;
	irq_mask[1] = (byte)IRQ2_M;
	irq_mask[2] = (byte)IRQ3_M;
	irq_mask[3] = (byte)IRQ4_M;

	// initialize the PMIC_IRQ
	if(pmic_write(SLAVE_ADDR_PM, (byte)IRQ1_REG_M, irq_mask, (byte)4) != PMIC_PASS)
	{
	    // IRQ MASK aren't written.
	     printk("Write IRQ Mask failed \n");
	}

	s3c_gpio_setpull(GPIO_AP_PMIC_IRQ, S3C_GPIO_PULL_UP);

	set_irq_type(PMIC_IRQ, IRQ_TYPE_LEVEL_LOW);

	pmic_int_wq = create_singlethread_workqueue("pmic_int_wq");
	
	if (!pmic_int_wq)
		return -ENOMEM;

	Set_MAX8998_PM_REG(ENBATTMON, 1);
	Set_MAX8998_PM_REG(LBTH1,0x7); // 3.57V
	Set_MAX8998_PM_REG(LBHYST1,0x0);
	Set_MAX8998_PM_REG(LBTH2,0x5); // 3.4V	
	Set_MAX8998_PM_REG(LBHYST2,0x0);
	set_low_bat_interrupt(0);

	wake_lock_init(&pmic_wake_lock, WAKE_LOCK_SUSPEND, "pmic");
 
	INIT_WORK(&pmic_int_work, pmic_isr );			

	hrtimer_init(&charger_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	charger_timer.function = charger_timer_func;
/*
	hrtimer_init(&pm_sys_init_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pm_sys_init_timer.function = pm_sys_init_done;
	hrtimer_start(&pm_sys_init_timer,
					ktime_set(30000 / 1000, (30000 % 1000) * 1000000),
					HRTIMER_MODE_REL);
*/
	lpm_mode_check();

	ret = request_irq(PMIC_IRQ, pmic_irq, IRQF_PROBE_SHARED|IRQF_DISABLED, "pmic irq", 0);
		
	if (ret == 0) {
		printk("pmic interrupt registered \n");
	}
	else {
		printk("request_irq failed\n");
		return;
	}
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_IRQ                                

DESCRIPTION
    When some irq mask is changed, this function can be used.
    If you send max8998_isr as null(0) point, it means that the irq is masked.
    If max8998_isr points some functions, it means that the irq is unmasked.

INPUT PARAMETERS
    reg_num                   : IRQ Mask register number
    void (*max8998_isr)(void) : If irq is happened, then this routine is running.

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void Set_MAX8998_PM_IRQ(max8998_pm_section_type reg_num, void (*max8998_isr)(void))
{
    byte reg_buff;
    byte value;
    
    if((reg_num < START_IRQMASK) || (reg_num > END_IRQMASK))
    {
        // Invalid IRQ SET command
        return;
    }

    spin_lock(&pmic_access_lock);

    if(pmic_read(SLAVE_ADDR_PM, max8998pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Read I2C error
        return;
    }
    if(max8998_isr == 0)
    {   // if max8998_isr is a null pointer
        value = 1;
        reg_buff = (reg_buff | max8998pm[reg_num].mask);
        max8998_irq_table[reg_num - START_IRQMASK].irq_ptr = NULL;
    }
    else
    {
        value = 0;
        reg_buff = (reg_buff & max8998pm[reg_num].clear);
        max8998_irq_table[reg_num - START_IRQMASK].irq_ptr = max8998_isr;
    }

    if(pmic_write(SLAVE_ADDR_PM, max8998pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Write Vreg control failed
    }

    spin_unlock(&pmic_access_lock);
}


/*===========================================================================

FUNCTION MAX8998_PM_IRQ_isr                                

DESCRIPTION
    When IRQ pin is asserted, this isr routine check the irq bit and then
    proper function is called.
    Irq register can be set although irq is masked.
    So, the isr routine shoud check the irq mask bit, too.

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8998_PM_IRQ_isr()
{
    byte pm_irq_reg[4];
    byte pm_irq_mask[4];
    byte irq_name;
    int i;

    if(pmic_read(SLAVE_ADDR_PM, IRQ1_REG, pm_irq_reg, (byte)4) != PMIC_PASS)
    {
        // IRQ register isn't read
        printk("IRQ register isn't read \n");
        return; // return error
    }

    if(pmic_read(SLAVE_ADDR_PM, IRQ1_REG_M, pm_irq_mask, (byte)4) != PMIC_PASS)
    {
        // IRQ mask register isn't read
        printk("IRQ register isn't read \n");
        return; // return error
    }

	/*
		for(i=1; i<3; i++)
		{
			if(pm_irq_reg[i])	printk("pm_irq_reg[%d] = 0x%x \n", i, pm_irq_reg[i]);
		}
	*/

	/*	
	for(i=0;i<4;i++)
	{
			printk("pm_irq_reg[%d] = 0x%x \n", i, pm_irq_reg[i]);
			printk("pm_irq_reg_m[%d] = 0x%x \n", i, pm_irq_mask[i]);
	}
	*/		


    for(irq_name = START_IRQ; irq_name <= ENDOFIRQ; irq_name++)
    {
        if( (pm_irq_reg[max8998pm[irq_name].addr] & max8998pm[irq_name].mask)
            && ( (pm_irq_mask[max8998pm[irq_name].addr] & max8998pm[irq_name].mask) == 0)
            && (max8998_irq_table[irq_name].irq_ptr != 0) )
        {
            (max8998_irq_table[irq_name].irq_ptr)();
        }
    }
}
#endif //#ifndef BOOTLOADER

/*===========================================================================

      I N I T    R O U T I N E

===========================================================================*/
/*===========================================================================

FUNCTION MAX8998_PM_init

DESCRIPTION
    When power up, MAX8998_PM_init will initialize the MAX8998 for each part

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8998_PM_init(void)
{
/*
    #ifdef BOOTLOADER
#if defined(FEATURE_UNIV_REV00_C110)
       // ==============================================
    // ONOFF (1-3) REGISTER
    // ==============================================
    
    // ==============================================
    // ACTIVE DISCHARGE ENABLE (1-2) REGISTER
    // ==============================================
    // Default Enable
    
       // ==============================================
    // BUCK1, VARM  1.3V
    // ==============================================
    //set 1, 2  low
    GPIO_Set_Data(BUCK_1_EN_A, GPIO_DAT_LOW);
    GPIO_Set_Config(BUCK_1_EN_A, GPIO_CON_OUTPUT);
    GPIO_Set_Data(BUCK_1_EN_B, GPIO_DAT_LOW);
    GPIO_Set_Config(BUCK_1_EN_B, GPIO_CON_OUTPUT);
    Set_MAX8998_PM_OUTPUT_Voltage(DVSARM1, VCC_1p300);
    Set_MAX8998_PM_REG(EN1, 0); //control by pwren
    

       // ==============================================
    // BUCK2, DVSINT1,     1.3V : SET1-2 are grounded, so use DVSARM1
    // ==============================================
     GPIO_Set_Data(BUCK_2_EN, GPIO_DAT_LOW);
     GPIO_Set_Config(BUCK_2_EN, GPIO_CON_OUTPUT);
     Set_MAX8998_PM_OUTPUT_Voltage(DVSINT1, VCC_1p300);
     Set_MAX8998_PM_REG(EN2, 0); //control by pwren

    // ==============================================
    // BUCK3, VCC,      1.8V 
    // ==============================================
    //Default 1.8V

    // ==============================================
    // BUCK3, VCC,      1.2V 
    // ==============================================
    //Default 1.2V

   // ==============================================
    // LDO2, VALIVE,    1.2V
    // LDO3, VOTGI,     1.2V
    // ==============================================
    //Default 1.2V
   // ==============================================
    // LDO4, VDAC_2.8V_C100,   2.8V
    // ==============================================
    Set_MAX8998_PM_OUTPUT_Voltage(LDO4, VCC_2p800);
    //Set_MAX8998_PM_REG(ELDO4, 1);

       // ==============================================
    // LDO5, VTF_2.8V,   2.8V
    // ==============================================
    //Default 2.8V

       // ==============================================
    // LDO6, VCC_2.6V_C100,   2.6V
    // ==============================================
    //Default 2.6V

       // ==============================================
    // LDO7, OPTIC_3.0V,   3.0V
    // ==============================================
    //Default 3.0V
    Set_MAX8998_PM_REG(ELDO7, 1);

            // ==============================================
    // LDO8, VUSB_3.3V_C100, VADC_3.3V_C100,  3.3V
    // ==============================================
    //Default 3.3V

            // ==============================================
    // LDO9, VCC_2.8V_PDA, VCAM_2.8V_C100,  VUSB_HOST_2.8V 2.8V
    // ==============================================
    //Default 

            // ==============================================
    // LDO10, VPLL_1.2V_C100   1.2V
    // ==============================================
    //Default 
    Set_MAX8998_PM_REG(ELDO10, 0); //control by pwren    

#elif defined(FEATURE_UNIV_REV00_6442)

    // DVSARMx all are set as 1.2V which is default value
    // DVSARM1 is selected to set 1.2V for VARM
    GPIO_Set_Data(BUCK_1_EN_A, GPIO_DAT_LOW);
    GPIO_Set_Config(BUCK_1_EN_A, GPIO_CON_OUTPUT);
    GPIO_Set_Data(BUCK_1_EN_B, GPIO_DAT_LOW);
    GPIO_Set_Config(BUCK_1_EN_B, GPIO_CON_OUTPUT);

    // DVSINTx all are set as 1.2V which is default value
    // DVSINT1 is selected to set 1.2V for VINT
     GPIO_Set_Data(BUCK_2_EN, GPIO_DAT_LOW);
     GPIO_Set_Config(BUCK_2_EN, GPIO_CON_OUTPUT);	

    // I2C enable bits of BUCK1 & 2 are off. Becuase they must be controlled only by PWREN
    // BUCK1 is VARM, BUCK2 is VDDINT & BUCK3 is VDDMEM
    Set_MAX8998_PM_ONOFF_CNTL(ONOFF1_REG, ONOFF1_EN1_M | ONOFF1_EN2_M, 0);

    // I2C enable bit of LDO10 is off. Becuase it must be controlled only by PWREN
    // LDO10 is VPLL
    Set_MAX8998_PM_ONOFF_CNTL(ONOFF2_REG, ONOFF2_ELDO10_M, 0);
  
    // Target voltage for DVSARM1, 2, 3, 4 & DVSINT1, 2 are set by DVFS
#endif

    #else
    STATUS          status;
    status = NU_Create_Semaphore( &Maxim_PM_Semaphore, "PMIC SEMA", 1, NU_FIFO ) ;
    if (status != NU_SUCCESS)
    {
        printk("Maxim_PM_Semaphore create failed\n}");
        ERC_System_Error(status);
    }
    #endif
*/
}

//////////////////////////////////////////////////////////
/// RTC
/////////////////////////////////////////////////////////
#ifndef BOOTLOADER
#if 0
/*===========================================================================
FUNCTION clk_julian_to_maxim_rtc

SIDE EFFECTS
  None

===========================================================================*/
void clk_julian_to_maxim_rtc(byte hour_mode, maxim_rtc_julian_type *current_time_ptr, byte *maxim_rtc_ptr)
{
    unsigned short temp;

    //maxim_rtc_ptr[RTC_SEC] // bit[6:4]=10 Sec, bit[3:0]= Sec
    maxim_rtc_ptr[RTC_SEC_REG] = (((byte)(current_time_ptr->second / 10)) << 4) & 0x70;
    maxim_rtc_ptr[RTC_SEC_REG] |= ((byte)(current_time_ptr->second % 10)) & 0x0F;

    //maxim_rtc_ptr[RTC_MIN] // bit[6:4]=10 Min, bit[3:0]= Min
    maxim_rtc_ptr[RTC_MIN_REG] = (((byte)(current_time_ptr->minute / 10)) << 4) & 0x70;
    maxim_rtc_ptr[RTC_MIN_REG] |= ((byte)(current_time_ptr->minute % 10)) & 0x0F;

    //maxim_rtc_ptr[RTC_HOURS]  // bit[7]=Hour mode, bit[5:4]=10Hour or AM/PM&10Hour, bit[3:0]= Hour
    if(hour_mode == 0)
    { // 12 Hour mode
        maxim_rtc_ptr[RTC_HR_REG] = (byte)0x80;
    }
    else
    { // 24 Hour mode
        maxim_rtc_ptr[RTC_HR_REG] = (byte)0x0;
    }
    maxim_rtc_ptr[RTC_HR_REG] |= (((byte)(current_time_ptr->hour / 10)) << 4) & 0x30;
    maxim_rtc_ptr[RTC_HR_REG] |= ((byte)(current_time_ptr->hour % 10)) & 0x0F;

//current_time_ptr->day          = Day of month [1..31]
    //maxim_rtc_ptr[RTC_DATE] // bit[5:4]=10 Dat, bit[3:0]= Dat
    maxim_rtc_ptr[RTC_DATE_REG] = (((byte)(current_time_ptr->day / 10)) << 4) & 0x30;
    maxim_rtc_ptr[RTC_DATE_REG] |= ((byte)(current_time_ptr->day % 10)) & 0x0F;

    //maxim_rtc_ptr[RTC_MONTH] // bit[4]=10 Month, bit[3:0]= Month
    //current_time_ptr->month        = Month of year [1..12]
    maxim_rtc_ptr[RTC_MT_REG] = (((byte)(current_time_ptr->month / 10)) << 4) & 0x10;
    maxim_rtc_ptr[RTC_MT_REG] |= ((byte)(current_time_ptr->month % 10)) & 0x0F;

    //maxim_rtc_ptr[RTC_CEN] // bit[7:4]=1000 Year, bit[3:0]=100 Year
    //current_time_ptr->year         = Year [1980..2100]
    temp = (current_time_ptr->year);
    maxim_rtc_ptr[RTC_CEN_REG] = (byte)((temp / 1000) << 4) & 0xF0;
    temp = temp % 1000;
    maxim_rtc_ptr[RTC_CEN_REG] |= (byte)(temp / 100) & 0x0F;
    temp = temp % 100;

    //maxim_rtc_ptr[RTC_YEAR] // bit[7:4]=10 Year, bit[3:0]= Year
    maxim_rtc_ptr[RTC_YEAR_REG] = (byte)((temp / 10) << 4) & 0xF0;
    temp = temp % 10;
    maxim_rtc_ptr[RTC_YEAR_REG] |= (byte)temp & 0x0F;
}


/*===========================================================================
FUNCTION clk_maxim_rtc_to__julian


SIDE EFFECTS
  None

===========================================================================*/
void clk_maxim_rtc_to_julian(maxim_rtc_julian_type *current_time_ptr, byte *maxim_rtc_ptr)
{
    byte temp;

    //maxim_rtc_ptr[RTC_SEC] // bit[6:4]=10 Sec, bit[3:0]= Sec
    temp = maxim_rtc_ptr[RTC_SEC_REG];
    current_time_ptr->second = (unsigned short)(((temp >> 4)*10) + (temp & 0x0F));

    //maxim_rtc_ptr[RTC_MIN] // bit[6:4]=10 Min, bit[3:0]= Min
    temp = maxim_rtc_ptr[RTC_MIN_REG];
    current_time_ptr->minute = (unsigned short)(((temp >> 4)*10) + (temp & 0x0F));

    //maxim_rtc_ptr[RTC_HOURS]  // bit[7]=Hour mode, bit[5:4]=10Hour or AM/PM&10Hour, bit[3:0]= Hour
    temp = maxim_rtc_ptr[RTC_HR_REG] & 0x3F;
    current_time_ptr->hour = (unsigned short)(((temp >> 4)*10) + (temp & 0x0F));

    //current_time_ptr->day          = Day of month [1..31]
    //maxim_rtc_ptr[RTC_DATE] // bit[5:4]=10 Dat, bit[3:0]= Dat
    temp = maxim_rtc_ptr[RTC_DATE_REG] & 0x3F;
    current_time_ptr->day = (unsigned short)(((temp >> 4)*10) + (temp & 0x0F));

    //maxim_rtc_ptr[RTC_MONTH] // bit[4]=10 Month, bit[3:0]= Month
    //current_time_ptr->month        = Month of year [1..12]
    temp = maxim_rtc_ptr[RTC_MT_REG] & 0x1F;
    current_time_ptr->month = (unsigned short)(((temp >> 4)*10) + (temp & 0x0F));

    //maxim_rtc_ptr[RTC_CEN] // bit[7:4]=1000 Year, bit[3:0]=100 Year
    //current_time_ptr->year         = Year [1980..2100]
    temp = maxim_rtc_ptr[RTC_YEAR_REG];
    current_time_ptr->year = (unsigned short)(((temp >> 4)*10) + (temp & 0x0F));

    temp = maxim_rtc_ptr[RTC_CEN_REG];
    current_time_ptr->year += (unsigned short)(((temp >> 4)*1000) + (temp & 0x0F)*100);
}

/*===========================================================================
FUNCTION   maxim_pm_rtc_rw_cmd                                   EXTERNAL FUNCTION

SIDE EFFECTS
   Interrupts are locked for the duration of this function.
===========================================================================*/
pmic_status_type  maxim_pm_rtc_rw_cmd(
   maxim_rtc_cmd_type       cmd,
   maxim_rtc_julian_type   *current_time_ptr)
{
   pmic_status_type      errFlag    = PMIC_PASS;

   byte maxim_rtc_ptr[8];

   // do some sanity checking on params ...
   if (cmd >= MAXIM_RTC_INVALID_CMD)
   {
      return PMIC_FAIL;
   }
   if (!current_time_ptr)
   {
      return PMIC_FAIL;
   }

   switch (cmd)
   {
      case MAXIM_RTC_SET_CMD:
           if (MAXIM_RTC_12HR_MODE == pm_app_rtc_current_display_mode)  // Is it in AM/PM mode?
         {
            clk_julian_to_maxim_rtc(0, (maxim_rtc_julian_type*)current_time_ptr, (byte*)maxim_rtc_ptr);
         }
         else
         {
            clk_julian_to_maxim_rtc(1, (maxim_rtc_julian_type*)current_time_ptr, (byte*)maxim_rtc_ptr);
         }
         // start RTC

         Set_MAX8998_RTC(TIMEKEEPER, maxim_rtc_ptr);
	  break;

      case MAXIM_RTC_GET_CMD:
	  Get_MAX8998_RTC(TIMEKEEPER, maxim_rtc_ptr);
	  clk_maxim_rtc_to_julian((maxim_rtc_julian_type*)current_time_ptr, maxim_rtc_ptr);
         break;

      case MAXIM_RTC_INVALID_CMD:
         // this will never be reached here because sanity check for 'cmd'
         // has already been done above; just to satisfy LINT's curiousity!
         break;

      // we'll not have 'default' so as to leave open the possibility for LINT
      //   to warn us when we do miss any new enums added in the future.
   }

   return errFlag;
}


/*===========================================================================
FUNCTION   maxim_pm_rtc_alarm_rw_cmd                             EXTERNAL FUNCTION


   Interrupts are locked for the duration of this function.
===========================================================================*/
pmic_status_type maxim_pm_rtc_alarm_rw_cmd(
   maxim_rtc_cmd_type       cmd,
   maxim_rtc_alarm_type     what_alarm,
   maxim_rtc_julian_type   *alarm_time_ptr)
{
   pmic_status_type      errFlag    = PMIC_PASS;

   byte maxim_alarm_ptr[8];

   // do some sanity checking on params ...
   if (cmd >= MAXIM_RTC_INVALID_CMD)
   {
      return PMIC_FAIL;
   }

   if ( (!what_alarm)                         // Is this any alarm?
        // Is this a valid one?
        || (what_alarm > MAXIM_RTC_ALL_ALARMS)
                                              // Reason: PM_RTC_ALL_ALARMS is set up as an OR of all individual alarms
                                              //         which themselves are varying powers of 2. So any alarm passed
                                              //         to this func as something greater than PM_RTC_ALL_ALARMS will
                                              //         be a bogus one, and this check will then evaluate to 'true'.
        // Does this include more than 1 alarm at the same time?
        || (what_alarm & (what_alarm-1)) )
   {
      return PMIC_FAIL;
   }

   if (!alarm_time_ptr)
   {
      return PMIC_FAIL;
   }

   switch (cmd)
   {
      case MAXIM_RTC_SET_CMD:

         if (MAXIM_RTC_12HR_MODE == pm_app_rtc_current_display_mode)    // time is in AM/PM mode?
         {
            clk_julian_to_maxim_rtc(0, (maxim_rtc_julian_type*)alarm_time_ptr, (byte*)maxim_alarm_ptr);
         }
         else
         {
            clk_julian_to_maxim_rtc(1, (maxim_rtc_julian_type*)alarm_time_ptr, (byte*)maxim_alarm_ptr);
         }
         // start RTC
         Set_MAX8998_RTC(RTC_ALARM0, maxim_alarm_ptr);

	  Set_MAX8998_RTC_ADDR(RTC_ALRM0CONF_REG, 0x77);
         break;

      case MAXIM_RTC_GET_CMD:
	  Get_MAX8998_RTC(RTC_ALARM0, maxim_alarm_ptr);
	  clk_maxim_rtc_to_julian((maxim_rtc_julian_type*)alarm_time_ptr, maxim_alarm_ptr);
         break;

      case MAXIM_RTC_INVALID_CMD:
         // this will never be reached here because sanity check for 'cmd'
         // has already been done above; just to satisfy LINT's curiousity!
         break;

      // we'll not have 'default' so as to leave open the possibility for LINT
      //   to warn us when we do miss any new enums added in the future.
   }

   return errFlag;
}

pmic_status_type pm_reset_rtc_alarm(maxim_rtc_alarm_type what_alarm)
{
	if(Set_MAX8998_RTC_ADDR(RTC_ALRM0CONF_REG, 0x0))
		return PMIC_PASS;
	else
		return PMIC_FAIL;
}
#endif

void camera_ldo_control(unsigned char onoff)
{
    if(onoff)
    {
        Set_MAX8998_PM_REG(EN4, 1); // CAM_D_1.2V
        udelay(200);
        Set_MAX8998_PM_REG(ELDO11, 1); //Set_MAX8998_PM_REG(ELDO11, 1);
        Set_MAX8998_PM_REG(ELDO12, 1); //CAM_D_1.8V
        Set_MAX8998_PM_REG(ELDO13, 1); //CAM_A_2.8V
        Set_MAX8998_PM_REG(ELDO14, 1); //CAM_IO_1.8V
        Set_MAX8998_PM_REG(ELDO15, 1); //CAM_D_2.8V
        Set_MAX8998_PM_REG(ELDO17, 1); //VMOT_3.3V

    }
    else
    {
        Set_MAX8998_PM_REG(ELDO17, 0); //VMOT_3.3V
        Set_MAX8998_PM_REG(ELDO11, 0); //Set_MAX8998_PM_REG(ELDO11, 1);
        Set_MAX8998_PM_REG(ELDO12, 0); //CAM_D_1.8V
        Set_MAX8998_PM_REG(ELDO13, 0); //CAM_A_2.8V
        Set_MAX8998_PM_REG(ELDO14, 0); //CAM_IO_1.8V
        Set_MAX8998_PM_REG(ELDO15, 0); //CAM_D_2.8V
        udelay(300);
        Set_MAX8998_PM_REG(EN4, 0); // CAM_D_1.2V
    }

}

extern int set_tsp_for_ta_detect(int state);
void maxim_vac_connect(void)
{
	u8 UsbStatus=0;

	//printk("maxim_vac_connect \n");

	if(Get_MAX8998_PM_REG(VDCINOK_status))
	{
		if(curent_device_type == PM_CHARGER_NULL)
		{
			UsbStatus = FSA9480_Get_I2C_USB_Status();
			if(UsbStatus){
				//printk("maxim_USB_connect~~~ \n");
				curent_device_type = PM_CHARGER_USB_INSERT;
			}
			else{
				//printk("maxim_TA_connect~~~ \n");
				curent_device_type = PM_CHARGER_TA;
				MicroTAstatus = 1;
				set_tsp_for_ta_detect(1);
			}
			s3c_cable_changed();
		}
	}
	else
	{
		printk("%s: disconnect status skip\n", __func__);
	}
}

void maxim_vac_disconnect(void)
{
	//printk("maxim_vac_disconnect \n");
	if(!Get_MAX8998_PM_REG(VDCINOK_status))
	{

// [[ junghyunseok edit for stepcharging 20100506
   		del_timer_sync(&chargingstep_timer);

		if(charging_mode_get())
		{
			printk("Power off cause charger removed in LPM mode\n");
			//if (pm_power_off)
			//	pm_power_off();
		}	

		if(curent_device_type == PM_CHARGER_TA)
			set_tsp_for_ta_detect(0);

		MicroTAstatus = 0;
		s3c_cable_changed();
		curent_device_type = PM_CHARGER_NULL;
		
		low_bat1_int_status=0;
	}
	else
	{
		printk("%s: connect status skip\n", __func__);
	}
}


void maxim_topoff_change(void)
{
	if(curent_device_type==PM_CHARGER_TA)
	{	
#if defined(CONFIG_S5PC110_T959_BOARD) 
		Set_MAX8998_PM_REG(TP, 0x1); 
#else
		Set_MAX8998_PM_REG(TP, 0x0); 
#endif
		printk("%s dev: TA\n",__func__);
	}
	else if (curent_device_type==PM_CHARGER_USB_INSERT)
	{	
#if defined(CONFIG_S5PC110_T959_BOARD) 	
		Set_MAX8998_PM_REG(TP, 0x2); 
#else
		Set_MAX8998_PM_REG(TP, 0x1); 
#endif	
		printk("%s dev: USB\n",__func__);
	}
	else
	{
		printk("%s unknown dev\n",__func__);
	}
		
}

void maxim_charging_topoff(void)
{
	printk("%s\n",__func__);
	if(curent_device_type != PM_CHARGER_NULL)
		s3c_cable_charging();
	else
		printk("%s: wrong topoff !!!\n",__func__);
}
void maxim_pwr_press(void)
{
}
void maxim_pwr_release(void)
{
}
void maxim_low_battery_2nd(void) // 3.3v
{
	printk("%s\n",__func__);
	wake_lock_timeout(&pmic_wake_lock, 30 * HZ);
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504	
//	low_battery_power_off();
	low_battery_flag = 1;
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504	
        //printk("battery voltage is under 3.3V\n");
}
void maxim_low_battery_1st(void) // 3.57V
{
	printk("%s\n",__func__);
	wake_lock_timeout(&pmic_wake_lock, 5 * HZ);
	low_bat1_int_status=1;
        //printk("battery voltage is under 3.57V\n");
}
#endif


unsigned char maxim_chg_status(void)
{
	if(Get_MAX8998_PM_REG(VDCINOK_status)){
		if(FSA9480_Get_I2C_USB_Status()){
			//printk("maxim_USB_connect~~~ \n");
			curent_device_type = PM_CHARGER_USB_INSERT;
		}
		else{
			//printk("maxim_TA_connect~~~ \n");
			curent_device_type = PM_CHARGER_TA;
			MicroTAstatus = 1;
		}
		return 1;
	}
	else
		return 0;  
}

unsigned char maxim_lpm_chg_status(void)
{
	return Get_MAX8998_PM_REG(VDCINOK_status);
}
unsigned char maxim_jig_status(void)
{
	return Get_MAX8998_PM_REG(JIG_status);
}

unsigned int maxim_pwr_key_status(void)
{
	return Get_MAX8998_PM_REG(PWRON_status);
}

unsigned char maxim_charging_enable_status(void)
{
	return Get_MAX8998_PM_REG(CHGON_status);
}

#define BAT_DETECTED		1
#define BAT_NOT_DETECTED	0
unsigned char maxim_vf_status(void)
{
	unsigned char ret;
	if(Get_MAX8998_PM_REG(DETBAT_status))
		ret = BAT_NOT_DETECTED;
	else
		ret = BAT_DETECTED;
	return ret;
}

/***********************************************************************
IRQ1_REG
    PWRONR | PWRONF | JIGR | JIGF | DCINR | DCINF | X  | X
IRQ2_REG
     X   |    X    |   X   | X  | ALARM0  | ALARM1 |  SMPLEVNT  | WTSREVNT
IRQ3_REG
   CHGFAULT  | X  | DONER | CHGRSTF | DCINOVPR  | TOPOFFR | X  | ONKEY1S
IRQ4_REG
   X  | X  |  X  |  X  |   X   |  X   |   LOBAT2  |  LOBAT1
************************************************************************/
unsigned int max8998_poweron_reason(void)
{
	unsigned int power_src = 0;
	byte reg_buf[4];
	byte alarm_conf;

	Get_MAX8998_PM_ADDR(IRQ1_REG, reg_buf, 4);
	power_src = power_src |(reg_buf[3] << 24); //IRQ4_REG
       power_src = power_src |(reg_buf[2] << 8); //IRQ3_REG
       power_src = power_src |(reg_buf[1] << 16); //IRQ2_REG
       power_src = power_src |(reg_buf[0] << 0); //IRQ1_REG
	// clear Alarm Event
	// this function does a bad operation(access wrong address)
	//(void) Get_MAX8998_RTC_ADDR(RTC_ALRM0CONF_REG, &alarm_conf);

	return power_src;

}

extern int  FSA9480_PMIC_CP_USB(void);
extern int askonstatus;
extern int mtp_mode_on;

// [[ junghyunseok edit for stepcharging 20100506

/*===========================================================================
// [[ junghyunseok edit for stepcharging workqueue function 20100517
FUNCTION chargingstep_timer_func                                

DESCRIPTION
    to apply  step increment current 

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/

static void chargingstep_timer_func(unsigned long unused)
{
	//pr_info("[charing step]:%s  \n", __func__);

	schedule_work(&stepcharging_work);
}
// ]] junghyunseok edit for stepcharging 20100506	   


static void maxim_stepcharging_work(struct work_struct *work)
{


       stepchargingCount++; 
	   
	//pr_info("[BAT]:%s, stepchargingCount = %d\n", __func__, stepchargingCount);

       if(curent_device_type == PM_CHARGER_TA)
       {
       	//pr_info("[BAT]:%s,PM_CHARGER_TA ,  stepchargingCount = %d\n", __func__, stepchargingCount);

	      if(stepchargingCount == 1)				
           	{            // 90mA
	                     stepchargingreg_buff[0] = (stepchargingreg_buff[0] & 0xF8) | 0x00;
				Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 
				mod_timer(&chargingstep_timer, jiffies + msecs_to_jiffies(CHARGINGSTEP_INTERVAL));				
	        }
	    else if(stepchargingCount == 2)
			{             //380mA
	            stepchargingreg_buff[0] = (stepchargingreg_buff[0] & 0xF8) | 0x01;
				Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 
				mod_timer(&chargingstep_timer, jiffies + msecs_to_jiffies(CHARGINGSTEP_INTERVAL));				
            }		 	
	     else if(stepchargingCount == 3)
		    {             //475mA
			    stepchargingreg_buff[0] = (stepchargingreg_buff[0] & 0xF8) | 0x02;
				Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 
				mod_timer(&chargingstep_timer, jiffies + msecs_to_jiffies(CHARGINGSTEP_INTERVAL));				
		    }
	    else if(stepchargingCount == 4)
		    {             //600mA
            	           stepchargingCount = 0;
			    stepchargingreg_buff[0] = (stepchargingreg_buff[0] & 0xF8) | 0x05;
				Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 
		    }
	     else
		    {
				stepchargingCount = 0;
		    }
       }		
       else if(curent_device_type == PM_CHARGER_USB_INSERT)
       {
       	//pr_info("[BAT]:%s,PM_CHARGER_USB_INSERT ,  stepchargingCount = %d\n", __func__, stepchargingCount);
       

	      if(stepchargingCount == 1)				
           	{            // 90mA
	           		 stepchargingreg_buff[0] = (stepchargingreg_buff[0] & 0xF8) | 0x00;
				Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 
				mod_timer(&chargingstep_timer, jiffies + msecs_to_jiffies(CHARGINGSTEP_INTERVAL));				
		    }
	    else if(stepchargingCount == 2)
		    {             //380mA
			    	stepchargingreg_buff[0] = (stepchargingreg_buff[0] & 0xF8) | 0x01;
				Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 
				mod_timer(&chargingstep_timer, jiffies + msecs_to_jiffies(CHARGINGSTEP_INTERVAL));				
		    }		 	
	    else if(stepchargingCount == 3)
		    {             //475mA
            	           	stepchargingCount = 0;
			    	stepchargingreg_buff[0] = (stepchargingreg_buff[0] & 0xF8) | 0x02;
				Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 
		    }
	   else
			{
				stepchargingCount = 0;
			}
      	}	
	else
	{
		Set_MAX8998_PM_REG(CHGENB, 0x1); //disable charge
	}
	
}
// ]] junghyunseok edit for stepcharging workqueue function 20100517

void stepcharging_Timer_setup()
{

	INIT_WORK(&stepcharging_work, maxim_stepcharging_work);	// [[ junghyunseok edit for stepcharging workqueue function 20100517
	setup_timer(&chargingstep_timer, chargingstep_timer_func, 0);
}

void maxim_charging_control(unsigned int dev_type  , unsigned int cmd, int uicharging)
{

//	byte reg_buff[2];
	int value;
// [[junghyunseok edit to remove topoff 20100510	
    byte  iTemp;
	
	if(!cmd) //disable
	{
		Set_MAX8998_PM_REG(CHGENB, 0x1); //disable charge
		//printk("%s charging disable \n",__func__);
	}
	else if(dev_type==PM_CHARGER_TA)
	{
// [[ junghyunseok edit for recharging and full charging 20100406
#if defined(CONFIG_KEPLER_VER_B2) ||defined(CONFIG_T959_VER_B5)
// 0x06 address
              Get_MAX8998_PM_ADDR(IRQ3_REG_M, &iTemp, 1);
//              printk("[POW] %s , read 06 iTemp = %x \n",__func__, iTemp);
		if(iTemp == 0x0) 
			     iTemp = 0xFB;  	  
              iTemp = iTemp | TOPOFF_MASK_BIT;
//              printk("[POW] %s , write 06 iTemp = %x \n",__func__, iTemp);
		Set_MAX8998_PM_ADDR(IRQ3_REG_M, &iTemp, 1); //TOP-OFF INT disable 

// 0x0D address
              Get_MAX8998_PM_ADDR(CHGR2, &iTemp, 1);
//              printk("[POW] %s , read 0D iTemp = %x \n",__func__, iTemp);
              iTemp = iTemp | 0x30;  // disable Fast charging Timer
//              printk("[POW] %s , write 0D iTemp = %x \n",__func__, iTemp);
		Set_MAX8998_PM_ADDR(CHGR2, &iTemp, 1); //TOP-OFF INT disable 

		stepchargingreg_buff[0] = (0x0 <<5) |(0x3 << 3) |(0x5<<0) ; // CHG_TOPOFF_TH=10%, CHG_RST_HYS=disable, AC_FCGH= 600mA
// ]]junghyunseok edit to remove topoff 20100510
#else
	
		if(uicharging)
#if defined(CONFIG_S5PC110_T959_BOARD) 			
			stepchargingreg_buff[0] = (0x1 <<5) |(0x3 << 3) |(0x5<<0) ; // CHG_TOPOFF_TH=15%, CHG_RST_HYS=disable, AC_FCGH= 600mA
#else	
			stepchargingreg_buff[0] = (0x0 <<5) |(0x3 << 3) |(0x5<<0) ; // CHG_TOPOFF_TH=10%, CHG_RST_HYS=disable, AC_FCGH= 600mA
#endif
		else
			stepchargingreg_buff[0] = (0x2 <<5) |(0x3 << 3) |(0x5<<0) ; // CHG_TOPOFF_TH=20%, CHG_RST_HYS=disable, AC_FCGH= 600mA
#endif
// [[junghyunseok edit to remove topoff 20100510
		stepchargingreg_buff[1] = (0x2<<6) |(0x3<<4) | (0x0<<3) | (0x0<<1) | (0x0<<0); //ESAFEOUT1,2= 10, FCHG_TMR=Disable, MBAT_REG_TH=4.2V, MBATT_THERM_REG=105C


//	      printk("[STEPCharging ]  TA maxim_charging_control call  \n");
// [[ junghyunseok edit for stepcharging of behold3 20100510
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_T959_BOARD) 	
		//parkhj modify
              // when power on booting 
              stepchargingCount = 0;
              mod_timer(&chargingstep_timer, jiffies + msecs_to_jiffies(CHARGINGSTEP_INTERVAL));
#else
		stepchargingCount = 0;
		Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 	
#endif				  
	}
	else if(dev_type==PM_CHARGER_USB_INSERT)
	{	
		value = FSA9480_PMIC_CP_USB();
// [[junghyunseok edit to remove topoff 20100510		
#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)
// 0x06 address
              Get_MAX8998_PM_ADDR(IRQ3_REG_M, &iTemp, 1);
//              printk("[POW] %s , read 06 iTemp = %x \n",__func__, iTemp);
		if(iTemp == 0x0) 
			     iTemp = 0xFB;  	  
              iTemp = iTemp | TOPOFF_MASK_BIT;
//              printk("[POW] %s , write 06 iTemp = %x \n",__func__, iTemp);
		Set_MAX8998_PM_ADDR(IRQ3_REG_M, &iTemp, 1); //TOP-OFF INT disable 

// 0x0D address
              Get_MAX8998_PM_ADDR(CHGR2, &iTemp, 1);
//              printk("[POW] %s , read 0D iTemp = %x \n",__func__, iTemp);
              iTemp = iTemp | 0x30;  // disable Fast charging Timer
//              printk("[POW] %s , write 0D iTemp = %x \n",__func__, iTemp);
		Set_MAX8998_PM_ADDR(CHGR2, &iTemp, 1); //TOP-OFF INT disable 
// ]]junghyunseok edit to remove topoff 20100510
		stepchargingreg_buff[0] = (0x1 <<5) |(0x3 << 3) |(0x2<<0) ; // CHG_TOPOFF_TH=10%, CHG_RST_HYS=disable, AC_FCGH= 475mA		
#else		
	
		if(uicharging)
#if defined(CONFIG_S5PC110_T959_BOARD) 					
			stepchargingreg_buff[0] = (0x2<<5) |(0x3 << 3) |(0x2<<0) ; // CHG_TOPOFF_TH=20%, CHG_RST_HYS=disable, AC_FCGH= 475mA
#else
			stepchargingreg_buff[0] = (0x1<<5) |(0x3 << 3) |(0x2<<0) ; // CHG_TOPOFF_TH=15%, CHG_RST_HYS=disable, AC_FCGH= 475mA
#endif
		else
			stepchargingreg_buff[0] = (0x3 <<5) |(0x3 << 3) |(0x2<<0) ; // CHG_TOPOFF_TH=25%, CHG_RST_HYS=disable, AC_FCGH= 475mA
#endif
// [[junghyunseok edit to remove topoff 20100510
		if(value)
		{
			if (askonstatus||mtp_mode_on){			
				stepchargingreg_buff[1] = (0x1<<6) |(0x3<<4) | (0x0<<3) | (0x0<<1) | (0x0<<0); //ESAFEOUT1,2= 01, FCHG_TMR=disable, MBAT_REG_TH=4.2V, MBATT_THERM_REG=105C
				//printk("[Max8998_function]AP USB Power OFF, askon: %d, mtp : %d\n",askonstatus,mtp_mode_on);
				}
			else{
				stepchargingreg_buff[1] = (0x2<<6) |(0x3<<4) | (0x0<<3) | (0x0<<1) | (0x0<<0); //ESAFEOUT1,2= 10, FCHG_TMR=disable, MBAT_REG_TH=4.2V, MBATT_THERM_REG=105C
				//printk("[Max8998_function]AP USB Power ON, askon: %d, mtp : %d\n",askonstatus,mtp_mode_on);
				}
		}
		else
			stepchargingreg_buff[1] = (0x2<<5) |(0x3<<4) | (0x0<<3) | (0x0<<1) | (0x0<<0); //ESAFEOUT1,2= 01, FCHG_TMR=disable, MBAT_REG_TH=4.2V, MBATT_THERM_REG=105C
// ]]junghyunseok edit to remove topoff 20100510

//	      printk("[STEPCharging ]  USB  maxim_charging_control call  \n");
// [[ junghyunseok edit for stepcharging of behold3 20100510
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_T959_BOARD) 	
		// when power on booting 
		stepchargingCount = 0;
		mod_timer(&chargingstep_timer, jiffies + msecs_to_jiffies(CHARGINGSTEP_INTERVAL));
#else
		stepchargingCount = 0;
		Set_MAX8998_PM_ADDR(CHGR1, stepchargingreg_buff, 2); 	
#endif	
	}
	else
	{
		stepchargingCount = 0;
		Set_MAX8998_PM_REG(CHGENB, 0x1); //disable charge
		//printk("%s charging disable \n",__func__);
	}
}
// ]] junghyunseok edit for stepcharging 20100506

void maxim_batt_check(void)
{
	byte reg_buff[2];
	
	Get_MAX8998_PM_ADDR(CHGR1, reg_buff, 2); 
	Set_MAX8998_PM_REG(CHGENB, 0x1); 
	Set_MAX8998_PM_ADDR(CHGR1, reg_buff, 2); 
}

static enum hrtimer_restart pm_sys_init_done(struct hrtimer *timer)
{
	//hrtimer_cancel(timer);
	pm_sys_start=PMIC_SYS_DONE;
	//printk("%s \n",__func__);
	return HRTIMER_NORESTART;
}

int pm_get_sys_init_status(void)
{
	return pm_sys_start;
}
