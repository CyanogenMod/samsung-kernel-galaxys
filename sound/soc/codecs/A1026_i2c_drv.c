
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/delay.h>

#include <mach/gpio-jupiter.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 

#include "A1026_dev.h"
#include "A1026_regs.h"
#include "audience.h"



/*extern functions*/
int A1026_i2c_drv_init();
void A1026_i2c_drv_exit(void);

/*static functions*/
static int A1026_probe (struct i2c_client *);
static int A1026_remove(struct i2c_client *);
static int A1026_suspend(struct i2c_client *, pm_message_t mesg);
static int A1026_resume(struct i2c_client *);
static int a1026_i2c_probe(struct i2c_adapter *, int, int);

static struct i2c_client A1026_i2c_client;
static struct i2c_driver A1026_i2c_driver;
////////////////////////////////////////////////////////////////////
//[hdlnc_bp_ldj : 20100308
enum A1025STATE {A1026SLEEP,A1026WAKEUP};
enum A1025STATE A1026_state=A1026SLEEP;
//]hdlnc_bp_ldj : 20100308
extern unsigned int HWREV;
int prev_num_closetalk_data = 248;
int prev_num_bypass_data = 32;
int prev_num_fartalk_data;
int prev_num_NS0_data = 248;
enum
{
    eTRUE,
    eFALSE,
}dev_struct_status_t;

/*dev_state*/



typedef struct 
{
    struct mutex lock;
	
    struct i2c_client const *client;

    dev_state_t state;

    u16 registers[NUM_OF_REGISTERS];

    unsigned short valid;

    /*will be true is the client ans state fields are correct*/
    unsigned short valid_client_state;
}A1026_device_t;

/*extern functions*/
/**********************************************/
/*All the exported functions which view or modify the device
  state/data, do i2c com will have to lock the mutex before 
  doing so*/
/**********************************************/
int A1026_dev_init(struct i2c_client *);
int A1026_dev_exit(void);

void A1026_dev_mutex_init(void); 

int A1026_dev_suspend(void);
int A1026_dev_resume(void);

int A1026_dev_powerup(void);
int A1026_dev_powerdown(void);

void A1026SetFeature(unsigned int feature);
//[hdlnc_bp_ldj : 20100305
void A1026Sleep();
void A1026Wakeup();
//]hdlnc_bp_ldj : 20100305
/***********************************************/

/*static functions*/
/**********************************************/
static int powerup(void);
static int powerdown(void);

static unsigned int i2c_read(struct i2c_client *client, u16 reg);
static int i2c_write(struct i2c_client *client, u8 reg, u16 num);
int A1026_Firmware_i2c_write(struct i2c_client *client);


/**********************************************/

/*A1026 device structure*/
static A1026_device_t A1026_dev =
{
    .client = NULL,
    .valid = eFALSE,
    .valid_client_state = eFALSE,
};

int A1026_dev_init(struct i2c_client *client)
{
    int ret = 0;

    debug("A1026_dev_init called");
     
    A1026_dev.client = client;

    /***reset the device here****/

    A1026_dev.valid_client_state = eTRUE; 	 

    debug("A1026_dev_init call over");
	 
    return ret;
}

int A1026_dev_exit(void)
{
    int ret = 0;
	 
    debug("A1026_dev_exit called");

    A1026_dev.client = NULL;	

    A1026_dev.valid_client_state = eFALSE;
    A1026_dev.valid = eFALSE;

    debug("A1026_dev_exit call over");

    return ret;
}

void A1026_dev_mutex_init(void)
{
    mutex_init(&(A1026_dev.lock));
}

int A1026_dev_powerup(void)
{

	int ret = 0;
    debug("A1026_dev_powerup called");

	if( (ret = powerup()) < 0 )
	{
		debug("powerup failed");
	}
	else
	{
		/*initial settings*/
	}
//[hdlnc_bp_ldj : 20100308
	A1026_state=A1026WAKEUP;
	A1026Sleep();
//]hdlnc_bp_ldj : 20100308
    return ret;
}


int A1026_dev_powerdown(void)
{
    int ret = 0;

    debug("A1026_dev_powerdown called"); 

    if( A1026_dev.valid == eFALSE )
    {
        debug("A1026_dev_powerdown called when DS is invalid");
	       ret = -1;	  
    }    
    else if ( ( ret = powerdown()) < 0 )
    {
        debug("powerdown failed");
    }

    return ret;
}

int A1026_dev_suspend(void)
{
    int ret = 0;
	 
    debug("A1026_dev_suspend called");

    if( A1026_dev.valid_client_state== eFALSE )
    {
        debug("A1026_dev_suspend called when DS (state, client) is invalid");
	       ret = -1;	  
    }    
#if 0
    else if( A1026_dev.state.power_state == RADIO_ON )
    {
        ret = powerdown();
    }
#endif

    return ret;
}

int A1026_dev_resume(void)
{
    int ret = 0; 

    if( A1026_dev.valid_client_state == eFALSE )
    {
        debug("A1026_dev_resume called when DS (state, client) is invalid");
	       ret = -1;	  
    }    
    return ret;
}

int configureIO()
{
	//configure GPIOs for A1026

	if (gpio_is_valid(GPIO_GPH10))
	{
		if (gpio_request(GPIO_GPH10, "GPIO_GPH10"))
			debug("Failed to request GPIO_GPH10! \n");
		gpio_direction_output(GPIO_GPH10, 0);
	}


	if (gpio_is_valid(GPIO_GPH11))
	{
		if (gpio_request(GPIO_GPH11, "GPIO_GPH11"))
			debug("Failed to request GPIO_GPH11! \n");
		gpio_direction_output(GPIO_GPH11, 0);
	}

	if((HWREV == 0x08) || (HWREV == 0x04))	
	{	

		debug("HWREV:%d\n",HWREV);
	}
	else //from 02 board
	{
		if (gpio_is_valid(GPIO_GPH12))
		{
			if (gpio_request(GPIO_GPH12, "GPIO_GPH12"))
				debug("Failed to request GPIO_GPH12! \n");
			gpio_direction_output(GPIO_GPH12, 0);
		}	

	}


	return 0;

}

int A1026Loadfirmware()
{
	int ret, count;
	u16 data;
	u8 buf[2], buf_1[4]={0};
	//ldj : HWreset and sleep for 50ms!!
	gpio_set_value(GPIO_GPH10, 1);
	msleep(50);
	//ldj: send bootinit command(0x0001) to A1026

	printk("A1026 slave addr = 0x%x \n", A1026_i2c_client.addr<<1);

	buf[0] = 0xff;
	buf[1] = 0xff;

	ret = i2c_master_send(&A1026_i2c_client, a1026_bootinit_command, 2);

	printk("i2c_master_send ret = [%d] \n", ret);
	//ldj : sleep for 1ms!!
	msleep(1);
	//ldj : read reg to check bootinit command

	ret = i2c_master_recv(&A1026_i2c_client, buf, 1);
	printk("bootinit command after read reg = 0x%x \n", buf[0]);

	do
	{
		ret = A1026_Firmware_i2c_write(&A1026_i2c_client);
			msleep(20);
			i2c_master_send(&A1026_i2c_client, a1026_sync_polling_command, 4);
			msleep(1);
			i2c_master_recv(&A1026_i2c_client, buf_1, 4);
			ret = strncmp(a1026_sync_polling_command,buf_1,4);

		if(ret!=0) printk("A1026 SYNC fail 0x%x 0x%x 0x%x 0x%x, count = %d\n",buf_1[0],buf_1[1],buf_1[2],buf_1[3], count);
		count++;
		if(count>3) break;
	}while(ret!=0);		
	
	//ldj : sleep for 10ms
	msleep(10);
	
	
	return 0;
}



/**************************************************************/
static int powerup(void)
{
    int ret=0;

	int reg;
	 /****Resetting the device****/

	 //ldj : configureIO() for set the GPIOs 

	ret = configureIO();
	if(!ret)
	{
		printk("configure error!! \n");
	}

	//ldj : wakeuppin & HWreset by control GPIOs and then sleep for 3s
	gpio_set_value(GPIO_GPH11, 1);
	gpio_set_value(GPIO_GPH10, 0);

	if((HWREV == 0x08) || (HWREV == 0x04))	
	{	

		debug("HWREV:%d\n",HWREV);
	}
	else //from 02 board
	{
		gpio_set_value(GPIO_GPH12, 1);
	}
		
	//ldj : Download firmware.
	ret = A1026Loadfirmware();

//	A1026SetFeature(BYPASSMODE);
// temporary i2c  line to input for test  
	gpio_direction_input(GPIO_A1026_SCL);
	s3c_gpio_slp_setpull_updown(GPIO_A1026_SCL, S3C_GPIO_PULL_NONE);
	gpio_direction_input(GPIO_A1026_SDA);
	s3c_gpio_slp_setpull_updown(GPIO_A1026_SDA, S3C_GPIO_PULL_NONE);


	
    return ret;
}

static int powerdown(void)
{
    int ret = 0;


    return ret;
}


static unsigned int i2c_read(struct i2c_client *client, u16 reg)
{
	struct i2c_msg xfer[2];
	u16 data;
	int ret;

	data = ((reg & 0xff00) >> 8) | ((reg & 0xff) << 8);
	
	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;  //1
	//xfer[0].buf = &reg;
	xfer[0].buf = (void *)&data;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = 2;
	xfer[1].buf = (u8 *)&data;
	ret = i2c_transfer(client->adapter, xfer, 2);
	if (ret != 2) {
		printk("Failed to read 0x%x: %d\n", reg, ret);
		return 0;
	}

	return (data >> 8) | ((data & 0xff) << 8);

}
static int i2c_write(struct i2c_client *client, u8 reg, u16 num)
{
	int ret, i;
	u8 buf[32];

	for(i=0; i<num; i++)
	{
	//	buf[i] = reg[i];
	}
	ret = i2c_master_send(client, buf, num);
	
	if (ret != num)
	{
		printk("failed to write buf to A1026 \n");
		return -EIO;
	}
	return 0;
}

void A1026SetFeature(unsigned int feature)
{

	int ret, i;
	u8 buf[4];
											

//[hdlnc_bp_ldj : 20100305
	A1026Wakeup();
//]hdlnc_bp_ldj : 20100305	

	switch(feature)
	{	
		case CLOSETALK:
		{
				printk("A1026SetFeature CLOSETALK mode \n");

				int num_data = audience_closetalk(buf_ct_tuning);			
		
				if(num_data)
				{
						printk("[a1026 tuning data for write\n");
//						for(i=0;i<num_data;i++) 
//						printk("0x%x \n",buf_ct_tuning[i]);
						ret = i2c_master_send(&A1026_i2c_client, buf_ct_tuning, num_data);
						prev_num_closetalk_data = num_data;
				}
				else
				{
						printk("[a1026 write prev command\n");
//						for(i=0;i<prev_num_closetalk_data;i++) 
//						printk("0x%x \n",buf_ct_tuning[i]);
						ret = i2c_master_send(&A1026_i2c_client, buf_ct_tuning, prev_num_closetalk_data);
							
				}						
		}
		break;
		case FARTALK:
			printk("A1026SetFeature FARTALK mode \n");
			break;

		case BYPASSMODE:
		{
			printk("A1026SetFeature BYPASSMODE mode \n");

			int num_data = audience_bypass(buf_bypass_tuning);
			
			if(num_data)
			{
					printk("[a1026 tuning data for write\n");
//					for(i=0;i<num_data;i++) 
//					printk("0x%x \n",buf_bypass_tuning[i]);
					ret = i2c_master_send(&A1026_i2c_client, buf_bypass_tuning, num_data);
					prev_num_bypass_data = num_data;
			}
			else
			{
					printk("[a1026 write prev command\n");
//					for(i=0;i<prev_num_bypass_data;i++) 
//					printk("0x%x \n",buf_bypass_tuning[i]);
					ret = i2c_master_send(&A1026_i2c_client, buf_bypass_tuning, prev_num_bypass_data);							
			}				

		}
		break;
		case NS0:
			{
			printk("A1026SetFeature NS0 mode \n");

			int num_data = audience_NS0(buf_NS0_tuning);
			
			if(num_data)
			{
					printk("[a1026 tuning data for write\n");
//					for(i=0;i<num_data;i++) 
//					printk("0x%x \n",buf_NS0_tuning[i]);
					ret = i2c_master_send(&A1026_i2c_client, buf_NS0_tuning, num_data);
					prev_num_NS0_data = num_data;
			}
			else
			{
					printk("[a1026 write prev command\n");
//					for(i=0;i<prev_num_NS0_data;i++) 
//					printk("0x%x \n",buf_NS0_tuning[i]);
					ret = i2c_master_send(&A1026_i2c_client, buf_NS0_tuning, prev_num_NS0_data);							
			}		

		}		
		break;
		default:
			printk("A1026SetFeature Invalid mode \n");

	}
	

}
//[hdlnc_bp_ldj : 20100305
void A1026Sleep()
{
	char sleep_cmd[4]={0x80,0x10,0x00,0x01};
	char buf[4]={0};
//[hdlnc_bp_ldj : 20100308	
	int ret=0,count=0;

	if(A1026_state==A1026SLEEP) return;
	A1026_state=A1026SLEEP;
//]hdlnc_bp_ldj : 20100308	
	printk("A1026Sleep()\n");
//[hdlnc_bp_ldj : 20100308
	do 
	{
		i2c_master_send(&A1026_i2c_client, sleep_cmd, 4);
		msleep(20);
		ret = i2c_master_recv(&A1026_i2c_client, buf, 4);
		printk("A1025Sleep 0x%x 0x%x 0x%x 0x%x\n",buf[0],buf[1],buf[2],buf[3]);
		ret=strncmp(sleep_cmd,buf,4);
		if(ret!=0) printk("A1026 sleep faile\n");
		count++;
		if(count>10) break;
	}  while(ret!=0);

	printk("A1026Sleep() ret=%d\n",ret);
//]hdlnc_bp_ldj : 20100308
	
	msleep(140);
}

void A1026Wakeup()
{
	char wakeup_cmd[4]={0x80,0x00,0x00,0x00};
	char buf[4]={0};
//[hdlnc_bp_ldj : 20100308	
	int ret=0,count=0;

	if(A1026_state==A1026WAKEUP) return;
	A1026_state=A1026WAKEUP;
//]hdlnc_bp_ldj : 20100308	
	printk("A1026Wakeup()\n");
//[hdlnc_bp_ldj : 20100308
	msleep(10);
	do
	{
		gpio_set_value(GPIO_GPH11, 1);
		msleep(90);
		gpio_set_value(GPIO_GPH11, 0);
		msleep(20);

	i2c_master_send(&A1026_i2c_client, wakeup_cmd, 4);
	msleep(20);
	i2c_master_recv(&A1026_i2c_client, buf, 4);

		printk("A1025Wakeup 0x%x 0x%x 0x%x 0x%x\n",buf[0],buf[1],buf[2],buf[3]);
		ret=strncmp(wakeup_cmd,buf,4);
		if(ret!=0) printk("A1026 wake fail]\n");
		count++;
		if(count>10) break;
	} while(ret!=0);

	printk("A1026Wakeup() ret=%d\n",ret);
//]hdlnc_bp_ldj : 20100308
	gpio_set_value(GPIO_GPH11, 1);
}
//]hdlnc_bp_ldj : 20100305
int A1026_Firmware_i2c_write(struct i2c_client *client)
{
	int ret, total_num;
	unsigned int i, index;

	u8 buf[4] = {0x80, 0x00, 0x00, 0x00};

	if((HWREV == 0x08) || (HWREV == 0x04))
		total_num = TOTAL_NUM_OF_FW;	
	else
		total_num = TOTAL_NUM_OF_FW_2;
			
		for(i=0; i<(total_num/NUM_OF_BYTE); i++)
		{
			index = i*NUM_OF_BYTE;
			if((HWREV == 0x08) || (HWREV == 0x04))
				ret = i2c_master_send(&A1026_i2c_client, &a1026_firmware_buf[index], NUM_OF_BYTE);
			else
				ret = i2c_master_send(&A1026_i2c_client, &a1026_firmware_buf2[index], NUM_OF_BYTE);
			if(ret != NUM_OF_BYTE)
			{	
				printk("A1026 firmware download error!\n");
				return -1;
			}
		}	
	if((HWREV == 0x08) || (HWREV == 0x04))	
		ret = i2c_master_send(&A1026_i2c_client, &a1026_firmware_buf[index + NUM_OF_BYTE], REMAINED_NUM);
	else
//		ret = i2c_master_send(&A1026_i2c_client, &a1026_firmware_buf2[index + NUM_OF_BYTE], REMAINED_NUM);
			ret = i2c_master_send(&A1026_i2c_client, &a1026_firmware_buf2[index + NUM_OF_BYTE], 20);  //temp for B3024 firmaware binary 
	if(ret != REMAINED_NUM)
	{
		printk("A1026 firmware download error!\n");	
		return -1;
	}
	return 0;
}





////////////////////////////////////////////////////////////////////

/*I2C Setting*/
#define A1026_I2C_ADDRESS      0x3E

static unsigned short A1026_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short A1026_ignore[] = { I2C_CLIENT_END };
//static unsigned short A1026_i2c_probe[] = { 13, A1026_I2C_ADDRESS >> 1, I2C_CLIENT_END };
static unsigned short A1026_i2c_probe[] = { 13, A1026_I2C_ADDRESS, I2C_CLIENT_END };

static struct i2c_client_address_data A1026_addr_data = {
	.normal_i2c = A1026_normal_i2c,
	.ignore     = A1026_ignore,
	.probe      = A1026_i2c_probe,
};

static int A1026_i2c_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &A1026_addr_data, a1026_i2c_probe);
}

static int A1026_i2c_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	A1026_remove(client);

	return 0;
}

static struct i2c_driver A1026_i2c_driver =
{
    .driver = {
        .name = "A1026_driver",
    },
    	.id = 13,	
	.attach_adapter = &A1026_i2c_attach,
	.detach_client =  &A1026_i2c_detach,

    .suspend = &A1026_suspend,
    .resume = &A1026_resume,
};

static struct i2c_client A1026_i2c_client = {
    .name =   "A1026",
	.driver = &A1026_i2c_driver,
};

static int A1026_probe (struct i2c_client *client)
{
    int ret = 0;

    debug("A1026 i2c driver A1026_probe called"); 

    if( strcmp(client->name, "A1026") != 0 )
    {
        ret = -1;
        debug("A1026_probe: device not supported");
    }
    else if( (ret = A1026_dev_init(client)) < 0 )
    {
        debug("A1026_dev_init failed");
    }

    return ret;
}

static int a1026_i2c_probe(struct i2c_adapter *adapter, int address, int kind)
{
	int err = 0;         

	printk("----- %s %d\n", __func__, __LINE__);

	A1026_i2c_client.addr = address; 
	A1026_i2c_client.adapter = adapter;
	A1026_i2c_client.driver = &A1026_i2c_driver;

	if ((err = i2c_attach_client(&A1026_i2c_client)))
		return err;

	A1026_probe(&A1026_i2c_client);

	return 0;
}


static int A1026_remove(struct i2c_client *client)
{
    int ret = 0;

    debug("A1026 i2c driver A1026_remove called"); 

    if( strcmp(client->name, "A1026") != 0 )
    {
        ret = -1;
        debug("A1026_remove: device not supported");
    }
    else if( (ret = A1026_dev_exit()) < 0 )
    {
        debug("A1026_dev_exit failed");
    }

    return ret;
}

static int A1026_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret = 0;
	   
//    debug("A1026 i2c driver A1026_suspend called"); 

    if( strcmp(client->name, "A1026") != 0 )
    {
        ret = -1;
        debug("A1026_suspend: device not supported");
    }
    else if( (ret = A1026_dev_suspend()) < 0 )
    {
        debug("A1026_dev_disable failed");
    }

    return 0;
}

static int A1026_resume(struct i2c_client *client)
{
    int ret = 0;
	   
//    debug("A1026 i2c driver A1026_resume called"); 

    if( strcmp(client->name, "A1026") != 0 )
    {
        ret = -1;
        debug("A1026_resume: device not supported");
    }
    else if( (ret = A1026_dev_resume()) < 0 )
    {
        debug("A1026_dev_enable failed");
    }
 
    return 0;
}

int A1026_i2c_drv_init()
{
	struct i2c_board_info info;
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int ret;

	ret = i2c_add_driver(&A1026_i2c_driver);
	if (ret != 0) {
		return ret;
	}

	return 0;

}

void A1026_i2c_drv_exit(void)
{
    debug("A1026 i2c driver A1026_i2c_driver_exit called"); 

    i2c_del_driver(&A1026_i2c_driver);
}


