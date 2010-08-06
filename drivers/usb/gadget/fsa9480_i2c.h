/*
* ==============================================================================
*  Name          : fsa9480_i2c.h
*  Part of         : MicroUsbDetector Driver
*  Description :  Definitions of FSA9480
*  Version       : 0
*  Author         : wonsuk jung (grant.jung@samsung.com)
*
* ==============================================================================
*/


/* enalbing debug massage related with FSA9480*/
//#define FSA9480_DBG_ENABLE	1
//#define FSA9480_DBG_ENABLE	0
#undef FSA9480_DBG_ENABLE


#ifdef  FSA9480_DBG_ENABLE
#define DEBUG_FSA9480(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_FSA9480(fmt,args...) do {} while(0)
#endif


/********************************************************************/
/* FSA9480 Register definition                                                                                */
/********************************************************************/
/* DEVICE ID Register*/
#define REGISTER_DEVICEID	        0x01     
/* CONTROL Register*/
#define REGISTER_CONTROL	        0x02   
#define REGISTER_INTERRUPT1	        0x03  
#define REGISTER_INTERRUPT2	        0x04   
#define REGISTER_INTERRUPTMASK1 	0x05
#define REGISTER_INTERRUPTMASK2 	0x06
#define REGISTER_ADC 	            0x07
#define REGISTER_TIMINGSET1      	0x08
#define REGISTER_TIMINGSET2 	    0x09
#define REGISTER_DEVICETYPE1    	0x0A
#define REGISTER_DEVICETYPE2 	    0x0B
#define REGISTER_BUTTON1 	        0x0C
#define REGISTER_BUTTON2 	        0x0D
#define REGISTER_CARKITSTATUS 	    0x0E
#define REGISTER_CARKITINT1 	    0x0F
#define REGISTER_CARKITINT2 	    0x10
#define REGISTER_CARKITMASK1 	    0x11
#define REGISTER_CARKITMASK2 	    0x12
/* Manual SW1 Register*/
#define REGISTER_MANUALSW1	        0x13                       
/* Manual SW2 Register */
#define REGISTER_MANUALSW2	        0x14                   
/* Hidden Register*/
#define HIDDEN_REGISTER_MANUAL_OVERRDES1	0x1B     

/*define IRQ INTB*/
#define IRQ_FSA9480_INTB	IRQ_EINT(23)


//CR2 : Control Register
#define		INT_MASK				(0x1 << 0)
#define 	SW_WAIT		 			(0x1 << 1)
#define 	MANUAL_SW 				(0x1 << 2)
#define 	RAW_DATA				(0x1 << 3)
#define 	SW_OPEN					(0x1 << 4)

//CR3 : Interrupt 1 Register
#define		ATTACH					(0x1 << 0)
#define		DETACH					(0x1 << 1)
#define		KEY_PRESS				(0x1 << 2)
#define		LONG_KEY_PRESS			(0x1 << 3)
#define		LONG_KEY_RELEASE		(0x1 << 4)
#define		OVP_EN					(0x1 << 5)
#define		OCP_EN					(0x1 << 6)
#define		OVP_OCP_DIS				(0x1 << 7)

//CR5 : Interrupt 1 Mask Register
#define		ATTACH_INT_MASK				(0x1 <<0)
#define 	DETACH_INT_MASK 			(0x1 <<1)
#define 	KEY_PRESS_INT_MASK			(0x1 <<2)
#define 	LONGKEY_PRESS_INT_MASK 		(0x1 <<3)
#define 	LONGKEY_RELEASE_INT_MASK	(0x1 <<4)
#define 	OVP_INT_MASK 				(0x1 <<5)
#define 	OCP_INT_MASK 				(0x1 <<6)
#define 	OVP_OCP_DIS_INT_MASK 		(0x1 <<7)

//CR7 : ADC Register
#define		USB_OTG						0x00
#define		SEND_END					0x01
#define		AUDIO_REMOTE_S1_BUTTON		0x02
#define 	AUDIO_REMOTE_S2_BUTTON		0x03
#define 	AUDIO_REMOTE_S3_BUTTON		0x04
#define 	AUDIO_REMOTE_S4_BUTTON		0x05
#define 	AUDIO_REMOTE_S5_BUTTON		0x06
#define 	AUDIO_REMOTE_S6_BUTTON		0x07
#define 	AUDIO_REMOTE_S7_BUTTON		0x08
#define 	AUDIO_REMOTE_S8_BUTTON		0x09
#define 	AUDIO_REMOTE_S9_BUTTON		0x0A
#define 	AUDIO_REMOTE_S10_BUTTON		0x0B
#define 	AUDIO_REMOTE_S11_BUTTON		0x0C
#define 	AUDIO_REMOTE_S12_BUTTON		0x0D
#define		RESERVED_ACCESSORY_1		0x0E
#define 	RESERVED_ACCESSORY_2		0x0F
#define 	RESERVED_ACCESSORY_3		0x10
#define 	RESERVED_ACCESSORY_4		0x11
#define 	RESERVED_ACCESSORY_5		0x12
#define		AUDIO_DEICE_TYPE_2			0x13
#define		PHONE_POWERED_DEVICE		0X14
#define		TTY_CONVERTER				0x15
#define		UART_CABLE					0x16
#define 	CEA936A_TYPE_1_CHARGER		0x17
#define		FACTORY_MODE_BOOT_OFF_USB	0x18
#define 	FACTORY_MODE_BOOT_ON_USB	0x19
#define		AUDIO_VEDIO_CABLE			0x1A
#define		CEA936A_TYPE_2_CHARGER		0x1B
#define		FACTORY_MODE_BOOT_OFF_UART	0x1C
#define 	FACTORY_MODE_BOOT_ON_UART	0x1D
#define 	AUDIO_DEVICE_TYPE_1			0x1E
#define		USB_OR_ACCESSORY_DETACH		0x1F

//CR8 : Timing Set 1 Register
#define 	DEVICE_WAKE_UP_TIME_MASK	0x0F
#define 	KEY_PRESS_TIME_MASK			0xF0

#define		KEY_PRESS_TIME_300MS		0x20
#define 	KEY_PRESS_TIME_700MS		0x60
#define 	KEY_PRESS_TIME_1S			0x90

//CR9 : Timing Set 2 Register
#define		LONGKEY_PRESS_TIME_MASK		0x0F
#define		SWITCHING_TIME_MASK			0xF0

#define		LONGKEY_PRESS_TIME_1S		0x07
#define 	LONGKEY_PRESS_TIME_1_5S		0x0C

//CRA : Device Type 1 Register
#define		CRA_AUDIO_TYPE1             (0x1 <<0)
#define		CRA_AUDIO_TYPE2             (0x1 <<1)
#define		CRA_USB                     (0x1 <<2)
#define		CRA_UART                    (0x1 <<3)
#define		CRA_CARKIT                  (0x1 <<4)
#define		CRA_USB_CHARGER             (0x1 <<5)
#define		CRA_DEDICATED_CHG           (0x1 <<6)
#define		CRA_USB_OTG                 (0x1 <<7)

//CRB : Device Type 2 Register
#define		CRB_JIG_USB_ON              (0x1 <<0)
#define		CRB_JIG_USB_OFF             (0x1 <<1)
#define		CRB_JIG_UART_ON             (0x1 <<2)
#define		CRB_JIG_UART_OFF            (0x1 <<3)
#define		CRB_PPD                     (0x1 <<4)
#define		CRB_TTY                     (0x1 <<5)
#define		CRB_AV                      (0x1 <<6)
//Factory mode cable detected
#define     CRB_JIG_USB                 (0x3 <<0)
#define     CRB_JIG_UART                (0x3 <<2)

//Device1, 2 Register's Device Not Connected value
#define     DEVICE_TYPE_NC              0x00

//CRC : Button 1 Register
#define		BUTTON_SEND_END				(0x1 <<0)
#define 	BUTTON_1	 				(0x1 <<1)
#define 	BUTTON_2 					(0x1 <<2)
#define 	BUTTON_3 					(0x1 <<3)
#define 	BUTTON_4 					(0x1 <<4)
#define 	BUTTON_5 					(0x1 <<5)
#define 	BUTTON_6 					(0x1 <<6)
#define 	BUTTON_7 					(0x1 <<7)

//CRD : Button 2 Register
#define 	BUTTON_8		 			(0x1 <<0)
#define 	BUTTON_9					(0x1 <<1)
#define 	BUTTON_10					(0x1 <<2)
#define 	BUTTON_11					(0x1 <<3)
#define 	BUTTON_12					(0x1 <<4)
#define 	BUTTON_ERROR				(0x1 <<5)
#define 	BUTTON_UNKNOW				(0x1 <<6)

#define USBSTATUS_UMS					0x0
#define USBSTATUS_SAMSUNG_KIES		0x1
#define USBSTATUS_MTPONLY 				0x2
#define USBSTATUS_ASKON 				0x4
#define USBSTATUS_VTP					0x8
#define USBSTATUS_ADB					0x10
#define USBSTATUS_DM					0x20
#define USBSTATUS_ACM					0x30

//#define VODA




typedef enum
{
	RID_USB_OTG_MODE,			/* 0 0 0 0 0 	GND 		USB OTG Mode              */
	RID_AUD_SEND_END_BTN, 		/* 0 0 0 0 1 	2K		Audio Send_End Button*/
	RID_AUD_REMOTE_S1_BTN,		/* 0 0 0 1 0 	2.604K		Audio Remote S1 Button */
	RID_AUD_REMOTE_S2_BTN,		/* 0 0 0 1 1 	3.208K		Audio Remote S2 Button                         */
	RID_AUD_REMOTE_S3_BTN,		/* 0 0 1 0 0 	4.014K		Audio Remote S3 Button */
	RID_AUD_REMOTE_S4_BTN,		/* 0 0 1 0 1 	4.82K		Audio Remote S4 Button */
	RID_AUD_REMOTE_S5_BTN,		/* 0 0 1 1 0 	6.03K		Audio Remote S5 Button */
	RID_AUD_REMOTE_S6_BTN,		/* 0 0 1 1 1 	8.03K		Audio Remote S6 Button */
	RID_AUD_REMOTE_S7_BTN,		/* 0 1 0 0 0 	10.03K		Audio Remote S7 Button */
	RID_AUD_REMOTE_S8_BTN,		/* 0 1 0 0 1 	12.03K		Audio Remote S8 Button */
	RID_AUD_REMOTE_S9_BTN,		/* 0 1 0 1 0 	14.46K		Audio Remote S9 Button */
	RID_AUD_REMOTE_S10_BTN,		/* 0 1 0 1 1 	17.26K		Audio Remote S10 Button */
	RID_AUD_REMOTE_S11_BTN,		/* 0 1 1 0 0 	20.5K		Audio Remote S11 Button */
	RID_AUD_REMOTE_S12_BTN,		/* 0 1 1 0 1 	24.07K		Audio Remote S12 Button */
	RID_RESERVED_1,				/* 0 1 1 1 0 	28.7K		Reserved Accessory #1 */
	RID_RESERVED_2,				/* 0 1 1 1 1 	34K 		Reserved Accessory #2 */
	RID_RESERVED_3,				/* 1 0 0 0 0 	40.2K		Reserved Accessory #3 */
	RID_RESERVED_4,				/* 1 0 0 0 1 	49.9K		Reserved Accessory #4 */
	RID_RESERVED_5,				/* 1 0 0 1 0 	64.9K		Reserved Accessory #5 */
	RID_AUD_DEV_TY_2,			/* 1 0 0 1 1 	80.07K		Audio Device Type 2 */
	RID_PHONE_PWD_DEV,			/* 1 0 1 0 0 	102K		Phone Powered Device */
	RID_TTY_CONVERTER,			/* 1 0 1 0 1 	121K		TTY Converter */
	RID_UART_CABLE,				/* 1 0 1 1 0 	150K		UART Cable */
	RID_CEA936A_TY_1,			/* 1 0 1 1 1 	200K		CEA936A Type-1 Charger(1) */
	RID_FM_BOOT_OFF_USB,		/* 1 1 0 0 0 	255K		Factory Mode Boot OFF-USB */
	RID_FM_BOOT_ON_USB,			/* 1 1 0 0 1 	301K		Factory Mode Boot ON-USB */
	RID_AUD_VDO_CABLE,			/* 1 1 0 1 0 	365K		Audio/Video Cable */
	RID_CEA936A_TY_2,			/* 1 1 0 1 1 	442K		CEA936A Type-2 Charger(1) */
	RID_FM_BOOT_OFF_UART,		/* 1 1 1 0 0 	523K		Factory Mode Boot OFF-UART */
	RID_FM_BOOT_ON_UART,		/* 1 1 1 0 1 	619K		Factory Mode Boot ON-UART */
	RID_AUD_DEV_TY_1_REMOTE,	/* 1 1 1 1 0 	1000.07K	Audio Device Type 1 with Remote(1) */
	RID_AUD_DEV_TY_1_SEND = RID_AUD_DEV_TY_1_REMOTE ,		/* 1 1 1 1 0 	1002K		Audio Device Type 1 / Only Send-End(2) */
	RID_USB_MODE,				/* 1 1 1 1 1 	Open		USB Mode, Dedicated Charger or Accessory Detach */
	RID_MAX

}FSA9480_RID_ENUM_TYPE;

typedef enum
{
	FSA9480_REG_DEVICE_ID =0x01,
	FSA9480_REG_CONTROL=0x02,
	FSA9480_REG_INT_1=0x03,
	FSA9480_REG_INT_2=0x04,
	FSA9480_REG_IMASK_1=0x05,
	FSA9480_REG_IMASK_2=0x06,
	FSA9480_REG_ADC=0x07,
	FSA9480_REG_TINING_1=0x08,
	FSA9480_REG_TIMING_2=0x09,
	FSA9480_REG_DEV_TY_1=0x0A,
	FSA9480_REG_DEV_TY_2=0x0B,
	FSA9480_REG_BTN_1=0x0C,
	FSA9480_REG_BTN_2=0x0D,
	FSA9480_REG_CAR_KIT=0x0E,
	FSA9480_REG_CAR_KIT_INT_1=0x0F,
	FSA9480_REG_CAR_KIT_INT_2=0x10,
	FSA9480_REG_CAR_KIT_IMASK_1=0x11,
	FSA9480_REG_CAR_KIT_IMASK_2=0x12,
	FSA9480_REG_MANUAL_SW_1=0x13,
	FSA9480_REG_MANUAL_SW_2=0x14
}FSA9480_REG_ADDR;

typedef enum
{
	FSA9480_DEV_TY1_AUD_TY1 	= 0x01,
	FSA9480_DEV_TY1_AUD_TY2 	= 0x02,
	FSA9480_DEV_TY1_USB 		= 0x04,
	FSA9480_DEV_TY1_UART 		= 0x08,
	FSA9480_DEV_TY1_CAR_KIT 	= 0x10,
	FSA9480_DEV_TY1_USB_CHG 	= 0x20,
	FSA9480_DEV_TY1_DED_CHG 	= 0x40,
	FSA9480_DEV_TY1_USB_OTG = 0x80,
}FSA9480_DEV_TY1_TYPE;

typedef enum
{
	FSA9480_DEV_TY2_JIG_USB_ON	 	= 0x01,
	FSA9480_DEV_TY2_JIG_USB_OFF 	= 0x02,
	FSA9480_DEV_TY2_JIG_UART_ON	= 0x04,
	FSA9480_DEV_TY2_JIG_UART_OFF	= 0x08,
	FSA9480_DEV_TY2_PDD	= 0x10,
	FSA9480_DEV_TY2_TTY	= 0x20,
	FSA9480_DEV_TY2_AV	= 0x40,
}FSA9480_DEV_TY2_TYPE;

typedef enum
{
	FSA9480_INT1_ATTACH		=	0x01,
	FSA9480_INT1_DETACH 	=	0x02,
	FSA9480_INT1_KP 		= 	0x04,
	FSA9480_INT1_LKP 		= 	0x08,
	FSA9480_INT1_LKR 		= 	0x10,
	FSA9480_INT1_OVP_EN 	= 	0x20,
	FSA9480_INT1_OCP_EN 	= 	0x40,
	FSA9480_INT1_OVP_OCP_DIS= 	0x80
}FSA9480_INT1_TYPE;

typedef enum
{
	USB_SW_AP,
	USB_SW_CP
} USB_SWITCH_MODE;

typedef enum
{
	UART_SW_AP,
	UART_SW_CP
} UART_SWITCH_MODE;

typedef enum
{
	CONNECTIVITY_NV_USB_SW = 0,		// BIT 0
	CONNECTIVITY_NV_UART_SW = 1,	// BIT 1
	CONNECTIVITY_NV_ASK_ON = 2,	// BIT 2
	CONNECTIVITY_NV_DATA = 3, 		// BIT 4-7
	CONNECTIVITY_NV_DIAG = 4, 		// BIT 8-11
	CONNECTIVITY_NV_WINC = 5, 		// BIT 12-15
	CONNECTIVITY_NV_USB = 6, 		// BIT 16-19
	CONNECTIVITY_NV_MAX = 7, 		// BIT 16-19
}DRV_CONNECTIVITY_NV_TYPE;


typedef enum {
	AP_USB_MODE,
	AP_UART_MODE,
	CP_USB_MODE,
	CP_UART_MODE,
}Usb_Uart_Sw_Mode_type;


