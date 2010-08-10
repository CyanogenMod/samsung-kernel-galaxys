#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <plat/pm.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include <plat/s5pc110.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>
#include <mach/param.h>
#include "fsa9480_i2c.h"
#include <linux/syscalls.h> //denis
#include <linux/fcntl.h> //denis
#include <asm/uaccess.h> //denis
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <mach/max8998_function.h>
#include <linux/switch.h>

extern void otg_phy_init(void);
extern void otg_phy_off(void);

extern struct device *switch_dev;
extern int askonstatus;
extern int inaskonstatus;
extern int BOOTUP;

int mtp_mode_on = 0;



#define FSA9480_UART 	1
#define FSA9480_USB 	2

#define log_usb_disable 	0
#define log_usb_enable 		1
#define log_usb_active		2

static u8 MicroUSBStatus=0;
static u8 MicroJigUSBOnStatus=0;
static u8 MicroJigUSBOffStatus=0;
static u8 MicroJigUARTOffStatus=0;
static u8 MicroJigUARTOnStatus=0;
u8 MicroTAstatus=0;


#define FSA9480UCX		0x4A

/* switch selection */
#define USB_SEL_MASK  				(1 << 0)
#define UART_SEL_MASK				(1 << 1)
#define USB_SAMSUNG_KIES_MASK		(1 << 2)
#define USB_UMS_MASK				(1 << 3)
#define USB_MTP_MASK				(1 << 4)
#define USB_VTP_MASK				(1 << 5)
#define USB_ASKON_MASK				(1 << 6)

#define DRIVER_NAME  "usb_mass_storage"

static unsigned short fsa9480_normal_i2c[] = {I2C_CLIENT_END };
static unsigned short fsa9480_ignore[] = { I2C_CLIENT_END };
#ifdef CONFIG_JUPITER_VER_B4
static unsigned short fsa9480_i2c_probe[] = { 6, FSA9480UCX >> 1, I2C_CLIENT_END };
#else
static unsigned short fsa9480_i2c_probe[] = { 7, FSA9480UCX >> 1, I2C_CLIENT_END };
#endif

static u8 fsa9480_device1 = 0, fsa9480_device2 = 0, fsa9480_adc = 0;

static struct i2c_client fsa9480_i2c_client;
struct i2c_driver fsa9480_i2c_driver;

int usb_path = 0;
static int usb_power = 2;
static int usb_state = 0;
int log_via_usb = log_usb_disable; //denis


static u8 testvalue=0;
static u8 testvalue1=0;

static struct timer_list fsa9480_init_timer;
static wait_queue_head_t usb_detect_waitq;
static struct workqueue_struct *fsa9480_workqueue;
static struct work_struct fsa9480_work;
unsigned short fsa9480_probe=1;

struct switch_dev indicator_dev;

static struct i2c_client_address_data fsa9480_addr_data = {
	.normal_i2c = fsa9480_normal_i2c,
	.ignore     = fsa9480_ignore,
	.probe      = fsa9480_i2c_probe,
};

extern int currentusbstatus;
byte switchinginitvalue[12];
byte uart_message[6];
byte usb_message[5];
int factoryresetstatus=0;
extern int usb_mtp_select(int disable);
extern int usb_switch_select(int enable);
extern int askon_switch_select(int enable);
extern unsigned int charging_mode_get(void);


int samsung_kies_mtp_mode_flag;

void FSA9480_Enable_CP_USB(u8 enable);

//struct timer_list microusb_timer;
struct delayed_work micorusb_init_work;

FSA9480_DEV_TY1_TYPE FSA9480_Get_DEV_TYP1(void)
{
	return fsa9480_device1;
}
EXPORT_SYMBOL(FSA9480_Get_DEV_TYP1);


u8 FSA9480_Get_JIG_Status(void)
{
	if(MicroJigUSBOnStatus | MicroJigUSBOffStatus | MicroJigUARTOnStatus | MicroJigUARTOffStatus)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(FSA9480_Get_JIG_Status);

u8 FSA9480_Get_FPM_Status(void)
{
	if(fsa9480_adc == RID_FM_BOOT_ON_UART)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(FSA9480_Get_FPM_Status);

u8 FSA9480_Get_USB_Status(void)
{
	if( MicroUSBStatus | MicroJigUSBOnStatus | MicroJigUSBOffStatus )
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(FSA9480_Get_USB_Status);


u8 FSA9480_Get_TA_Status(void)
{
	if(MicroTAstatus)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(FSA9480_Get_TA_Status);

u8 FSA9480_Get_JIG_UART_Status(void)
{
	if(MicroJigUARTOnStatus | MicroJigUARTOffStatus)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(FSA9480_Get_JIG_UART_Status);


int get_usb_cable_state(void)
{
    //DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	return usb_state;
}

static int fsa9480_read(struct i2c_client *client, u8 reg, u8 *data)
{

	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

EXPORT_SYMBOL(fsa9480_read);


static int fsa9480_write(struct i2c_client *client, u8 reg, u8 data)
{

	int ret;
	u8 buf[2];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = data;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}

EXPORT_SYMBOL(fsa9480_write);


void ap_usb_power_on(int set_vaue)
{
 	byte reg_value=0;
	byte reg_address=0x0D;

	if(set_vaue){
		Get_MAX8998_PM_ADDR(reg_address, &reg_value, 1); // read 0x0D register
		reg_value = reg_value | (0x1 << 7);
		Set_MAX8998_PM_ADDR(reg_address,&reg_value,1);
		//printk("[ap_usb_power_on]AP USB Power ON, askon: %d, mtp : %d\n",askonstatus,mtp_mode_on);
			if(mtp_mode_on == 1) {
				samsung_kies_mtp_mode_flag = 1;
				//printk("************ [ap_usb_power_on] samsung_kies_mtp_mode_flag:%d, mtp:%d\n", samsung_kies_mtp_mode_flag, mtp_mode_on);
			}
			else {
				samsung_kies_mtp_mode_flag = 0;
				//printk("!!!!!!!!!!! [ap_usb_power_on]AP samsung_kies_mtp_mode_flag%d, mtp:%d\n",samsung_kies_mtp_mode_flag, mtp_mode_on);
			}
		}
	else{
		Get_MAX8998_PM_ADDR(reg_address, &reg_value, 1); // read 0x0D register
		reg_value = reg_value & ~(0x1 << 7);
		Set_MAX8998_PM_ADDR(reg_address,&reg_value,1);
		//printk("[ap_usb_power_on]AP USB Power OFF, askon: %d, mtp : %d\n",askonstatus,mtp_mode_on);
		}
		
}
void usb_api_set_usb_switch(USB_SWITCH_MODE usb_switch)
{

	if(usb_switch == USB_SW_CP)
	{
		//USB_SEL GPIO Set High => CP USB enable
		#ifdef CONFIG_JUPITER_VER_B4
		gpio_set_value(GPIO_USB_SEL, 1);
		#endif
		FSA9480_Enable_CP_USB(1);
	}
	else
	{
		//USB_SEL GPIO Set Low => AP USB enable
		#ifdef CONFIG_JUPITER_VER_B4
		gpio_set_value(GPIO_USB_SEL, 0);
		#endif
		FSA9480_Enable_CP_USB(0);

		//reconnect AP USB
		//usb_api_reconnect();
	}

}


void Ap_Cp_Switch_Config(u16 ap_cp_mode)
{
	switch (ap_cp_mode) {
		case AP_USB_MODE:
			usb_path=1;
			usb_api_set_usb_switch(USB_SW_AP);
			break;
		case AP_UART_MODE:
			gpio_set_value(GPIO_UART_SEL, 1);
			break;
		case CP_USB_MODE:
			usb_path=2;
			usb_api_set_usb_switch(USB_SW_CP);
			break;
		case CP_UART_MODE:
			gpio_set_value(GPIO_UART_SEL, 0);			
			break;
		default:
			break;//printk("Ap_Cp_Switch_Config error");
	}
		
}


/* MODEM USB_SEL Pin control */
/* 1 : PDA, 2 : MODEM */
#define SWITCH_PDA			1
#define SWITCH_MODEM		2

void usb_switching_value_update(int value)
{
	if(value == SWITCH_PDA)
		usb_message[0] = 'A';
	else
		usb_message[0] = 'C';		

	usb_message[1] = 'P';
	usb_message[2] = 'U';
	usb_message[3] = 'S';
	usb_message[4] = 'B';

}

void uart_switching_value_update(int value)
{
	if(value == SWITCH_PDA)
		uart_message[0] = 'A';
	else
		uart_message[0] = 'C';

	uart_message[1] = 'P';
	uart_message[2] = 'U';
	uart_message[3] = 'A';
	uart_message[4] = 'R';
	uart_message[5] = 'T';

}

void switching_value_update(void)
{
	int index;
	
	for(index=0;index<5;index++)
		switchinginitvalue[index] = usb_message[index];

	for(index=5;index<11;index++)
		switchinginitvalue[index] = uart_message[index-5];

	switchinginitvalue[11] = '\0';

}


static ssize_t factoryreset_value_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{

	if(strncmp(buf, "FACTORYRESET", 12) == 0 || strncmp(buf, "factoryreset", 12) == 0)
		factoryresetstatus = 0xAE;

	return size;
}

static DEVICE_ATTR(FactoryResetValue, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, NULL, factoryreset_value_store);


/* for sysfs control (/sys/class/sec/switch/usb_sel) */
static ssize_t usb_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 i, pData;

	sprintf(buf, "USB Switch : %s\n", usb_path==SWITCH_PDA?"PDA":"MODEM");

    	for(i = 0; i <= 0x14; i++) 
		fsa9480_read(&fsa9480_i2c_client, i, &pData);

	return sprintf(buf, "%s\n", buf);
}

void usb_switch_mode(int sel)
{
    DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	  
	if (sel == SWITCH_PDA)
	{
		DEBUG_FSA9480("[FSA9480] Path : PDA\n");
		Ap_Cp_Switch_Config(AP_USB_MODE);
	} else if (sel == SWITCH_MODEM) 
	{
		DEBUG_FSA9480("[FSA9480] Path : MODEM\n");
		Ap_Cp_Switch_Config(CP_USB_MODE);
	} else
		DEBUG_FSA9480("[FSA9480] Invalid mode...\n");
}
EXPORT_SYMBOL(usb_switch_mode);

void microusb_uart_status(int status)
{
	int switch_sel;
	int uart_sel;
	int usb_sel;


	if(!FSA9480_Get_JIG_UART_Status())	
		return;
	
	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	uart_sel = (switch_sel & (int)(UART_SEL_MASK)) >> 1;
	usb_sel = switch_sel & (int)(USB_SEL_MASK);


	if(status){
		if(uart_sel)
			Ap_Cp_Switch_Config(AP_UART_MODE);	
		else
			Ap_Cp_Switch_Config(CP_UART_MODE);	
		}
	else{
		if(!usb_sel)
			Ap_Cp_Switch_Config(AP_USB_MODE);
		}

}

static ssize_t usb_sel_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
    DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	int switch_sel;

	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	if(strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0) {
		usb_switch_mode(SWITCH_PDA);
		usb_switching_value_update(SWITCH_PDA);
		switch_sel |= USB_SEL_MASK;
	}

	if(strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {
		usb_switch_mode(SWITCH_MODEM);
		usb_switching_value_update(SWITCH_MODEM);		
		switch_sel &= ~USB_SEL_MASK;
	}

	switching_value_update();

	if (sec_set_param_value)
		sec_set_param_value(__SWITCH_SEL, &switch_sel);

	microusb_uart_status(0);

	return size;
}

static DEVICE_ATTR(usb_sel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, usb_sel_show, usb_sel_store);


int connectivity_switching_init_state=0;
void usb_switch_state(void)
{
	int switch_sel;
	int usb_sel;

	if(!connectivity_switching_init_state)
		return;

	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	usb_sel = switch_sel & (int)(USB_SEL_MASK);
	
	if(usb_sel)
	{
		usb_switch_mode(SWITCH_PDA);
		usb_switching_value_update(SWITCH_PDA);
	}
	else
	{
		usb_switch_mode(SWITCH_MODEM);
		usb_switching_value_update(SWITCH_MODEM);
	}
}


void uart_insert_switch_state(void)
{

	if(!connectivity_switching_init_state)
		return;
	
	int switch_sel;
	int usb_sel;
	
	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	usb_sel = switch_sel & (int)(USB_SEL_MASK);

	if(!usb_sel)
		Ap_Cp_Switch_Config(AP_USB_MODE);

}

void uart_remove_switch_state(void)
{
	int switch_sel;
	int usb_sel;

	if(!connectivity_switching_init_state)
		return;
	
	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	usb_sel = switch_sel & (int)(USB_SEL_MASK);

	if(usb_sel)
		Ap_Cp_Switch_Config(AP_USB_MODE);
	else
		Ap_Cp_Switch_Config(CP_USB_MODE);

}




/**********************************************************************
*    Name         : usb_state_show()
*    Description : for sysfs control (/sys/class/sec/switch/usb_state)
*                        return usb state using fsa9480's device1 and device2 register
*                        this function is used only when NPS want to check the usb cable's state.
*    Parameter   :
*                       
*                       
*    Return        : USB cable state's string
*                        USB_STATE_CONFIGURED is returned if usb cable is connected
***********************************************************************/
static ssize_t usb_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int cable_state;

	cable_state = get_usb_cable_state();

	sprintf(buf, "%s\n", (cable_state & (CRB_JIG_USB<<8 | CRA_USB<<0 ))?"USB_STATE_CONFIGURED":"USB_STATE_NOTCONFIGURED");

	return sprintf(buf, "%s\n", buf);
} 


/**********************************************************************
*    Name         : usb_state_store()
*    Description : for sysfs control (/sys/class/sec/switch/usb_state)
*                        noting to do.
*    Parameter   :
*                       
*                       
*    Return        : None
*
***********************************************************************/
static ssize_t usb_state_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
    DEBUG_FSA9480("[FSA9480]%s\n ", __func__);


	return 0;
}

/*sysfs for usb cable's state.*/
static DEVICE_ATTR(usb_state, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, usb_state_show, usb_state_store);


static int uart_current_owner = 1;
static ssize_t uart_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	if ( uart_current_owner )		
		return sprintf(buf, "%s[UART Switch] Current UART owner = PDA \n", buf);	
	else			
		return sprintf(buf, "%s[UART Switch] Current UART owner = MODEM \n", buf);
}

static ssize_t uart_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{	
	int switch_sel;

	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	if (strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0)	{		
		Ap_Cp_Switch_Config(AP_UART_MODE);
		uart_switching_value_update(SWITCH_PDA);
		uart_current_owner = 1;		
		switch_sel |= UART_SEL_MASK;
		//printk("[UART Switch] Path : PDA\n");	
	}	

	if (strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {		
		Ap_Cp_Switch_Config(CP_UART_MODE);
		uart_switching_value_update(SWITCH_MODEM);
		uart_current_owner = 0;		
		switch_sel &= ~UART_SEL_MASK;
		//printk("[UART Switch] Path : MODEM\n");	
	}

	switching_value_update();	

	if (sec_set_param_value)
		sec_set_param_value(__SWITCH_SEL, &switch_sel);

	return size;
}

static DEVICE_ATTR(uart_sel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, uart_switch_show, uart_switch_store);


//denis
static ssize_t DMport_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
		
		return sprintf(buf, "%s[UsbMenuSel test] ready!! \n", buf);	
}

static ssize_t DMport_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{	
#if 0
	int fd;
	int i;
	char dm_buf[] = "DM port test sucess\n";
	int old_fs = get_fs();
	set_fs(KERNEL_DS);
	
		if (strncmp(buf, "DMport", 5) == 0 || strncmp(buf, "dmport", 5) == 0) {		
			
				fd = sys_open("/dev/dm", O_RDWR, 0);
				if(fd < 0){
				//printk("Cannot open the file");
				return fd;}
				for(i=0;i<5;i++)
				{		
				sys_write(fd,dm_buf,sizeof(dm_buf));
				mdelay(1000);
				}
		sys_close(fd);
		set_fs(old_fs);				
		}

		if ((strncmp(buf, "logusb", 6) == 0)||(log_via_usb == log_usb_enable)) {		
			log_via_usb = log_usb_active;
				//printk("denis_test_prink_log_via_usb_1\n");
				mdelay(1000);
				//printk(KERN_INFO"%s: 21143 10baseT link beat good.\n", "denis_test");
			set_fs(old_fs);				
			}
		return size;
#else
		if (strncmp(buf, "ENABLE", 6) == 0)
		{
			usb_switch_select(USBSTATUS_DM);
		}

		if (strncmp(buf, "DISABLE", 6) == 0)
		{
			usb_switch_select(USBSTATUS_SAMSUNG_KIES);			
		}
		
	return size;
#endif
}


static DEVICE_ATTR(DMport, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, DMport_switch_show, DMport_switch_store);


//denis
static ssize_t DMlog_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
		
		return sprintf(buf, "%s[DMlog test] ready!! \n", buf);	
}

static ssize_t DMlog_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{
		if (strncmp(buf, "CPONLY", 6) == 0)
		{
			//printk("DMlog selection : CPONLY\n");
}

		if (strncmp(buf, "APONLY", 6) == 0)
		{
			//printk("DMlog selection : APONLY\n");
		}

		if (strncmp(buf, "CPAP", 4) == 0)
		{
			//printk("DMlog selection : AP+CP\n");
		}
		
		
	return size;
}


static DEVICE_ATTR(DMlog, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, DMlog_switch_show, DMlog_switch_store);


void UsbMenuSelStore(int sel)
{	
	int switch_sel;

	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);	

	if(sel == 0){
		switch_sel &= ~(int)USB_UMS_MASK;
		switch_sel &= ~(int)USB_MTP_MASK;
		switch_sel &= ~(int)USB_VTP_MASK;
		switch_sel &= ~(int)USB_ASKON_MASK;		
		switch_sel |= (int)USB_SAMSUNG_KIES_MASK;	
		}
	else if(sel == 1){
		switch_sel &= ~(int)USB_UMS_MASK;
		switch_sel &= ~(int)USB_SAMSUNG_KIES_MASK;
		switch_sel &= ~(int)USB_VTP_MASK;
		switch_sel &= ~(int)USB_ASKON_MASK;				
		switch_sel |= (int)USB_MTP_MASK;		
		}
	else if(sel == 2){
		switch_sel &= ~(int)USB_SAMSUNG_KIES_MASK;
		switch_sel &= ~(int)USB_MTP_MASK;
		switch_sel &= ~(int)USB_VTP_MASK;
		switch_sel &= ~(int)USB_ASKON_MASK;				
		switch_sel |= (int)USB_UMS_MASK;
		}
	else if(sel == 3){
		switch_sel &= ~(int)USB_UMS_MASK;
		switch_sel &= ~(int)USB_MTP_MASK;
		switch_sel &= ~(int)USB_SAMSUNG_KIES_MASK;
		switch_sel &= ~(int)USB_ASKON_MASK;				
		switch_sel |= (int)USB_VTP_MASK;	
		}
	else if(sel == 4){
		switch_sel &= ~(int)USB_UMS_MASK;
		switch_sel &= ~(int)USB_MTP_MASK;
		switch_sel &= ~(int)USB_VTP_MASK;
		switch_sel &= ~(int)USB_SAMSUNG_KIES_MASK;				
		switch_sel |= (int)USB_ASKON_MASK;	
		}
	
	if (sec_set_param_value)
		sec_set_param_value(__SWITCH_SEL, &switch_sel);
}

void PathSelStore(int sel)
{	
	int switch_sel;

	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);	

	if(sel == AP_USB_MODE){
		switch_sel |= USB_SEL_MASK;	
		}
	else if(sel == CP_USB_MODE){
		switch_sel &= ~USB_SEL_MASK;		
		}
	else if(sel == AP_UART_MODE){
		switch_sel |= UART_SEL_MASK;
		}
	else if(sel == CP_UART_MODE){
		switch_sel &= ~UART_SEL_MASK;	
		}
	
	if (sec_set_param_value)
		sec_set_param_value(__SWITCH_SEL, &switch_sel);
}



static ssize_t UsbMenuSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
		
		if (currentusbstatus == USBSTATUS_UMS) 
			return sprintf(buf, "%s[UsbMenuSel] UMS\n", buf);	
		
		else if (currentusbstatus == USBSTATUS_SAMSUNG_KIES) 
			return sprintf(buf, "%s[UsbMenuSel] ACM_MTP\n", buf);	
		
		else if (currentusbstatus == USBSTATUS_MTPONLY) 
			return sprintf(buf, "%s[UsbMenuSel] MTP\n", buf);	
		
		else if (currentusbstatus == USBSTATUS_ASKON) 
			return sprintf(buf, "%s[UsbMenuSel] ASK\n", buf);	
		
		else if (currentusbstatus == USBSTATUS_VTP) 
			return sprintf(buf, "%s[UsbMenuSel] VTP\n", buf);	
		
		else if (currentusbstatus == USBSTATUS_ADB) 
			return sprintf(buf, "%s[UsbMenuSel] ACM_ADB_UMS\n", buf);	
}


static ssize_t UsbMenuSel_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{		
	
		if (strncmp(buf, "KIES", 4) == 0)
		{
			UsbMenuSelStore(0);		
			usb_switch_select(USBSTATUS_SAMSUNG_KIES);
		}

		if (strncmp(buf, "MTP", 3) == 0)
		{
			UsbMenuSelStore(1);					
			usb_switch_select(USBSTATUS_MTPONLY);
		}
		
		if (strncmp(buf, "UMS", 3) == 0)
		{
			UsbMenuSelStore(2);							
			usb_switch_select(USBSTATUS_UMS);
		}

		if (strncmp(buf, "VTP", 3) == 0)
		{
			UsbMenuSelStore(3);							
			usb_switch_select(USBSTATUS_VTP);
		}

		if (strncmp(buf, "ASKON", 5) == 0)
		{		
			UsbMenuSelStore(4);									
			usb_switch_select(USBSTATUS_ASKON);			
		}


	return size;
}


static DEVICE_ATTR(UsbMenuSel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, UsbMenuSel_switch_show, UsbMenuSel_switch_store);

extern int inaskonstatus;
static ssize_t AskOnStatus_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(inaskonstatus)
		return sprintf(buf, "%s\n", "Blocking");
	else
		return sprintf(buf, "%s\n", "NonBlocking");
	
}


static ssize_t AskOnStatus_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{		
	
	return size;
}


static DEVICE_ATTR(AskOnStatus, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, AskOnStatus_switch_show, AskOnStatus_switch_store);

//denis
static ssize_t AskOnMenuSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
		
		return sprintf(buf, "%s[AskOnMenuSel] Port test ready!! \n", buf);	
}

static ssize_t AskOnMenuSel_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{		
	
		if (strncmp(buf, "KIES", 4) == 0)
		{
			askon_switch_select(USBSTATUS_SAMSUNG_KIES);
		}

		if (strncmp(buf, "MTP", 3) == 0)
		{
			askon_switch_select(USBSTATUS_MTPONLY);
		}
		
		if (strncmp(buf, "UMS", 3) == 0)
		{
			askon_switch_select(USBSTATUS_UMS);
		}

		if (strncmp(buf, "VTP", 3) == 0)
		{
			askon_switch_select(USBSTATUS_VTP);
		}

	return size;
}


static DEVICE_ATTR(AskOnMenuSel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, AskOnMenuSel_switch_show, AskOnMenuSel_switch_store);


static ssize_t Mtp_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
		
		return sprintf(buf, "%s[Mtp] MtpDeviceOn \n", buf);	
}

static ssize_t Mtp_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{		
	
		if (strncmp(buf, "Mtp", 3) == 0)
		{
			if(mtp_mode_on)
				{
				//printk("[Mtp_switch_store]AP USB power on. \n");
#ifdef VODA
				askon_switch_select(USBSTATUS_SAMSUNG_KIES);
#endif
				ap_usb_power_on(1);
			}
		}
	else if (strncmp(buf, "OFF", 3) == 0)
		{
				//printk("[Mtp_switch_store]AP USB power off. \n");
				usb_mtp_select(1);
		}
	return size;
}


static DEVICE_ATTR(Mtp, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, Mtp_switch_show, Mtp_switch_store);

static int mtpinitstatus=0;
static ssize_t MtpInitStatusSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	if(mtpinitstatus == 2)
		return sprintf(buf, "%s\n", "START");
	else
		return sprintf(buf, "%s\n", "STOP");

}

static ssize_t MtpInitStatusSel_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{
	mtpinitstatus = mtpinitstatus + 1;

	return size;
}


static DEVICE_ATTR(MtpInitStatusSel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, MtpInitStatusSel_switch_show, MtpInitStatusSel_switch_store);

static int askinitstatus=0;
static ssize_t AskInitStatusSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	if(askinitstatus == 2)
		return sprintf(buf, "%s\n", "START");
	else
		return sprintf(buf, "%s\n", "STOP");

}

static ssize_t AskInitStatusSel_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{
	askinitstatus = askinitstatus + 1;

	return size;
}


static DEVICE_ATTR(AskInitStatusSel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, AskInitStatusSel_switch_show, AskInitStatusSel_switch_store);


static ssize_t get_SwitchingInitValue(struct device *dev, struct device_attribute *attr,	const char *buf)
{	
	return snprintf(buf, 12, "%d\n", switchinginitvalue);
}

static DEVICE_ATTR(SwitchingInitValue, S_IRUGO, get_SwitchingInitValue, NULL);


int  FSA9480_PMIC_CP_USB(void)
{
	int switch_sel;
	int usb_sel;


	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	usb_sel = switch_sel & (int)(USB_SEL_MASK);

	return usb_sel;

}

int check_reg=0;
extern unsigned int HWREV;

void FSA9480_Enable_CP_USB(u8 enable)
{
	u8 data = 0;
	byte reg_value=0;
	byte reg_address=0x0D;

	if(enable)
	{
		//printk("[FSA9480_Enable_CP_USB] Enable CP USB\n");
		mdelay(10);
		Get_MAX8998_PM_ADDR(reg_address, &reg_value, 1); // read 0x0D register
		check_reg = reg_value;
		reg_value = ((0x2<<5)|reg_value);
		check_reg = reg_value;
		Set_MAX8998_PM_ADDR(reg_address,&reg_value,1);
		check_reg = reg_value;
			
		mdelay(10);
#ifdef CONFIG_S5PC110_T959_BOARD // not Kepler
		//printk("HWREV is 0x%x\n", HWREV);		
		if ((HWREV == 0x0d)||(HWREV == 0x0e)||(HWREV == 0x0f)||(HWREV == 0x10))
		fsa9480_write(&fsa9480_i2c_client, REGISTER_MANUALSW1, 0x48); // D+/- switching by Audio_L/R in HW04
		else
#endif
		fsa9480_write(&fsa9480_i2c_client, REGISTER_MANUALSW1, 0x90);	// D+/- switching by V_Audio_L/R in HW03

		mdelay(10);
		fsa9480_write(&fsa9480_i2c_client, REGISTER_CONTROL, 0x1A);	//manual switching

	}
	else
	{
		//printk("[FSA9480_Enable_AP_USB] Enable AP USB\n");
		Get_MAX8998_PM_ADDR(reg_address, &reg_value, 1); // read 0x0D register

		if(askonstatus||mtp_mode_on)
		ap_usb_power_on(0);
		else
		ap_usb_power_on(1);
		mdelay(10);
		fsa9480_write(&fsa9480_i2c_client, REGISTER_CONTROL, 0x1E);	

	#if 0	
	mdelay(10);
	reg_value = 0x40;
	Set_MAX8998_PM_ADDR(reg_address,&reg_value,1);
		
	mdelay(10);
	fsa9480_write(&fsa9480_i2c_client, REGISTER_MANUALSW1, 0x90);	
	
	mdelay(10);
	fsa9480_write(&fsa9480_i2c_client, REGISTER_CONTROL, 0x1A); 
	#endif
	}

}

void FSA9480_Enable_SPK(u8 enable)
{
	u8 data = 0;
	byte reg_value=0;
	byte reg_address=0x0D;

	if(enable)
	{
		DEBUG_FSA9480("FSA9480_Enable_SPK --- enable\n");
		msleep(10);
		Get_MAX8998_PM_ADDR(reg_address, &reg_value, 1); // read 0x0D register
		check_reg = reg_value;
		reg_value = ((0x2<<5)|reg_value);
		check_reg = reg_value;
		Set_MAX8998_PM_ADDR(reg_address,&reg_value,1);
		check_reg = reg_value;
			
		msleep(10);
		fsa9480_write(&fsa9480_i2c_client, REGISTER_MANUALSW1, 0x90);	// D+/- switching by V_Audio_L/R in HW03
		msleep(10);
		fsa9480_write(&fsa9480_i2c_client, REGISTER_CONTROL, 0x1A);	//manual switching

	}
}


void UsbIndicator(u8 state)
{
	switch_set_state(&indicator_dev, state);
}

extern void askon_gadget_disconnect(void);
extern int s3c_usb_cable(int connected);
extern void vps_status_change(int status);
extern void car_vps_status_change(int status);

void FSA9480_ProcessDevice(u8 dev1, u8 dev2, u8 attach)
{

	DEBUG_FSA9480("FSA9480_ProcessDevice function!!!!\n");
	// [[ junghyunseok edit for attach/dettach debug 20010617
	printk("[FSA9480]FSA INTR = dev1 : 0x%x, dev2 : 0x%x , Attach : 0x%x\n",dev1, dev2, attach);
	if(dev1)
	{
		switch(dev1)
		{
			case FSA9480_DEV_TY1_AUD_TY1:
				DEBUG_FSA9480("Audio Type1 ");
				if(attach & FSA9480_INT1_ATTACH)
					DEBUG_FSA9480("FSA9480_DEV_TY1_AUD_TY1 --- ATTACH\n");
				else
					DEBUG_FSA9480("FSA9480_DEV_TY1_AUD_TY1 --- DETACH\n");
			break;

			case FSA9480_DEV_TY1_AUD_TY2:
				DEBUG_FSA9480("Audio Type2 ");
			break;

			case FSA9480_DEV_TY1_USB:
			{
				DEBUG_FSA9480("USB attach or detach: %d",attach);
				if(attach & FSA9480_INT1_ATTACH)
				{
					DEBUG_FSA9480("FSA9480_DEV_TY1_USB --- ATTACH\n");
					MicroUSBStatus = 1;
					log_via_usb = log_usb_enable; //denis
					#if 0
					if(connectivity_switching_init_state)
						s3c_usb_cable(1);		
					#endif		
					usb_switch_state();
					if(!askonstatus)
						UsbIndicator(1);
					else
						inaskonstatus = 0;				
				}
				else if(attach & FSA9480_INT1_DETACH)
				{	
					MicroUSBStatus = 0;
					inaskonstatus = 0;
					#if 0
					if(connectivity_switching_init_state)
						s3c_usb_cable(0);							
					#endif							
					UsbIndicator(0);
					askon_gadget_disconnect();
					DEBUG_FSA9480("FSA9480_DEV_TY1_USB --- DETACH\n");
				}
			}
			break;

			case FSA9480_DEV_TY1_UART:
				DEBUG_FSA9480("UART \n");
			break;

			case FSA9480_DEV_TY1_CAR_KIT:
				DEBUG_FSA9480("Carkit \n");
			break;

			case FSA9480_DEV_TY1_USB_CHG:
				DEBUG_FSA9480("USB \n");
			break;

			case FSA9480_DEV_TY1_DED_CHG:
				DEBUG_FSA9480("Dedicated Charger \n");
			break;

			case FSA9480_DEV_TY1_USB_OTG:
				DEBUG_FSA9480("USB OTG \n");
			break;

			default:
				DEBUG_FSA9480("Unknown device \n");
			break;
		}

	}

	if(dev2)
	{
		switch(dev2)
		{
			case FSA9480_DEV_TY2_JIG_USB_ON:
				DEBUG_FSA9480("JIG USB ON attach or detach: %d",attach);
				if(attach & FSA9480_INT1_ATTACH)
				{
					DEBUG_FSA9480("FSA9480_DEV_TY2_JIG_USB_ON --- ATTACH\n");
					MicroJigUSBOnStatus = 1;
					#if 0	
					if(connectivity_switching_init_state)
						s3c_usb_cable(1);							
					#endif							
					usb_switch_state();
					if(!askonstatus)
						UsbIndicator(1);
					else
						inaskonstatus = 0;				

				}
				else if(attach & FSA9480_INT1_DETACH)
				{
					DEBUG_FSA9480("FSA9480_DEV_TY2_JIG_USB_ON --- DETACH\n");
					MicroJigUSBOnStatus = 0;
					inaskonstatus = 0;
#if 0	
					if(connectivity_switching_init_state)
						s3c_usb_cable(0);							
#endif		
					UsbIndicator(0);
					askon_gadget_disconnect();					
				}
			break;

			case FSA9480_DEV_TY2_JIG_USB_OFF:
				if(attach & FSA9480_INT1_ATTACH)
				{
					DEBUG_FSA9480("FSA9480_DEV_TY2_JIG_USB_OFF --- ATTACH\n");
					MicroJigUSBOffStatus = 1;
#if 0	
					if(connectivity_switching_init_state)
						s3c_usb_cable(1);							
#endif		
					usb_switch_state();
					if(!askonstatus)
						UsbIndicator(1);
					else
						inaskonstatus = 0;									
				}
				else if(attach & FSA9480_INT1_DETACH)
				{
					DEBUG_FSA9480("FSA9480_DEV_TY2_JIG_USB_OFF --- DETACH\n");
					MicroJigUSBOffStatus = 0;
					inaskonstatus = 0;
#if 0	
					if(connectivity_switching_init_state)
						s3c_usb_cable(0);
#endif				
					UsbIndicator(0);
					askon_gadget_disconnect();					
				}
				DEBUG_FSA9480("JIG USB OFF \n");
			break;

			case FSA9480_DEV_TY2_JIG_UART_ON:
				if(attach & FSA9480_INT1_ATTACH)
				{
					DEBUG_FSA9480("FSA9480_DEV_TY2_JIG_UART_ON --- ATTACH\n");
					MicroJigUARTOnStatus = 1;
					car_vps_status_change(1);
				}
				else
				{
					DEBUG_FSA9480("FSA9480_DEV_TY2_JIG_UART_ON --- DETACH\n");
					MicroJigUARTOnStatus = 0;
					car_vps_status_change(0);
				}
				DEBUG_FSA9480("JIG UART ON \n");
			break;

			case FSA9480_DEV_TY2_JIG_UART_OFF:
				if(attach & FSA9480_INT1_ATTACH)
				{
					DEBUG_FSA9480("FSA9480_DEV_TY2_JIG_UART_OFF --- ATTACH\n");
					MicroJigUARTOffStatus = 1;
					uart_insert_switch_state();					
				}
				else
				{
					DEBUG_FSA9480("FSA9480_DEV_TY2_JIG_UART_OFF --- DETACH\n");
					MicroJigUARTOffStatus = 0;
					uart_remove_switch_state();										
				}
				DEBUG_FSA9480("JIT UART OFF \n");
			break;

			case FSA9480_DEV_TY2_PDD:
				DEBUG_FSA9480("PPD \n");
			break;

			case FSA9480_DEV_TY2_TTY:
				DEBUG_FSA9480("TTY \n");
			break;

			case FSA9480_DEV_TY2_AV:
			{
				DEBUG_FSA9480("AudioVideo \n");
				if(attach & FSA9480_INT1_ATTACH){
					DEBUG_FSA9480("FSA9480_DEV_TY2_AV --- ATTACH\n");
#if defined CONFIG_S5PC110_T959_BOARD
					if(1)
#elif defined CONFIG_S5PC110_KEPLER_BOARD
					if (1)
#else
					if (HWREV == 0x0d)
#endif
						{
						DEBUG_FSA9480("FSA9480_enable_spk\n");						
						vps_status_change(1);
						FSA9480_Enable_SPK(1);
						}
					}
				else{
					DEBUG_FSA9480("FSA9480_DEV_TY2_AV --- DETACH\n");
#if defined CONFIG_S5PC110_T959_BOARD
					if(1)
#elif defined CONFIG_S5PC110_KEPLER_BOARD
					if (1)
#else
					if (HWREV == 0x0d)
#endif
						vps_status_change(0);
			            }
			}
			break;

			default:
				DEBUG_FSA9480("Unknown device \n");
			break;
		}
	}

}

EXPORT_SYMBOL(FSA9480_ProcessDevice);

void FSA9480_ReadIntRegister(void)
{

	u8 interrupt1 , device1, device2, temp;
	u8 data = 0;
	int i;

	DEBUG_FSA9480("FSA9480_ReadIntRegister function!!!!\n");


	 fsa9480_read(&fsa9480_i2c_client, REGISTER_INTERRUPT1, &interrupt1);

 	 msleep(5);
	
	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICETYPE1, &device1);

 	 msleep(5);

	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICETYPE2, &device2);

	usb_state = (device2 << 8) | (device1 << 0);

	if(interrupt1 & FSA9480_INT1_ATTACH)
	{
		fsa9480_device1 = device1;
		fsa9480_device2 = device2;
		if(fsa9480_device1 != FSA9480_DEV_TY1_DED_CHG){
			DEBUG_FSA9480("FSA9480_enable LDO8\n");
		s3c_usb_cable(1);//mkh
			}
		if(fsa9480_device1&FSA9480_DEV_TY1_CAR_KIT)
		{
			msleep(5);
			fsa9480_write(&fsa9480_i2c_client, REGISTER_CARKITSTATUS, 0x02);

			msleep(5);
			fsa9480_read(&fsa9480_i2c_client, REGISTER_CARKITINT1, &temp);
		}
	}

 	 msleep(5);


	 fsa9480_write(&fsa9480_i2c_client, REGISTER_CONTROL, 0x1E);	 

	 fsa9480_write(&fsa9480_i2c_client, REGISTER_INTERRUPTMASK1, 0xFC);

	 FSA9480_ProcessDevice(fsa9480_device1, fsa9480_device2, interrupt1);	 

	if(interrupt1 & FSA9480_INT1_DETACH)
	{
		if(fsa9480_device1 != FSA9480_DEV_TY1_DED_CHG){
		DEBUG_FSA9480("FSA9480_disable LDO8\n");
		s3c_usb_cable(0);
		}
		fsa9480_device1 = 0;
		fsa9480_device2 = 0;
	}
	
	enable_irq(IRQ_FSA9480_INTB);
}
EXPORT_SYMBOL(FSA9480_ReadIntRegister);



irqreturn_t fsa9480_interrupt(int irq, void *ptr)
{
	disable_irq(IRQ_FSA9480_INTB);

	queue_work(fsa9480_workqueue, &fsa9480_work);

	return IRQ_HANDLED; 
}
EXPORT_SYMBOL(fsa9480_interrupt);


void fsa9480_interrupt_init(void)
{		
	 s3c_gpio_cfgpin(GPIO_JACK_INT_N, S5PC11X_GPH2_7_EXT_INT32_7);
	 s3c_gpio_setpull(GPIO_JACK_INT_N, S3C_GPIO_PULL_NONE);
	 set_irq_type(IRQ_FSA9480_INTB, IRQ_TYPE_EDGE_FALLING);

	 if (request_irq(IRQ_FSA9480_INTB, fsa9480_interrupt, IRQF_DISABLED, "FSA9480 Detected", NULL)) 
		 DEBUG_FSA9480("[FSA9480]fail to register IRQ[%d] for FSA9480 USB Switch \n", IRQ_FSA9480_INTB);

}
EXPORT_SYMBOL(fsa9480_interrupt_init);


void fsa9480_init(void)
{

	 u8 data = 0;

	 fsa9480_write(&fsa9480_i2c_client, HIDDEN_REGISTER_MANUAL_OVERRDES1, 0x01); //RESET

	 mdelay(10);

	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICEID, &data);

	 mdelay(10);
	
	 fsa9480_write(&fsa9480_i2c_client, REGISTER_CONTROL, 0x1E);

	 mdelay(10);

	 fsa9480_write(&fsa9480_i2c_client, REGISTER_INTERRUPTMASK1, 0xFC);

	 mdelay(10);

	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICETYPE1, &fsa9480_device1);

	 mdelay(10);

	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICETYPE2, &fsa9480_device2);
	 
}
EXPORT_SYMBOL(fsa9480_init);

static int fsa9480_modify(struct i2c_client *client, u8 reg, u8 data, u8 mask)
{
   u8 original_value, modified_value;

   fsa9480_read(client, reg, &original_value);

   mdelay(10);
   
   modified_value = ((original_value&~mask) | data);
   
   fsa9480_write(client, reg, modified_value);

   mdelay(10);

   return 0;
}


void fsa9480_init_status(void)
{
	u8 pData;
	
	fsa9480_modify(&fsa9480_i2c_client,REGISTER_CONTROL,~INT_MASK, INT_MASK);
	
	fsa9480_read(&fsa9480_i2c_client, 0x13, &pData);
}

u8 FSA9480_Get_I2C_USB_Status(void)
{
	u8 device1, device2, usbstatus;

	
	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICETYPE1, &device1);
  	 msleep(5);
	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICETYPE2, &device2);

	if((device1==FSA9480_DEV_TY1_USB)||(device2==FSA9480_DEV_TY2_JIG_USB_ON)||(device2==FSA9480_DEV_TY2_JIG_USB_OFF))
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(FSA9480_Get_I2C_USB_Status);


void connectivity_switching_init(struct work_struct *ignored)
{
	int switch_sel;
	int usb_sel,uart_sel,samsung_kies_sel,ums_sel,mtp_sel,vtp_sel,askon_sel;
	int index;
	int lpm_mode_check = charging_mode_get();

	if (sec_get_param_value){
		sec_get_param_value(__SWITCH_SEL, &switch_sel);
		cancel_delayed_work(&micorusb_init_work);
		}
	else{
		schedule_delayed_work(&micorusb_init_work, msecs_to_jiffies(200));		
		return;
		}

	if(BOOTUP){
		BOOTUP = 0; 
		otg_phy_init(); //USB Power on after boot up.
		}

	usb_sel = switch_sel & (int)(USB_SEL_MASK);
	uart_sel = (switch_sel & (int)(UART_SEL_MASK)) >> 1;
	samsung_kies_sel = (switch_sel & (int)(USB_SAMSUNG_KIES_MASK)) >> 2;
	ums_sel = (switch_sel & (int)(USB_UMS_MASK)) >> 3;
	mtp_sel = (switch_sel & (int)(USB_MTP_MASK)) >> 4;
	vtp_sel = (switch_sel & (int)(USB_VTP_MASK)) >> 5;
	askon_sel = (switch_sel & (int)(USB_ASKON_MASK)) >> 6;

//	printk("[denis][connectivity_switching_init] Fatroty reset value : %x\n",factoryresetstatus); 
//	printk("[denis][connectivity_switching_init] Fatroty reset value : %d\n",switch_sel); 

	if((switch_sel == 0x1) || (factoryresetstatus == 0xAE)){
		PathSelStore(AP_USB_MODE);
		Ap_Cp_Switch_Config(AP_USB_MODE);	
		usb_switching_value_update(SWITCH_PDA);

		PathSelStore(CP_UART_MODE);
		Ap_Cp_Switch_Config(CP_UART_MODE);	
		uart_switching_value_update(SWITCH_MODEM);
		}
	else{
		if(usb_sel){
			Ap_Cp_Switch_Config(AP_USB_MODE);	
			usb_switching_value_update(SWITCH_PDA);
			}
		else{
			if(MicroJigUARTOffStatus || MicroJigUARTOnStatus){
				Ap_Cp_Switch_Config(AP_USB_MODE);
				}
			else{
				Ap_Cp_Switch_Config(CP_USB_MODE);	
				usb_switching_value_update(SWITCH_MODEM);
				}
			}
	
		if(uart_sel){
			Ap_Cp_Switch_Config(AP_UART_MODE);	
			uart_switching_value_update(SWITCH_PDA);
			}
		else{
			Ap_Cp_Switch_Config(CP_UART_MODE);	
			uart_switching_value_update(SWITCH_MODEM);
			}
		}

/*Turn off usb power when LPM mode*/
		if(lpm_mode_check)
			otg_phy_off();
			
		switching_value_update();	

	if((switch_sel == 1) || (factoryresetstatus == 0xAE)){
#if defined(CONFIG_S5PC110_T959_BOARD)		
		usb_switch_select(USBSTATUS_UMS);		
		ap_usb_power_on(1);
		UsbMenuSelStore(2);	
#else			
		usb_switch_select(USBSTATUS_SAMSUNG_KIES);
		mtp_mode_on = 1;
		ap_usb_power_on(0);
		UsbMenuSelStore(0);	
#endif	
		}
	else{
		if(usb_sel){
			if(samsung_kies_sel){
				usb_switch_select(USBSTATUS_SAMSUNG_KIES);
/*USB Power off till MTP Appl launching*/				
				mtp_mode_on = 1;
				ap_usb_power_on(0);
			}else if(mtp_sel){
				usb_switch_select(USBSTATUS_MTPONLY);
/*USB Power off till MTP Appl launching*/				
				mtp_mode_on = 1;
				ap_usb_power_on(0);
			}else if(ums_sel){
				usb_switch_select(USBSTATUS_UMS);
			}else if(vtp_sel){
				usb_switch_select(USBSTATUS_VTP);
			}else if(askon_sel){
				usb_switch_select(USBSTATUS_ASKON);
			}			
			}
		}

	if(!FSA9480_Get_USB_Status()){
		s3c_usb_cable(1);
		mdelay(5);
		s3c_usb_cable(0);
		}

	//printk("[FSA9480]connectivity_switching_init = switch_sel : 0x%x\n",switch_sel);
	microusb_uart_status(1);
	
	connectivity_switching_init_state=1;

	//del_timer(&microusb_timer);
	
}


static ssize_t print_switch_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", DRIVER_NAME);
}

static ssize_t print_switch_state(struct switch_dev *sdev, char *buf)
{
	int usbstatus;

	usbstatus = FSA9480_Get_USB_Status();

	if(usbstatus){
		if((currentusbstatus== USBSTATUS_UMS) || (currentusbstatus== USBSTATUS_ADB))
			return sprintf(buf, "%s\n", "InsertOnline");
		else
			return sprintf(buf, "%s\n", "InsertOffline");
		}
	else{
		if((currentusbstatus== USBSTATUS_UMS) || (currentusbstatus== USBSTATUS_ADB))
			return sprintf(buf, "%s\n", "RemoveOnline");
		else
			return sprintf(buf, "%s\n", "RemoveOffline");
		}		
}


static int fsa9480_codec_probe(struct i2c_adapter *adap, int addr, int kind)
{
	 int ret;
	 unsigned int microusb_data;
	 
#ifdef CONFIG_JUPITER_VER_B4
	 s3c_gpio_cfgpin( PMIC_I2C_SCL, S5PC11X_GPJ4_OUTPUT(3) );
	 s3c_gpio_setpull( PMIC_I2C_SCL, S3C_GPIO_PULL_UP); 
	 
	 s3c_gpio_cfgpin( PMIC_I2C_SDA, S5PC11X_GPJ4_OUTPUT(0));
	 s3c_gpio_setpull( PMIC_I2C_SDA, S3C_GPIO_PULL_UP); 

	 s3c_gpio_cfgpin( GPIO_USB_SEL, S5PC11X_MP04_OUTPUT(0));
	 s3c_gpio_setpull( GPIO_USB_SEL, S3C_GPIO_PULL_NONE); 
#else
	 s3c_gpio_cfgpin( GPIO_USB_SDA_28V, S5PC11X_GPJ3_OUTPUT(4));
	 s3c_gpio_setpull( GPIO_USB_SDA_28V, S3C_GPIO_PULL_UP);	 

	 s3c_gpio_cfgpin( GPIO_USB_SCL_28V, S5PC11X_GPJ3_OUTPUT(5) );
	 s3c_gpio_setpull( GPIO_USB_SCL_28V, S3C_GPIO_PULL_UP); 
#endif

	 s3c_gpio_cfgpin( GPIO_UART_SEL, S5PC11X_MP05_OUTPUT(7) );
	 s3c_gpio_setpull( GPIO_UART_SEL, S3C_GPIO_PULL_NONE);


	if (device_create_file(switch_dev, &dev_attr_uart_sel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_uart_sel.attr.name);
	 
	if (device_create_file(switch_dev, &dev_attr_usb_sel) < 0)
		DEBUG_FSA9480("[FSA9480]Failed to create device file(%s)!\n", dev_attr_usb_sel.attr.name);

	if (device_create_file(switch_dev, &dev_attr_usb_state) < 0)
		DEBUG_FSA9480("[FSA9480]Failed to create device file(%s)!\n", dev_attr_usb_state.attr.name);

	//denis
	if (device_create_file(switch_dev, &dev_attr_DMport) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_DMport.attr.name);

	if (device_create_file(switch_dev, &dev_attr_DMlog) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_DMlog.attr.name);
	
	 
	if (device_create_file(switch_dev, &dev_attr_UsbMenuSel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_UsbMenuSel.attr.name);
	 
	if (device_create_file(switch_dev, &dev_attr_AskOnMenuSel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_AskOnMenuSel.attr.name);

	if (device_create_file(switch_dev, &dev_attr_Mtp) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_Mtp.attr.name);

	if (device_create_file(switch_dev, &dev_attr_SwitchingInitValue) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_SwitchingInitValue.attr.name);		

	if (device_create_file(switch_dev, &dev_attr_FactoryResetValue) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_FactoryResetValue.attr.name);		
	
	if (device_create_file(switch_dev, &dev_attr_AskOnStatus) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_AskOnStatus.attr.name);			
	
	if (device_create_file(switch_dev, &dev_attr_MtpInitStatusSel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_MtpInitStatusSel.attr.name);			
	
	if (device_create_file(switch_dev, &dev_attr_AskInitStatusSel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_AskInitStatusSel.attr.name);			

	 
	 init_waitqueue_head(&usb_detect_waitq); 
	 INIT_WORK(&fsa9480_work, FSA9480_ReadIntRegister);
	 fsa9480_workqueue = create_singlethread_workqueue("fsa9480_workqueue");
	 

	 fsa9480_i2c_client.adapter = adap;
	 fsa9480_i2c_client.addr = addr;

	 ret = i2c_attach_client(&fsa9480_i2c_client);
	 if (ret < 0) {
		DEBUG_FSA9480("[FSA9480]failed to attach codec at addr %x\n", addr);
		return -1;
	 }

	 indicator_dev.name = DRIVER_NAME;
	 indicator_dev.print_name = print_switch_name;
	 indicator_dev.print_state = print_switch_state;
	 switch_dev_register(&indicator_dev);

	 fsa9480_init_status();

	 fsa9480_interrupt_init();

	 fsa9480_init();
	 
	 INIT_DELAYED_WORK(&micorusb_init_work, connectivity_switching_init);
	 schedule_delayed_work(&micorusb_init_work, msecs_to_jiffies(200));

	 
	 //init_timer(&microusb_timer);
	 //microusb_timer.expires = jiffies + msecs_to_jiffies(200);
	 //microusb_timer.data = microusb_data;
	 //microusb_timer.function = connectivity_switching_init;
	 //add_timer(&microusb_timer);
}
EXPORT_SYMBOL(fsa9480_codec_probe);

static int fsa9480_i2c_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

void fsa9480_wakeup(void)
{
	u8 interrupt1, device1, device2;
	u8 usbon,jigusbon,jigusboff;
	u8 usbstatus;

	 fsa9480_write(&fsa9480_i2c_client, REGISTER_CONTROL, 0x1E);

	 mdelay(5);

	 fsa9480_write(&fsa9480_i2c_client, REGISTER_INTERRUPTMASK1, 0xFC);

	 mdelay(5);
	 
	 fsa9480_read(&fsa9480_i2c_client, REGISTER_INTERRUPT1, &interrupt1);

	 mdelay(5);
	
	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICETYPE1, &device1);

	 mdelay(5);

	 fsa9480_read(&fsa9480_i2c_client, REGISTER_DEVICETYPE2, &device2);

	 usbon = 0;
	 jigusbon = 0;
	 jigusboff = 0;
	 usbstatus = 0;
	 if(interrupt1){
	 	if(device1 == FSA9480_DEV_TY1_USB)
			usbon = 1;
		else if(device2 == FSA9480_DEV_TY2_JIG_USB_ON)
			jigusbon = 1;
		else if(device2 == FSA9480_DEV_TY2_JIG_USB_OFF)
			jigusboff = 1;
	 	}

	 usbstatus = usbon | jigusbon | jigusboff |MicroJigUSBOnStatus | MicroJigUSBOffStatus;

	 if(!usbstatus){
		s3c_usb_cable(1);
		mdelay(5);
		s3c_usb_cable(0);
	 	}

}


static int fsa9480_i2c_resume(struct platform_device *dev)
{
	return 0;
}


static int fsa9480_i2c_attach(struct i2c_adapter *adap)
{
    //DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	return i2c_probe(adap, &fsa9480_addr_data, fsa9480_codec_probe);
}
EXPORT_SYMBOL(fsa9480_i2c_attach);


static int fsa9480_i2c_detach(struct i2c_client *client)
{
	DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	i2c_detach_client(client);
	return 0;
}

EXPORT_SYMBOL(fsa9480_i2c_detach);


/* corgi i2c codec control layer */
struct i2c_driver fsa9480_i2c_driver = {
	.driver = {
		.name = "fsa9480 I2C Codec",
		.owner = THIS_MODULE,
	},
	.id =             0,
	.attach_adapter = fsa9480_i2c_attach,
	.detach_client =  fsa9480_i2c_detach,
	.resume =  fsa9480_i2c_resume,
	.suspend =  fsa9480_i2c_suspend,
	.command =        NULL,
};


static struct i2c_client fsa9480_i2c_client = {
	.name =   "fsa9480",
	.driver = &fsa9480_i2c_driver,
};


