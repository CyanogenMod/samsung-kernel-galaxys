/*
 *  Copyright (C) 2004 Samsung Electronics
 *             SW.LEE <hitchcar@samsung.com>
 *            - based on Russell King : pcf8583.c
 * 	      - added  smdk24a0, smdk2440
 *            - added  poseidon (s3c24a0+wavecom)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Driver for FIMC2.x Camera Decoder
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <mach/hardware.h>

#include <plat/gpio-cfg.h>
#include <plat/egpio.h>

#include "../s3c_camif.h"

#include "s5k4ca.h"

// function define
//#define MEASURE_AF_OPERATION
//#define CONFIG_LOAD_FILE

/// debugging print
//#define __TRACE_CAM_SENSOR__
//#define __TRACE_FULL_CAM_SENSOR__

#if defined(__TRACE_FULL_CAM_SENSOR__)
	#define __TRACE_CAM_SENSOR(s) (s)
	#define __TRACE_FULL_CAM_SENSOR(s) (s) 
#elif defined(__TRACE_CAM_SENSOR__)
	#define __TRACE_CAM_SENSOR(s) (s)
	#define __TRACE_FULL_CAM_SENSOR(s)
#else
	#define __TRACE_CAM_SENSOR(s)
	#define __TRACE_FULL_CAM_SENSOR(s)
#endif

// Purpose of verifying I2C operaion. must be ignored later.
//#define LOCAL_CONFIG_S5K4CA_I2C_TEST

static struct i2c_driver s5k4ca_driver;

static void s5k4ca_sensor_gpio_init(void);
void s5k4ca_sensor_enable(void);
static void s5k4ca_sensor_disable(void);

static int s5k4ca_sensor_init(void);
static void s5k4ca_sensor_exit(void);

static int s5k4ca_sensor_change_size(struct i2c_client *client, int size);

#ifdef CONFIG_FLASH_AAT1271A
	extern int aat1271a_flash_init(void);
	extern void aat1271a_flash_exit(void);
	extern void aat1271a_falsh_camera_control(int ctrl);
	extern void aat1271a_falsh_movie_control(int ctrl);
#endif

#ifdef CONFIG_LOAD_FILE
	static int s5k4ca_regs_table_write(char *name);
#endif

/* 
 * MCLK: 24MHz, PCLK: 54MHz
 * 
 * In case of PCLK 54MHz
 *
 * Preview Mode (1024 * 768)  
 * 
 * Capture Mode (2048 * 1536)
 * 
 * Camcorder Mode
 */
static camif_cis_t s5k4ca_data = {
	itu_fmt:       	CAMIF_ITU601,
	order422:      	CAMIF_CRYCBY,
	camclk:        	24000000,		
	source_x:      	1024,		
	source_y:      	768,
	win_hor_ofst:  	0,
	win_ver_ofst:  	0,
	win_hor_ofst2: 	0,
	win_ver_ofst2: 	0,
	polarity_pclk: 	0,
	polarity_vsync:	1,
	polarity_href: 	0,
	reset_type:		CAMIF_RESET,
	reset_udelay: 	5000,
};

/* #define S5K4CA_ID	0x78 */

static unsigned short s5k4ca_normal_i2c[] = { (S5K4CA_ID >> 1), I2C_CLIENT_END };
static unsigned short s5k4ca_ignore[] = { I2C_CLIENT_END };
static unsigned short s5k4ca_probe[] = { I2C_CLIENT_END };

static int previous_scene_mode = 10;
static unsigned short lux_value = 0;
static int first_init_end = 0;

static struct i2c_client_address_data s5k4ca_addr_data = {
	.normal_i2c = s5k4ca_normal_i2c,
	.ignore		= s5k4ca_ignore,
	.probe		= s5k4ca_probe,
};

static int s5k4ca_sensor_read(struct i2c_client *client,
		unsigned short subaddr, unsigned short *data)
{
	int ret;
	unsigned char buf[2];
	struct i2c_msg msg = { client->addr, 0, 2, buf };
	
	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
		goto error;

	msg.flags = I2C_M_RD;
	
	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
		goto error;

	*data = ((buf[0] << 8) | buf[1]);

error:
	return ret;
}

static int s5k4ca_sensor_write(struct i2c_client *client, 
		unsigned short subaddr, unsigned short val)
{
	unsigned char buf[4];
	struct i2c_msg msg = { client->addr, 0, 4, buf };

	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);
	buf[2] = (val >> 8);
	buf[3] = (val & 0xFF);

	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

static int s5k4ca_sensor_write_list(struct i2c_client *client, struct samsung_short_t *list,char *name)
{
	int i, ret;
	ret = 0;
#ifdef CONFIG_LOAD_FILE 
	s5k4ca_regs_table_write(name);	
#else
	for (i = 0; list[i].subaddr != 0xffff; i++)
		ret = s5k4ca_sensor_write(client, list[i].subaddr, list[i].value);
#endif
	return ret;
}

static void s5k4ca_sensor_get_id(struct i2c_client *client)
{
	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] +%s\n",__func__));
	unsigned short id = 0;
	
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x01FA);
	s5k4ca_sensor_read(client, 0x0F12, &id);

	(printk("[CAM-SENSOR] =Sensor ID(0x%04x) is %s!\n", id, (id == 0x4CA4) ? "Valid" : "Invalid")); 
	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] -%s\n",__func__));
}

static void s5k4ca_sensor_gpio_init(void)
{
	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] +%s\n",__func__));
	I2C_CAM_DIS;
	MCAM_RST_DIS;
	VCAM_RST_DIS;
	CAM_PWR_DIS;
	AF_PWR_DIS;
	MCAM_STB_DIS;
	VCAM_STB_DIS;
	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] -%s\n",__func__));
}

#if defined(CONFIG_LDO_LP8720)
extern void	s5k4ca_sensor_power_init(void);	
#endif

void s5k4ca_sensor_enable(void)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	s5k4ca_sensor_gpio_init();

	MCAM_STB_EN;

	/* > 0 ms */
	msleep(1);

	AF_PWR_EN;	

#if defined(CONFIG_LDO_LP8720)
	s5k4ca_sensor_power_init();	
#endif

	CAM_PWR_EN;

	/* > 0 ms */
	msleep(1);

	/* MCLK Set */
	clk_set_rate(cam_clock, s5k4ca_data.camclk);

	/* MCLK Enable */
	clk_enable(cam_clock);
	clk_enable(cam_hclk);
	
	msleep(1);

	MCAM_RST_EN;
	
	msleep(40);
	
	I2C_CAM_EN;
	(printk("[CAM-SENSOR] -%s\n",__func__));
}

static void s5k4ca_sensor_disable(void)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	I2C_CAM_DIS;
	
	MCAM_STB_DIS;

	/* > 20 cycles */
	msleep(1);

	/* MCLK Disable */
	clk_disable(cam_clock);
	clk_disable(cam_hclk);

	/* > 0 ms */
	msleep(1);

	MCAM_RST_DIS;

	/* > 0 ms */
	msleep(1);

	AF_PWR_DIS;

	CAM_PWR_DIS;
	(printk("[CAM-SENSOR] -%s\n",__func__));
}

static void sensor_init(struct i2c_client *client)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	int ret = 0;

	ret = s5k4ca_sensor_write_list(client,s5k4ca_init0,"s5k4ca_init0");

	msleep(100);	

	/* Check Sensor ID */
	s5k4ca_sensor_get_id(client);

	ret = s5k4ca_sensor_write_list(client,s5k4ca_init1,"s5k4ca_init1");

	first_init_end = 1;

	//s5k4ca_sensor_change_size(client, SENSOR_XGA);
	(printk("[CAM-SENSOR] -%s ret=%d\n",__func__,ret));
}

static int s5k4cagx_attach(struct i2c_adapter *adap, int addr, int kind)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	struct i2c_client *c;
	
	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, "s5k4ca");
	c->addr = addr;
	c->adapter = adap;
	c->driver = &s5k4ca_driver;
	s5k4ca_data.sensor = c;

	(printk("[CAM-SENSOR] -%s\n",__func__));
#ifdef LOCAL_CONFIG_S5K4CA_I2C_TEST
	i2c_attach_client(c);
	msleep(10);
	sensor_init(c);

	return 0;
#else
	return i2c_attach_client(c);
#endif
}

static int s5k4ca_sensor_attach_adapter(struct i2c_adapter *adap)
{
	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] =%s\n",__func__));
	return i2c_probe(adap, &s5k4ca_addr_data, s5k4cagx_attach);
}

static int s5k4ca_sensor_detach(struct i2c_client *client)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	i2c_detach_client(client);
	(printk("[CAM-SENSOR] -%s\n",__func__));
	return 0;
}

static int s5k4ca_sensor_mode_set(struct i2c_client *client, int type)
{
	int i, size;
//	unsigned short light = 0;
	int delay = 300;

	printk("[CAM-SENSOR] =Sensor Mode ");

	if (type & SENSOR_PREVIEW)
	{	
		printk("-> Preview ");
		
//		s5k4ca_sensor_write_list(client,s5k4ca_preview_0,"s5k4ca_preview_0"); // 1024 x 768 

		s5k4ca_sensor_write_list(client,s5k4ca_preview,"s5k4ca_preview"); // preview start
				
		if (type & SENSOR_NIGHTMODE)
		{	
			printk("Night\n");
			s5k4ca_sensor_write_list(client,s5k4ca_nightmode_on,"s5k4ca_nightmode_on");
		}
		else
		{	
			printk("Normal\n");
			s5k4ca_sensor_write_list(client,s5k4ca_nightmode_off,"s5k4ca_nightmode_off");
		}

		if(first_init_end)
			first_init_end = 0;
		else
			delay = 0;
	}
	else if (type & SENSOR_CAPTURE)
	{	
		printk("-> Capture ");

//		s5k4ca_sensor_write_list(client,s5k4ca_capture_0,"s5k4ca_capture_0"); // 2048x1536(3M)
		
/*		s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	
		s5k4ca_sensor_write(client, 0x002C, 0x7000);
		s5k4ca_sensor_write(client, 0x002E, 0x12FE);
		msleep(100);
		s5k4ca_sensor_read(client, 0x0F12, &light);*/

		if (lux_value <= 0x40) /* Low light */
		{	
			if (type & SENSOR_NIGHTMODE)
			{	
				printk("Night Low Light light=0x%04x\n",lux_value);
				delay = 1600;
				s5k4ca_sensor_write_list(client,s5k4ca_snapshot_nightmode,"s5k4ca_snapshot_nightmode");
			}
			else
			{	
				printk("Normal Low Light light=0x%04x\n",lux_value);
				delay = 800;
				s5k4ca_sensor_write_list(client,s5k4ca_snapshot_low,"s5k4ca_snapshot_low");
			}
		}
		else
		{
			printk("Normal Normal Light light=0x%04x\n",lux_value);
			delay = 200;
			s5k4ca_sensor_write_list(client,s5k4ca_snapshot_normal,"s5k4ca_snapshot_normal");
		}
	}
	else if (type & SENSOR_CAMCORDER )
	{
		printk("[CAM-SENSOR] =Record\n");
		delay = 300;
		s5k4ca_sensor_write_list(client,s5k4ca_fps_15fix,"s5k4ca_fps_15fix");
	}

	msleep(delay);
	
	printk("[CAM-SENSOR] =delay time(%d msec)\n", delay);
	
	return 0;
}

static int s5k4ca_sensor_change_size(struct i2c_client *client, int size)
{
	switch (size) {
		case SENSOR_XGA:
			s5k4ca_sensor_mode_set(client, SENSOR_PREVIEW);
			break;

		case SENSOR_QXGA:
			s5k4ca_sensor_mode_set(client, SENSOR_CAPTURE);
			break;		
	
		default:
			printk("[CAM-SENSOR] =Unknown Size! (Only XGA & QXGA)\n");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_af_control(struct i2c_client *client, int type)
{

    int count = 50;
    int tmpVal = 0;
    int ret = 0;
	int size = 0;
	int i = 0;
	unsigned short light = 0;

    switch (type)
    {
		
        case 0:
            printk("[CAM-SENSOR] =Focus Mode -> Manual\n");
            break;

        case 1:
            printk("[CAM-SENSOR] =Focus Mode -> Single\n");
#ifdef MEASURE_AF_OPERATION
			struct timeval a,b;
			long diff;
			do_gettimeofday(&a);
#endif
			s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	
			s5k4ca_sensor_write(client, 0x002C, 0x7000);
			s5k4ca_sensor_write(client, 0x002E, 0x12FE);
//			msleep(100);
			s5k4ca_sensor_read(client, 0x0F12, &light);
			lux_value = light;
			if (light < 0x80){ /* Low light AF*/
				s5k4ca_sensor_write_list(client,s5k4ca_af_low_lux_val,"s5k4ca_af_low_lux_val");
				printk("[CAM-SENSOR] =Low Light AF Single light=0x%04x\n",light);
			}else{
				s5k4ca_sensor_write_list(client,s5k4ca_af_normal_lux_val,"s5k4ca_af_normal_lux_val");
				printk("[CAM-SENSOR] =Normal Light AF Single light=0x%04x\n",light);
			}

            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, 0x00FF);

            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            msleep(140);

            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, 0x00F1);
            msleep(50);  

            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0003); // AF Freeze
            msleep(50);

            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0002); // AF Single 

            do
            {
                if( count == 0)
                    break;

	            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
                s5k4ca_sensor_write(client, 0x002C, 0x7000);    
                s5k4ca_sensor_write(client, 0x002E, 0x130E);
				if (light < 0x80)
	                msleep(250);
				else
					msleep(100);
                s5k4ca_sensor_read(client, 0x0F12, &tmpVal); 

                count--;

                printk("[CAM-SENSOR] =CAM 3M AF Status Value = %x \n", tmpVal); 

            }
            while( (tmpVal & 0x3) != 0x3 && (tmpVal & 0x3) != 0x2 );

            if(count == 0  )
            {
                printk("[CAM-SENSOR] =CAM 3M AF_Single Mode Fail.==> TIMEOUT \n");
                ret = 0;
            }

            if((tmpVal & 0x3) == 0x02)
            {
				s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	
				s5k4ca_sensor_write(client, 0x0028, 0x7000);

				s5k4ca_sensor_write(client, 0x002A, 0x030E);	
                s5k4ca_sensor_write(client, 0x0F12, 0x00FF);

				s5k4ca_sensor_write(client, 0x002A, 0x030C);	
				s5k4ca_sensor_write(client, 0x0F12, 0x0000);
				msleep(140);

				s5k4ca_sensor_write(client, 0x002A, 0x030E);	
				s5k4ca_sensor_write(client, 0x0F12, 0x00F1);
                msleep(50);     

				s5k4ca_sensor_write(client, 0x002A, 0x030C);	
                s5k4ca_sensor_write(client, 0x0F12, 0x0003);

				s5k4ca_sensor_write(client, 0x0028, 0x7000);
				s5k4ca_sensor_write(client, 0x002A, 0x161C);	
				s5k4ca_sensor_write(client, 0x0F12, 0x82A8);

                printk("[CAM-SENSOR] =CAM 3M AF_Single Mode Fail.==> FAIL \n");

                ret = 0;
            }

            if(tmpVal & 0x3 == 0x3)
            {
                printk("[CAM-SENSOR] =CAM 3M AF_Single Mode SUCCESS. \r\n");
                ret = 1;
            }

#ifdef MEASURE_AF_OPERATION
			do_gettimeofday(&b);
			diff = ((b.tv_sec*1000000+b.tv_usec) - (a.tv_sec*1000000+a.tv_usec))/1000;
			printk("[CAM-SENSOR] =CAM:3M AF_SINGLE SET : %d msec\n", diff);
#else
	        printk("[CAM-SENSOR] =CAM:3M AF_SINGLE SET \r\n");
#endif
            msleep(200);
            break;

		case 2:
			printk("[CAM-SENSOR] =Focus Mode -> Macro\n");			
			s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	
			s5k4ca_sensor_write(client, 0x0028, 0x7000);

			s5k4ca_sensor_write(client, 0x002A, 0x030E);	
			s5k4ca_sensor_write(client, 0x0F12, 0x0030);

			s5k4ca_sensor_write(client, 0x002A, 0x030C);	
			s5k4ca_sensor_write(client, 0x0F12, 0x0000);
			msleep(140);

			s5k4ca_sensor_write(client, 0x002A, 0x030E);	
			s5k4ca_sensor_write(client, 0x0F12, 0x0040);
			msleep(50);

			s5k4ca_sensor_write(client, 0x0028, 0x7000);	
			s5k4ca_sensor_write(client, 0x002A, 0x161C);
			s5k4ca_sensor_write(client, 0x0F12, 0xA2A8);	
  			break;

        default:
            break;

    }

    return ret;
}

static int s5k4ca_sensor_change_effect(struct i2c_client *client, int type)
{
	int size;	
	
	printk("[CAM-SENSOR] =Effects Mode %d",type);

	switch (type)
	{
		case 0:
		default:
			printk("-> Mode None\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_off,"s5k4ca_effect_off");
			break;
		case 1:
			printk("-> Mode Gray\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_gray,"s5k4ca_effect_gray");
			break;
		case 2:
			printk("-> Mode Sepia\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_sepia,"s5k4ca_effect_sepia");
			break;
		case 3:
			printk("-> Mode Negative\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_negative,"s5k4ca_effect_negative");
			break;
		case 4:
			printk("-> Mode Aqua\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_aqua,"s5k4ca_effect_aqua");
			break;
		case 5:
			printk("-> Mode Sketch\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_sketch,"s5k4ca_effect_sketch");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_change_br(struct i2c_client *client, int type)
{
	int size;

	printk("[CAM-SENSOR] =Brightness Mode %d",type);

	switch (type)
	{
		case 0:	
		default :
			printk("-> Brightness Minus 4\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus4,"s5k4ca_br_minus4");
			break;
		case 1:
			printk("-> Brightness Minus 3\n");	
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus3,"s5k4ca_br_minus3");
			break;
		case 2:
			printk("-> Brightness Minus 2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus2,"s5k4ca_br_minus2");
			break;
		case 3:
			printk("-> Brightness Minus 1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus1,"s5k4ca_br_minus1");
			break;
		case 4:
			printk("-> Brightness Zero\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_zero,"s5k4ca_br_zero");
			break;
		case 5:
			printk("-> Brightness Plus 1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_plus1,"s5k4ca_br_plus1");
			break;
		case 6:
			printk("-> Brightness Plus 2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_plus2,"s5k4ca_br_plus2");
			break;
		case 7:
			printk("-> Brightness Plus 3\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_plus3,"s5k4ca_br_plus3");
			break;
		case 8:
			printk("-> Brightness Plus 4\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_plus4,"s5k4ca_br_plus4");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_change_wb(struct i2c_client *client, int type)
{
	int size;
	
	printk("[CAM-SENSOR] =White Balance Mode %d",type);

	switch (type)
	{
		case 0:
		default :
			printk("-> WB auto mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_auto,"s5k4ca_wb_auto");
			break;
		case 1:
			printk("-> WB Sunny mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_sunny,"s5k4ca_wb_sunny");
			break;
		case 2:
			printk("-> WB Cloudy mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_cloudy,"s5k4ca_wb_cloudy");
			break;
		case 3:
			printk("-> WB Flourescent mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_fluorescent,"s5k4ca_wb_fluorescent");
			break;
		case 4:
			printk("-> WB Tungsten mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_tungsten,"s5k4ca_wb_tungsten");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_change_contrast(struct i2c_client *client, int type)
{
	int size;
	
	printk("[CAM-SENSOR] =Contras Mode %d",type);

	switch (type)
	{
		case 0:
			printk("-> Contrast -2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_m2,"s5k4ca_contrast_m2");
			break;
		case 1:
			printk("-> Contrast -1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_m1,"s5k4ca_contrast_m1");
			break;
		default :
		case 2:
			printk("-> Contrast 0\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_0,"s5k4ca_contrast_0");
			break;
		case 3:
			printk("-> Contrast +1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_p1,"s5k4ca_contrast_p1");
			break;
		case 4:
			printk("-> Contrast +2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_P2,"s5k4ca_contrast_P2");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_change_saturation(struct i2c_client *client, int type)
{
	int size;
	
	printk("[CAM-SENSOR] =Saturation Mode %d",type);

	switch (type)
	{
		case 0:
			printk("-> Saturation -2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_m2,"s5k4ca_Saturation_m2");
			break;
		case 1:
			printk("-> Saturation -1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_m1,"s5k4ca_Saturation_m1");
			break;
		case 2:
		default :
			printk("-> Saturation 0\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_0,"s5k4ca_Saturation_0");
			break;
		case 3:
			printk("-> Saturation +1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_p1,"s5k4ca_Saturation_p1");
			break;
		case 4:
			printk("-> Saturation +2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_P2,"s5k4ca_Saturation_P2");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_change_sharpness(struct i2c_client *client, int type)
{
	int size;
	
	printk("[CAM-SENSOR] =Sharpness Mode %d",type);

	switch (type)
	{
		case 0:
			printk("-> Sharpness -2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_m2,"s5k4ca_Sharpness_m2");
			break;
		case 1:
			printk("-> Sharpness -1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_m1,"s5k4ca_Sharpness_m1");
			break;
		case 2:
		default :
			printk("-> Sharpness 0\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_0,"s5k4ca_Sharpness_0");
			break;
		case 3:
			printk("-> Sharpness +1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_p1,"s5k4ca_Sharpness_p1");
			break;
		case 4:
			printk("-> Sharpness +2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_P2,"s5k4ca_Sharpness_P2");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_change_iso(struct i2c_client *client, int type)
{
	int size;
	
	printk("[CAM-SENSOR] =Iso Mode %d",type);

	switch (type)
	{
		case 0:
		default :
			printk("-> ISO AUTO\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso_auto,"s5k4ca_iso_auto");
			break;
		case 1:
			printk("-> ISO 50\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso50,"s5k4ca_iso50");
			break;
		case 2:
			printk("-> ISO 100\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso100,"s5k4ca_iso100");
			break;
		case 3:
			printk("-> ISO 200\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso200,"s5k4ca_iso200");
			break;
		case 4:
			printk("-> ISO 400\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso400,"s5k4ca_iso400");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_change_photometry(struct i2c_client *client, int type)
{
	int size;
	
	printk("[CAM-SENSOR] =Photometry Mode %d",type);

	switch (type)
	{
		case 0:
			printk("-> Photometry SPOT\n");
			s5k4ca_sensor_write_list(client,s5k4ca_measure_brightness_spot,"s5k4ca_measure_brightness_spot");
			break;
		case 1:
		default :
			printk("-> Photometry Default\n");
			s5k4ca_sensor_write_list(client,s5k4ca_measure_brightness_default,"s5k4ca_measure_brightness_default");
			break;
		case 2:
			printk("-> Photometry CENTER\n");
			s5k4ca_sensor_write_list(client,s5k4ca_measure_brightness_center,"s5k4ca_measure_brightness_center");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_change_scene_mode(struct i2c_client *client, int type)
{
	int size;
	
	printk("[CAM-SENSOR] =Scene Mode %d",type);
	switch (previous_scene_mode)
	{
		case 0:
			printk("-> portrait off");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_portrait_off,"s5k4ca_scene_portrait_off");
			break;
		case 1:
			printk("-> landscape off");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_landscape_off,"s5k4ca_scene_landscape_off");
			break;
		case 2:
			printk("-> sports off");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_sports_off,"s5k4ca_scene_sports_off");
			break;
		case 3:
			printk("-> sunset off");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_sunset_off,"s5k4ca_scene_sunset_off");
			break;
		case 4:
			printk("-> dawn off");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_dawn_off,"s5k4ca_scene_dawn_off");
			break;
		case 5:
			printk("-> againstlight off");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_againstlight_off,"s5k4ca_scene_againstlight_off");
			break;
		case 6:
			printk("-> text off");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_text_off,"s5k4ca_scene_text_off");
			break;
		default :
			printk("-> UnKnow Scene Mode off");
			break;
	}

	switch (type)
	{
		case 0:
			printk("-> portrait\n");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_portrait_on,"s5k4ca_scene_portrait_on");
			break;
		case 1:
			printk("-> landscape\n");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_landscape_on,"s5k4ca_scene_landscape_on");
			break;
		case 2:
			printk("-> sports\n");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_sports_on,"s5k4ca_scene_sports_on");
			break;
		case 3:
			printk("-> sunset\n");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_sunset_on,"s5k4ca_scene_sunset_on");
			break;
		case 4:
			printk("-> dawn\n");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_dawn_on,"s5k4ca_scene_dawn_on");
			break;
		case 5:
			printk("-> againstlight\n");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_againstlight_on,"s5k4ca_scene_againstlight_on");
			break;
		case 6:
			printk("-> text\n");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_text_on,"s5k4ca_scene_text_on");
			break;
		default :
			printk("-> UnKnow Scene Mode\n");
			break;
	}

	previous_scene_mode = type;
	return 0;
}

static int s5k4ca_sensor_user_read(struct i2c_client *client, s5k4ca_short_t *r_data)
{
	s5k4ca_sensor_write(client, 0x002C, r_data->page);
	s5k4ca_sensor_write(client, 0x002E, r_data->subaddr);
	return s5k4ca_sensor_read(client, 0x0F12, &(r_data->value));
}

static int s5k4ca_sensor_user_write(struct i2c_client *client, unsigned short *w_data)
{
	return s5k4ca_sensor_write(client, w_data[0], w_data[1]);
}

static int s5k4ca_sensor_exif_read(struct i2c_client *client, exif_data_t *exif_data)
{
	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] +%s\n",__func__));

	int ret = 0;
	
//	unsigned short lux = 0;
	unsigned short extime = 0;

	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	/// exposure time
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x1C3C);
//	msleep(100);
	s5k4ca_sensor_read(client, 0x0F12, &extime);

//	msleep(10);

/*	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	/// Incident Light value 
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x12FE);
	msleep(100);
	s5k4ca_sensor_read(client, 0x0F12, &lux);*/

	exif_data->exposureTime = extime/100;
	exif_data->lux = lux_value;

	(printk("[CAM-SENSOR] =%s extime=%d, lux=%d,\n",__func__,extime/100,lux_value));
	lux_value = 0;

	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] +%s\n",__func__));
	return ret;
}

static int s5k4ca_sensor_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] +%s cmd=0x%x\n",__func__,cmd));
	struct v4l2_control *ctrl;
	unsigned short *w_data;		/* To support user level i2c */	
	s5k4ca_short_t *r_data;
	exif_data_t *exif_data;

	int ret=0;

	switch (cmd)
	{
		case SENSOR_INIT:
			sensor_init(client);
			break;

		case USER_ADD:
			break;

		case USER_EXIT:
			s5k4ca_sensor_exit();
			break;

		case SENSOR_EFFECT:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_effect(client, ctrl->value);
			break;

		case SENSOR_BRIGHTNESS:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_br(client, ctrl->value);
			break;

		case SENSOR_WB:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_wb(client, ctrl->value);
			break;

		case SENSOR_SCENE_MODE:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_scene_mode(client, ctrl->value);
			break;

		case SENSOR_PHOTOMETRY:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_photometry(client, ctrl->value);
			break;

		case SENSOR_ISO:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_iso(client, ctrl->value);
			break;

		case SENSOR_CONTRAST:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_contrast(client, ctrl->value);
			break;

		case SENSOR_SATURATION:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_saturation(client, ctrl->value);
			break;

		case SENSOR_SHARPNESS:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_sharpness(client, ctrl->value);
			break;

		case SENSOR_AF:
			ctrl = (struct v4l2_control *)arg;
			ret = s5k4ca_sensor_af_control(client, ctrl->value);
			break;

		case SENSOR_MODE_SET:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_mode_set(client, ctrl->value);
			break;

		case SENSOR_XGA:
			s5k4ca_sensor_change_size(client, SENSOR_XGA);	
			break;

		case SENSOR_QXGA:
			s5k4ca_sensor_change_size(client, SENSOR_QXGA);	
			break;

		case SENSOR_QSVGA:
			s5k4ca_sensor_change_size(client, SENSOR_QSVGA);
			break;

		case SENSOR_VGA:
			s5k4ca_sensor_change_size(client, SENSOR_VGA);
			break;

		case SENSOR_SVGA:
			s5k4ca_sensor_change_size(client, SENSOR_SVGA);
			break;

		case SENSOR_SXGA:
			s5k4ca_sensor_change_size(client, SENSOR_SXGA);
			break;

		case SENSOR_UXGA:
			s5k4ca_sensor_change_size(client, SENSOR_UXGA);
			break;

		case SENSOR_USER_WRITE:
			w_data = (unsigned short *)arg;
			s5k4ca_sensor_user_write(client, w_data);
			break;

		case SENSOR_USER_READ:
			r_data = (s5k4ca_short_t *)arg;
			s5k4ca_sensor_user_read(client, r_data);
			break;
	
		case SENSOR_FLASH_CAMERA:
			ctrl = (struct v4l2_control *)arg;
#ifdef CONFIG_FLASH_AAT1271A
			aat1271a_falsh_camera_control(ctrl->value);	
#endif			
			break;

		case SENSOR_FLASH_MOVIE:
			ctrl = (struct v4l2_control *)arg;
#ifdef CONFIG_FLASH_AAT1271A
			aat1271a_falsh_movie_control(ctrl->value);	
#endif
			break;

		case SENSOR_EXIF_DATA:
			exif_data = (exif_data_t *)arg;
			s5k4ca_sensor_exif_read(client, exif_data);	
			break;

		default:
			break;
	}
	__TRACE_CAM_SENSOR(printk("[CAM-SENSOR] -%s cmd=0x%x\n",__func__,cmd));
	return ret;
}

static struct i2c_driver s5k4ca_driver = {
	.driver = {
		.name = "s5k4ca",
	},
	.id = S5K4CA_ID,
	.attach_adapter = s5k4ca_sensor_attach_adapter,
	.detach_client = s5k4ca_sensor_detach,
	.command = s5k4ca_sensor_command
};


#ifdef CONFIG_LOAD_FILE

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

static char *s5k4ca_regs_table = NULL;

static int s5k4ca_regs_table_size;

void s5k4ca_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int i;
	int ret;
	mm_segment_t fs = get_fs();

	printk("%s %d\n", __func__, __LINE__);

	set_fs(get_ds());
#if 0
	filp = filp_open("/data/camera/s5k4ca.h", O_RDONLY, 0);
#else
	filp = filp_open("/sdcard/s5k4ca.h", O_RDONLY, 0);
#endif
	if (IS_ERR(filp)) {
		printk("file open error\n");
		return;
	}
	l = filp->f_path.dentry->d_inode->i_size;	
	printk("l = %ld\n", l);
	dp = kmalloc(l, GFP_KERNEL);
	if (dp == NULL) {
		printk("Out of Memory\n");
		filp_close(filp, current->files);
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	if (ret != l) {
		printk("Failed to read file ret = %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return;
	}

	filp_close(filp, current->files);
	
	set_fs(fs);

	s5k4ca_regs_table = dp;
	
	s5k4ca_regs_table_size = l;

	*((s5k4ca_regs_table + s5k4ca_regs_table_size) - 1) = '\0';

	printk("s5k4ca_regs_table 0x%08x, %ld\n", dp, l);
}

void s5k4ca_regs_table_exit(void)
{
	printk("%s %d\n", __func__, __LINE__);
	if (s5k4ca_regs_table) {
		kfree(s5k4ca_regs_table);
		s5k4ca_regs_table = NULL;
	}	
}

static int s5k4ca_regs_table_write(char *name)
{
	char *start, *end, *reg, *data;	
	unsigned short addr, value;
	char reg_buf[7], data_buf[7];

	*(reg_buf + 6) = '\0';
	*(data_buf + 6) = '\0';

	start = strstr(s5k4ca_regs_table, name);
	
	end = strstr(start, "};");

	while (1) {	
		/* Find Address */	
		reg = strstr(start,"{0x");		
		if (reg)
			start = (reg + 16);
		if ((reg == NULL) || (reg > end))
			break;
		/* Write Value to Address */	
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 1), 6);	
			memcpy(data_buf, (reg + 9), 6);	
			addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16); 
			value = (unsigned short)simple_strtoul(data_buf, NULL, 16); 
//			printk("addr 0x%04x, value 0x%04x\n", addr, value);
			if (addr == 0xdddd)
			{
/*				if (value == 0x0010)
					mdelay(10);
				else if (value == 0x0020)
					mdelay(20);
				else if (value == 0x0030)
					mdelay(30);
				else if (value == 0x0040)
					mdelay(40);
				else if (value == 0x0050)
					mdelay(50);
				else if (value == 0x0100)
					mdelay(100);*/
				mdelay(value);
				printk("delay 0x%04x, value 0x%04x\n", addr, value);
			}	
			else
				s5k4ca_sensor_write(s5k4ca_data.sensor, addr, value);
		}
	}

	return 0;
}
#endif

static int s5k4ca_sensor_init(void)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	int ret;

#ifdef CONFIG_LOAD_FILE
	s5k4ca_regs_table_init();
#endif

//	s5k4ca_sensor_enable();
	
	s3c_camif_open_sensor(&s5k4ca_data);

	if (s5k4ca_data.sensor == NULL)
		if ((ret = i2c_add_driver(&s5k4ca_driver)))
			return ret;

	if (s5k4ca_data.sensor == NULL) {
		i2c_del_driver(&s5k4ca_driver);	
		return -ENODEV;
	}

	s3c_camif_register_sensor(&s5k4ca_data);
	(printk("[CAM-SENSOR] -%s\n",__func__));
	return 0;
}

static void s5k4ca_sensor_exit(void)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	s5k4ca_sensor_disable();

#ifdef CONFIG_LOAD_FILE
	s5k4ca_regs_table_exit();
#endif
	
	if (s5k4ca_data.sensor != NULL)
		s3c_camif_unregister_sensor(&s5k4ca_data);
	(printk("[CAM-SENSOR] -%s\n",__func__));
}

static struct v4l2_input s5k4ca_input = {
	.index		= 0,
	.name		= "Camera Input (S5K4CA)",
	.type		= V4L2_INPUT_TYPE_CAMERA,
	.audioset	= 1,
	.tuner		= 0,
	.std		= V4L2_STD_PAL_BG | V4L2_STD_NTSC_M,
	.status		= 0,
};

static struct v4l2_input_handler s5k4ca_input_handler = {
	s5k4ca_sensor_init,
	s5k4ca_sensor_exit	
};

#ifdef CONFIG_VIDEO_SAMSUNG_MODULE
static int s5k4ca_sensor_add(void)
{
	(printk("[CAM-SENSOR] =%s\n",__func__));
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_init();
#endif
	return s3c_camif_add_sensor(&s5k4ca_input, &s5k4ca_input_handler);
}

static void s5k4ca_sensor_remove(void)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	if (s5k4ca_data.sensor != NULL)
		i2c_del_driver(&s5k4ca_driver);
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_exit();
#endif
	s3c_camif_remove_sensor(&s5k4ca_input, &s5k4ca_input_handler);
	(printk("[CAM-SENSOR] -%s\n",__func__));
}

module_init(s5k4ca_sensor_add)
module_exit(s5k4ca_sensor_remove)

MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("I2C Client Driver For FIMC V4L2 Driver");
MODULE_LICENSE("GPL");
#else
int s5k4ca_sensor_add(void)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_init();
#endif	
#ifdef LOCAL_CONFIG_S5K4CA_I2C_TEST
	return s5k4ca_sensor_init();
#else
	(printk("[CAM-SENSOR] -%s\n",__func__));
	return s3c_camif_add_sensor(&s5k4ca_input, &s5k4ca_input_handler);
#endif
}

void s5k4ca_sensor_remove(void)
{
	(printk("[CAM-SENSOR] +%s\n",__func__));
	if (s5k4ca_data.sensor != NULL)
		i2c_del_driver(&s5k4ca_driver);
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_exit();
#endif
	s3c_camif_remove_sensor(&s5k4ca_input, &s5k4ca_input_handler);
	(printk("[CAM-SENSOR] -%s\n",__func__));
}
#endif
