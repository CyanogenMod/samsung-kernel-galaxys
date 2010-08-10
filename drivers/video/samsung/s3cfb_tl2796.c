/*
 * s6e63m0 AMOLED Panel Driver for the Samsung Universal board
 *
 * Derived from drivers/video/omap/lcd-apollon.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/lcd.h>
#include <linux/backlight.h>


#include <plat/gpio-cfg.h>
#include <plat/regs-lcd.h>

#include "s3cfb.h"

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK			0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

#define DIM_BL	20
#define MIN_BL	30
#define MAX_BL	255

#define MAX_GAMMA_VALUE	24	// we have 25 levels. -> 16 levels -> 24 levels
#define CRITICAL_BATTERY_LEVEL 5

#define GAMMASET_CONTROL //for 1.9/2.2 gamma control from platform
#define ACL_ENABLE


/*********** for debug **********************************************************/
#if 0 
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/



static int locked = 0;
static int ldi_enable = 0;
int backlight_level = 0;
int bd_brightness = 0;

int current_gamma_value = -1;
int spi_ing = 0;
int on_19gamma = 0;

static DEFINE_MUTEX(spi_use);

//extern unsigned int get_battery_level(void);
//extern unsigned int is_charging_enabled(void);

struct s5p_lcd{
	struct spi_device *g_spi;
	struct lcd_device *lcd_dev;
	struct backlight_device *bl_dev;

};

#ifdef GAMMASET_CONTROL
struct class *gammaset_class;
struct device *switch_gammaset_dev;
#endif

#ifdef ACL_ENABLE
int acl_enable = 0;
int cur_acl = 0;

struct class *acl_class;
struct device *switch_aclset_dev;
#endif

#ifdef CONFIG_FB_S3C_MDNIE
extern void init_mdnie_class(void);
#endif


static struct s5p_lcd lcd;

const unsigned short s6e63m0_SEQ_DISPLAY_ON[] = {
	//insert
	0x029,
	ENDDEF, 0x0000
};

const unsigned short s6e63m0_SEQ_DISPLAY_OFF[] = {
	//insert
	0x028,
	ENDDEF, 0x0000
};

const unsigned short s6e63m0_SEQ_STANDBY_ON[] = {
	0x010,
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

const unsigned short s6e63m0_SEQ_STANDBY_OFF[] = {
	0x011,
	SLEEPMSEC, 	120,
	ENDDEF, 0x0000
};

const unsigned short s6e63m0_SEQ_SETTING[] = {
	SLEEPMSEC, 10,
	//panel setting
	0x0F8,
	0x101,	0x127,
	0x127,	0x107,
	0x107,	0x154,
	0x19F,	0x163,
	0x186,	0x11A,
	0x133,	0x10D,
	0x100,	0x100,
	
	//display condition set
	0x0F2,
	0x102,	0x103,
	0x11C,	0x110,
	0x110,
	
	0x0F7,
#ifdef CONFIG_S5PC110_FLEMING_BOARD
	0x100,
#else
	0x103,
#endif
	0x100,	
	0x100,
	
	//etc condition set
	0x0F6,
	0x100,	0x18E,
	0x107,
	
	0x0B3,
	0x16C,
	
	0x0B5,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	
	0x0B6,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	
	0x0B7,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,

	0x0B8,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,

	0x0B9,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	
	0x0BA,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	
	ENDDEF, 0x0000
};

const unsigned short s6e63m0_22gamma_300cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x170,
	0x16E,
	0x14E,
	0x1BC,
	0x1C0,
	0x1AF,
	0x1B3,
	0x1B8,
	0x1A5,
	0x1C5,
	0x1C7,
	0x1BB,
	0x100,
	0x1B9,
	0x100,
	0x1B8,
	0x100,
	0x1FC,
                                        
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_290cd[] = {
	//gamma set                                   
	0x0FA,                                        
                                                
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x171,
	0x170,
	0x150,
	0x1BD,
	0x1C1,
	0x1B0,
	0x1B2,
	0x1B8,
	0x1A4,
	0x1C6,
	0x1C7,
	0x1BB,
	0x100,
	0x1B6,
	0x100,
	0x1B6,
	0x100,
	0x1FA,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_280cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x16E,
	0x16C,
	0x14D,
	0x1BE,
	0x1C3,
	0x1B1,
	0x1B3,
	0x1B8,
	0x1A5,
	0x1C6,
	0x1C8,
	0x1BB,
	0x100,
	0x1B4,
	0x100,
	0x1B3,
	0x100,
	0x1F7,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_270cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x171,
	0x16C,
	0x150,
	0x1BD,
	0x1C3,
	0x1B0,
	0x1B4,
	0x1B8,
	0x1A6,
	0x1C6,
	0x1C9,
	0x1BB,
	0x100,
	0x1B2,
	0x100,
	0x1B1,
	0x100,
	0x1F4,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_260cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x174,
	0x16E,
	0x154,
	0x1BD,
	0x1C2,
	0x1B0,
	0x1B5,
	0x1BA,
	0x1A7,
	0x1C5,
	0x1C9,
	0x1BA,
	0x100,
	0x1B0,
	0x100,
	0x1AE,
	0x100,
	0x1F1,
                                       
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_250cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x174,
	0x16D,
	0x154,
	0x1BF,
	0x1C3,
	0x1B2,
	0x1B4,
	0x1BA,
	0x1A7,
	0x1C6,
	0x1CA,
	0x1BA,
	0x100,
	0x1AD,
	0x100,
	0x1AB,
	0x100,
	0x1ED,
                                        
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_240cd[] = {
	//gamma set                                   
	0x0FA,                                        
                                                
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x176,
	0x16F,
	0x156,
	0x1C0,
	0x1C3,
	0x1B2,
	0x1B5,
	0x1BA,
	0x1A8,
	0x1C6,
	0x1CB,
	0x1BB,
	0x100,
	0x1AA,
	0x100,
	0x1A8,
	0x100,
	0x1E9,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_230cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x175,
	0x16F,
	0x156,
	0x1BF,
	0x1C3,
	0x1B2,
	0x1B6,
	0x1BB,
	0x1A8,
	0x1C7,
	0x1CB,
	0x1BC,
	0x100,
	0x1A8,
	0x100,
	0x1A6,
	0x100,
	0x1E6,
                                        
	                                              
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_220cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x178,
	0x16F,
	0x158,
	0x1BF,
	0x1C4,
	0x1B3,
	0x1B5,
	0x1BB,
	0x1A9,
	0x1C8,
	0x1CC,
	0x1BC,
	0x100,
	0x1A6,
	0x100,
	0x1A3,
	0x100,
	0x1E2,
                                        
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_210cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x179,
	0x16D,
	0x157,
	0x1C0,
	0x1C4,
	0x1B4,
	0x1B7,
	0x1BD,
	0x1AA,
	0x1C8,
	0x1CC,
	0x1BD,
	0x100,
	0x1A2,
	0x100,
	0x1A0,
	0x100,
	0x1DD,
                                       
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_200cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x179,
	0x16D,
	0x158,
	0x1C1,
	0x1C4,
	0x1B4,
	0x1B6,
	0x1BD,
	0x1AA,
	0x1CA,
	0x1CD,
	0x1BE,
	0x100,
	0x19F,
	0x100,
	0x19D,
	0x100,
	0x1D9,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_190cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x17A,
	0x16D,
	0x159,
	0x1C1,
	0x1C5,
	0x1B4,
	0x1B8,
	0x1BD,
	0x1AC,
	0x1C9,
	0x1CE,
	0x1BE,
	0x100,
	0x19D,
	0x100,
	0x19A,
	0x100,
	0x1D5,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_180cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,
	0x118,	
	0x108,
	0x124,
	0x17B,
	0x16D,
	0x15B,
	0x1C0,
	0x1C5,
	0x1B3,
	0x1BA,
	0x1BE,
	0x1AD,
	0x1CA,
	0x1CE,
	0x1BF,
	0x100,
	0x199,
	0x100,
	0x197,
	0x100,
	0x1D0,
                                        
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_170cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x17C,
	0x16D,
	0x15C,
	0x1C0,
	0x1C6,
	0x1B4,
	0x1BB,
	0x1BE,
	0x1AD,
	0x1CA,
	0x1CF,
	0x1C0,
	0x100,
	0x196,
	0x100,
	0x194,
	0x100,
	0x1CC,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_160cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x17F,
	0x16E,
	0x15F,
	0x1C0,
	0x1C6,
	0x1B5,
	0x1BA,
	0x1BF,
	0x1AD,
	0x1CB,
	0x1CF,
	0x1C0,
	0x100,
	0x194,
	0x100,
	0x191,
	0x100,
	0x1C8,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_150cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x180,
	0x16E,
	0x15F,
	0x1C1,
	0x1C6,
	0x1B6,
	0x1BC,
	0x1C0,
	0x1AE,
	0x1CC,
	0x1D0,
	0x1C2,
	0x100,
	0x18F,
	0x100,
	0x18D,
	0x100,
	0x1C2,
                                        
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_140cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x180,
	0x16C,
	0x15F,
	0x1C1,
	0x1C6,
	0x1B7,
	0x1BC,
	0x1C1,
	0x1AE,
	0x1CD,
	0x1D0,
	0x1C2,
	0x100,
	0x18C,
	0x100,
	0x18A,
	0x100,
	0x1BE,
                                        
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_130cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x18C,
	0x16C,
	0x160,
	0x1C3,
	0x1C7,
	0x1B9,
	0x1BC,
	0x1C1,
	0x1AF,
	0x1CE,
	0x1D2,
	0x1C3,
	0x100,
	0x188,
	0x100,
	0x186,
	0x100,
	0x1B8,
                                        
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_120cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x182,
	0x16B,
	0x15E,
	0x1C4,
	0x1C8,
	0x1B9,
	0x1BD,
	0x1C2,
	0x1B1,
	0x1CE,
	0x1D2,
	0x1C4,
	0x100,
	0x185,
	0x100,
	0x182,
	0x100,
	0x1B3,
                                        
	                                              
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_110cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x186,
	0x16A,
	0x160,
	0x1C5,
	0x1C7,
	0x1BA,
	0x1BD,
	0x1C3,
	0x1B2,
	0x1D0,
	0x1D4,
	0x1C5,
	0x100,
	0x180,
	0x100,
	0x17E,
	0x100,
	0x1AD,
                                        
	                                              
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
                                                
const unsigned short s6e63m0_22gamma_100cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x186,
	0x169,
	0x160,
	0x1C6,
	0x1C8,
	0x1BA,
	0x1BF,
	0x1C4,
	0x1B4,
	0x1D0,
	0x1D4,
	0x1C6,
	0x100,
	0x17C,
	0x100,
	0x17A,
	0x100,
	0x1A7,
                                        
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_90cd[] = { 
	//gamma set                                   
	0x0FA,                                        
                                                
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x1B9,
	0x169,
	0x164,
	0x1C7,
	0x1C8,
	0x1BB,
	0x1C0,
	0x1C5,
	0x1B4,
	0x1D2,
	0x1D5,
	0x1C9,
	0x100,
	0x177,
	0x100,
	0x176,
	0x100,
	0x1A0,
                                        
                                                
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_80cd[] = { 
	//gamma set                                   
	0x0FA,                                        
                                                
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x189,
	0x168,
	0x165,
	0x1C9,
	0x1C9,
	0x1BC,
	0x1C1,
	0x1C5,
	0x1B6,
	0x1D2,
	0x1D5,
	0x1C9,
	0x100,
	0x173,
	0x100,
	0x172,
	0x100,
	0x19A,
                                        
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_70cd[] = { 
	//gamma set                                   
	0x0FA,                                        
                                                
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x18E,
	0x162,
	0x16B,
	0x1C7,
	0x1C9,
	0x1BB,
	0x1C3,
	0x1C7,
	0x1B7,
	0x1D3,
	0x1D7,
	0x1CA,
	0x100,
	0x16E,
	0x100,
	0x16C,
	0x100,
	0x194,
                                        
                                                
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_60cd[] = { 
	//gamma set                                   
	0x0FA,                                        
                                                
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x191,
	0x15E,
	0x16E,
	0x1C9,
	0x1C9,
	0x1BD,
	0x1C4,
	0x1C9,
	0x1B8,
	0x1D3,
	0x1D7,
	0x1CA,
	0x100,
	0x169,
	0x100,
	0x167,
	0x100,
	0x18D,
                                        
                                                
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_22gamma_50cd[] = { 
	//gamma set                                   
	0x0FA,                                        

	0x102,
	
	0x118,                                        
	0x108,
	0x124,
	0x196,
	0x158,
	0x172,
	0x1CB,
	0x1CA,
	0x1BF,
	0x1C6,
	0x1C9,
	0x1BA,
	0x1D6,
	0x1D9,
	0x1CD,
	0x100,
	0x161,
	0x100,
	0x161,
	0x100,
	0x183,
	                                      
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short *p22Gamma_set[] = {        
                                                
	s6e63m0_22gamma_50cd,//0                      
	s6e63m0_22gamma_70cd,                         
	s6e63m0_22gamma_80cd,                         
	s6e63m0_22gamma_90cd,                         
	s6e63m0_22gamma_100cd,                     
	s6e63m0_22gamma_110cd,  //5                      
	s6e63m0_22gamma_120cd,                        
	s6e63m0_22gamma_130cd,                        
	s6e63m0_22gamma_140cd,	                      
	s6e63m0_22gamma_150cd,                    
	s6e63m0_22gamma_160cd,   //10                     
	s6e63m0_22gamma_170cd,                        
	s6e63m0_22gamma_180cd,                        
	s6e63m0_22gamma_190cd,	                      
	s6e63m0_22gamma_200cd,                    
	s6e63m0_22gamma_210cd,  //15                      
	s6e63m0_22gamma_220cd,                        
	s6e63m0_22gamma_230cd,                        
	s6e63m0_22gamma_240cd,                        
	s6e63m0_22gamma_250cd,                   
	s6e63m0_22gamma_260cd,  //20                       
	s6e63m0_22gamma_270cd,                        
	s6e63m0_22gamma_280cd,                        
	s6e63m0_22gamma_290cd,                        
	s6e63m0_22gamma_300cd,//24                    
};                                              
                                                
const unsigned short s6e63m0_19gamma_300cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x179,
	0x17A,
	0x15B,
	0x1C1,
	0x1C5,
	0x1B5,
	0x1B8,
	0x1BD,
	0x1AB,
	0x1CB,
	0x1CE,
	0x1C1,
	0x100,
	0x1B8,
	0x100,
	0x1B7,
	0x100,
	0x1FC,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_290cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x178,
	0x17A,
	0x15B,
	0x1C2,
	0x1C7,
	0x1B6,
	0x1BA,
	0x1BE,
	0x1AC,
	0x1CB,
	0x1CE,
	0x1C2,
	0x100,
	0x1B8,
	0x100,
	0x1B6,
	0x100,
	0x1FB,
                                        
                                                
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_280cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,  
	
	0x118,
	0x108,
	0x124,
	0x17B,
	0x17D,
	0x15F,
	0x1C1,
	0x1C7,
	0x1B5,
	0x1BA,
	0x1BE,
	0x1AD,
	0x1CC,
	0x1CE,
	0x1C2,
	0x100,
	0x1B5,
	0x100,
	0x1B4,
	0x100,
	0x1F8,
                                        
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_270cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17C,
	0x17E,
	0x161,
	0x1C1,
	0x1C6,
	0x1B5,
	0x1BA,
	0x1BF,
	0x1AD,
	0x1CC,
	0x1CF,
	0x1C2,
	0x100,
	0x1B3,
	0x100,
	0x1B1,
	0x100,
	0x1F5,
                                        
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_260cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x179,
	0x17C,
	0x15E,
	0x1C3,
	0x1C7,
	0x1B6,
	0x1BA,
	0x1BF,
	0x1AE,
	0x1CC,
	0x1D0,
	0x1C2,
	0x100,
	0x1B1,
	0x100,
	0x1AE,
	0x100,
	0x1F2,
                                       
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_250cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x178,
	0x17C,
	0x15E,
	0x1C3,
	0x1C8,
	0x1B6,
	0x1BC,
	0x1C0,
	0x1AF,
	0x1CC,
	0x1CF,
	0x1C2,
	0x100,
	0x1AE,
	0x100,
	0x1AC,
	0x100,
	0x1EE,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_240cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17A,
	0x17F,
	0x161,
	0x1C2,
	0x1C7,
	0x1B6,
	0x1BC,
	0x1C1,
	0x1AF,
	0x1CE,
	0x1D0,
	0x1C3,
	0x100,
	0x1AB,
	0x100,
	0x1A9,
	0x100,
	0x1EA,
                            
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_230cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17A,
	0x17E,
	0x161,
	0x1C4,
	0x1C8,
	0x1B8,
	0x1BB,
	0x1C1,
	0x1AF,
	0x1CE,
	0x1D1,
	0x1C3,
	0x100,
	0x1A8,
	0x100,
	0x1A6,
	0x100,
	0x1E6,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_220cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17C,
	0x17F,
	0x162,
	0x1C5,
	0x1C8,
	0x1B9,
	0x1BC,
	0x1C1,
	0x1B0,
	0x1CE,
	0x1D2,
	0x1C3,
	0x100,
	0x1A5,
	0x100,
	0x1A3,
	0x100,
	0x1E2,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_210cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17B,
	0x17F,
	0x161,
	0x1C6,
	0x1C8,
	0x1B9,
	0x1BE,
	0x1C2,
	0x1B2,
	0x1CE,
	0x1D3,
	0x1C4,
	0x100,
	0x1A2,
	0x100,
	0x1A0,
	0x100,
	0x1DD,
                                       
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_200cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17B,
	0x17E,
	0x161,
	0x1C5,
	0x1C9,
	0x1B9,
	0x1BF,
	0x1C3,
	0x1B2,
	0x1CF,
	0x1D3,
	0x1C5,
	0x100,
	0x19F,
	0x100,
	0x19D,
	0x100,
	0x1D9,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_190cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17D,
	0x17F,
	0x163,
	0x1C5,
	0x1C9,
	0x1BA,
	0x1BF,
	0x1C4,
	0x1B2,
	0x1D0,
	0x1D3,
	0x1C6,
	0x100,
	0x19C,
	0x100,
	0x19A,
	0x100,
	0x1D5,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_180cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x180,
	0x182,
	0x165,
	0x1C6,
	0x1C9,
	0x1BB,
	0x1BF,
	0x1C4,
	0x1B3,
	0x1D0,
	0x1D4,
	0x1C6,
	0x100,
	0x199,
	0x100,
	0x197,
	0x100,
	0x1D0,
                                        
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_170cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17D,
	0x17F,
	0x161,
	0x1C7,
	0x1CA,
	0x1BB,
	0x1C0,
	0x1C5,
	0x1B5,
	0x1D1,
	0x1D4,
	0x1C8,
	0x100,
	0x196,
	0x100,
	0x194,
	0x100,
	0x1CB,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_160cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17C,
	0x17E,
	0x161,
	0x1C7,
	0x1CB,
	0x1BB,
	0x1C1,
	0x1C5,
	0x1B5,
	0x1D1,
	0x1D5,
	0x1C8,
	0x100,
	0x193,
	0x100,
	0x190,
	0x100,
	0x1C7,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_150cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17E,
	0x17F,
	0x163,
	0x1C8,
	0x1CB,
	0x1BC,
	0x1C1,
	0x1C6,
	0x1B6,
	0x1D2,
	0x1D6,
	0x1C8,
	0x100,
	0x18F,
	0x100,
	0x18D,
	0x100,
	0x1C2,
                                        
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_140cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17E,
	0x17F,
	0x165,
	0x1C8,
	0x1CC,
	0x1BC,
	0x1C2,
	0x1C6,
	0x1B6,
	0x1D3,
	0x1D6,
	0x1C9,
	0x100,
	0x18C,
	0x100,
	0x18A,
	0x100,
	0x1BE,
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_130cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x182,
	0x17F,
	0x169,
	0x1C8,
	0x1CC,
	0x1BD,
	0x1C2,
	0x1C7,
	0x1B6,
	0x1D4,
	0x1D7,
	0x1CB,
	0x100,
	0x188,
	0x100,
	0x186,
	0x100,
	0x1B8,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_120cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x183,
	0x17F,
	0x169,
	0x1C9,
	0x1CC,
	0x1BE,
	0x1C3,
	0x1C8,
	0x1B8,
	0x1D4,
	0x1D7,
	0x1CB,
	0x100,
	0x185,
	0x100,
	0x183,
	0x100,
	0x1B3,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_110cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x185,
	0x181,
	0x16A,
	0x1CB,
	0x1CD,
	0x1BF,
	0x1C4,
	0x1C8,
	0x1B9,
	0x1D5,
	0x1D9,
	0x1CC,
	0x100,
	0x180,
	0x100,
	0x17E,
	0x100,
	0x1AD,
                                        
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_100cd[] = {
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x186,
	0x181,
	0x16B,
	0x1CA,
	0x1CD,
	0x1BE,
	0x1C6,
	0x1C9,
	0x1BB,
	0x1D5,
	0x1DA,
	0x1CD,
	0x100,
	0x17C,
	0x100,
	0x17A,
	0x100,
	0x1A7,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_90cd[] = { 
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x188,
	0x181,
	0x16D,
	0x1CB,
	0x1CE,
	0x1C0,
	0x1C6,
	0x1CA,
	0x1BB,
	0x1D6,
	0x1DA,
	0x1CE,
	0x100,
	0x178,
	0x100,
	0x176,
	0x100,
	0x1A1,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_80cd[] = { 
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x18E,
	0x17F,
	0x172,
	0x1CB,
	0x1CF,
	0x1C1,
	0x1C6,
	0x1CB,
	0x1BB,
	0x1D8,
	0x1DB,
	0x1CF,
	0x100,
	0x173,
	0x100,
	0x171,
	0x100,
	0x19B,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_70cd[] = { 
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x192,
	0x17F,
	0x175,
	0x1CC,
	0x1CF,
	0x1C2,
	0x1C7,
	0x1CC,
	0x1BD,
	0x1D8,
	0x1DC,
	0x1CF,
	0x100,
	0x16E,
	0x100,
	0x16C,
	0x100,
	0x194,
                                        
		                                            
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_60cd[] = { 
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x193,
	0x17F,
	0x174,
	0x1CD,
	0x1CF,
	0x1C3,
	0x1CA,
	0x1CD,
	0x1C0,
	0x1D8,
	0x1DD,
	0x1D1,
	0x100,
	0x169,
	0x100,
	0x167,
	0x100,
	0x18C,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
const unsigned short s6e63m0_19gamma_50cd[] = { 
	//gamma set                                   
	0x0FA,                                        
	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x194,
	0x17C,
	0x173,
	0x1CF,
	0x1D0,
	0x1C5,
	0x1CA,
	0x1CE,
	0x1C0,
	0x1DB,
	0x1DF,
	0x1D4,
	0x100,
	0x162,
	0x100,
	0x160,
	0x100,
	0x183,
                                        
			                                          
                                                
	                                              
	//gamma update                                
	0x0FA,	                                      
	0x103,                                        
                                                
	SLEEPMSEC, 10,                                
                                                
	//Display on                                  
	0x029,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
                                                
const unsigned short *p19Gamma_set[] = {        
	s6e63m0_19gamma_50cd,//0                      
	//s6e63m0_19gamma_50cd,                         
	s6e63m0_19gamma_70cd,                         
	s6e63m0_19gamma_80cd,                         
	s6e63m0_19gamma_90cd,                         
	s6e63m0_19gamma_100cd, //5                    
	s6e63m0_19gamma_110cd,                        
	s6e63m0_19gamma_120cd,                        
	s6e63m0_19gamma_130cd,                        
	s6e63m0_19gamma_140cd,	                      
	s6e63m0_19gamma_150cd,  //10                  
	s6e63m0_19gamma_160cd,                        
	s6e63m0_19gamma_170cd,                        
	s6e63m0_19gamma_180cd,                        
	s6e63m0_19gamma_190cd,	                      
	s6e63m0_19gamma_200cd, //15                   
	s6e63m0_19gamma_210cd,                        
	s6e63m0_19gamma_220cd,                        
	s6e63m0_19gamma_230cd,                        
	s6e63m0_19gamma_240cd,                        
	s6e63m0_19gamma_250cd,//20                    
	s6e63m0_19gamma_260cd,                        
	s6e63m0_19gamma_270cd,                        
	s6e63m0_19gamma_280cd,                        
	s6e63m0_19gamma_290cd,                        
	s6e63m0_19gamma_300cd,//25                    
};                                           

#ifdef ACL_ENABLE
const unsigned short acl_cutoff_off[] = {
	//ACL Off
	0x0C0,
	0x100,

	ENDDEF, 0x0000 
};

// ACL INIT : delta Y is all zero.
const unsigned short acl_cutoff_init[] = {
	0x0F0,
	0x15A,
	0x15A,

	// ACL parameter set
	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x100,  0x100,  0x100,	
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

const unsigned short acl_cutoff_12p[] = {
	0x0F0,
	0x15A,
	0x15A,
	
	// ACL parameter set
	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x103,  0x104,       	
	0x106,  0x107,  0x109,  0x10A,       	
	0x10C,  0x10D,  0x10F,  0x110,	

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

const unsigned short acl_cutoff_22p[] = {
	0x0F0,
	0x15A,
	0x15A,

	// ACL parameter set
	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x104,  0x106,       	
	0x109,  0x10C,  0x10F,  0x111,       	
	0x114,  0x117,  0x119,  0x11C,	

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

const unsigned short acl_cutoff_30p[] = {
	0x0F0,
	0x15A,
	0x15A,
	
	// ACL parameter set
	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x105,  0x108,       	
	0x10C,  0x110,  0x114,  0x117,       	
	0x11B,  0x11F,  0x122,  0x126,	

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

const unsigned short acl_cutoff_35p[] = {
	0x0F0,
	0x15A,
	0x15A,

	// ACL parameter set
	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x106,  0x10A,       	
	0x10F,  0x113,  0x118,  0x11C,       	
	0x121,  0x125,  0x12A,  0x12E,	

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

const unsigned short acl_cutoff_40p[] = {
	0x0F0,
	0x15A,
	0x15A,
	
	// ACL parameter set
	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x106,  0x10B,       	
	0x111,  0x116,  0x11B,  0x120,       	
	0x125,  0x12B,  0x130,  0x135,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

const unsigned short *ACL_cutoff_set[] = {
	acl_cutoff_off,
	acl_cutoff_12p,
	acl_cutoff_22p,
	acl_cutoff_30p,
	acl_cutoff_35p,
	acl_cutoff_40p,
};
#endif


static struct s3cfb_lcd s6e63m0 = {
	.width = 480,
	.height = 800,
	.p_width = 52,
	.p_height = 86,
	.bpp = 24,
	.freq = 60,
	
	.timing = {
		.h_fp = 16,
		.h_bp = 16,
		.h_sw = 2,
		.v_fp = 28,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};


static int s6e63m0_spi_write_driver(int reg)
{
	u16 buf[1];
	int ret;
	struct spi_message msg;

	struct spi_transfer xfer = {
		.len	= 2,
		.tx_buf	= buf,
	};

	buf[0] = reg;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);


	ret = spi_sync(lcd.g_spi, &msg);

	if(ret < 0)
		err("%s::%d -> spi_sync failed Err=%d\n",__func__,__LINE__,ret);
	return ret ;

}


static void s6e63m0_spi_write(unsigned short reg)
{
  	s6e63m0_spi_write_driver(reg);	
}

static void s6e63m0_panel_send_sequence(const unsigned short *wbuf)
{
	int i = 0;

	mutex_lock(&spi_use);

	gprintk("#################SPI start##########################\n");
	
	//spi_ing = 1;
	
	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC){
			s6e63m0_spi_write(wbuf[i]);
			i+=1;}
		else{
			msleep(wbuf[i+1]);
			i+=2;}
	}
	
	//spi_ing = 0;

	gprintk("#################SPI end##########################\n");

	mutex_unlock(&spi_use);
}

int IsLDIEnabled(void)
{
	return ldi_enable;
}
EXPORT_SYMBOL(IsLDIEnabled);


static void SetLDIEnabledFlag(int OnOff)
{
	ldi_enable = OnOff;
}

void tl2796_ldi_init(void)
{
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_SETTING);
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_STANDBY_OFF);

	SetLDIEnabledFlag(1);
	printk(KERN_DEBUG "LDI enable ok\n");
	dev_dbg(lcd.lcd_dev,"%s::%d -> ldi initialized\n",__func__,__LINE__);	
}


void tl2796_ldi_enable(void)
{
}

void tl2796_ldi_disable(void)
{
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_STANDBY_ON);
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_DISPLAY_OFF);

	SetLDIEnabledFlag(0);
	printk(KERN_DEBUG "LDI disable ok\n");
	dev_dbg(lcd.lcd_dev,"%s::%d -> ldi disabled\n",__func__,__LINE__);	
}

void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	s6e63m0.init_ldi = NULL;
	ctrl->lcd = &s6e63m0;
}

//mkh:lcd operations and functions
static int s5p_lcd_set_power(struct lcd_device *ld, int power)
{
	printk("s5p_lcd_set_power is called: %d", power);
	if(power)
	{
		s6e63m0_panel_send_sequence(s6e63m0_SEQ_DISPLAY_ON);
	}
	else
{
		s6e63m0_panel_send_sequence(s6e63m0_SEQ_DISPLAY_OFF);
	}
}

static struct lcd_ops s5p_lcd_ops = {
	.set_power = s5p_lcd_set_power,
};

//mkh:backlight operations and functions

int get_bl_update_status(void)
{
	return bd_brightness;
}
EXPORT_SYMBOL(get_bl_update_status);


void bl_update_status_22gamma(int bl)
{
	int gamma_value = 0;
	int i;

	for(i=0; i<100; i++)
	{
		gprintk("ldi_enable : %d \n",ldi_enable);

		if(IsLDIEnabled())
			break;
		
		msleep(10);
	};

	if(!(current_gamma_value == -1))
	{
	gprintk("#################22gamma start##########################\n");
	s6e63m0_panel_send_sequence(p22Gamma_set[current_gamma_value]);
		gprintk("#################22gamma end##########################\n");
	}


	//printk("bl_update_status_22gamma : current_gamma_value(%d) \n",current_gamma_value);
}
EXPORT_SYMBOL(bl_update_status_22gamma);


void bl_update_status_19gamma(int bl)
{
	int gamma_value = 0;
	int i;

	for(i=0; i<100; i++)
	{
		gprintk("ldi_enable : %d \n",ldi_enable);

		if(IsLDIEnabled())
			break;
		
		msleep(10);
	};

	if(!(current_gamma_value == -1))
	{
	gprintk("#################19gamma start##########################\n");
	s6e63m0_panel_send_sequence(p19Gamma_set[current_gamma_value]);
	gprintk("#################19gamma end##########################\n");
	}

	//printk("bl_update_status_19gamma : current_gamma_value(%d) \n",current_gamma_value);
}
EXPORT_SYMBOL(bl_update_status_19gamma);


#ifdef GAMMASET_CONTROL //for 1.9/2.2 gamma control from platform
static ssize_t gammaset_file_cmd_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	gprintk("called %s \n",__func__);

	return sprintf(buf,"%u\n",bd_brightness);
}
static ssize_t gammaset_file_cmd_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	
    sscanf(buf, "%d", &value);

	//printk(KERN_INFO "[gamma set] in gammaset_file_cmd_store, input value = %d \n",value);
	if(IsLDIEnabled()==0)
	{
		//printk(KERN_DEBUG "[gamma set] return because LDI is disabled, input value = %d \n",value);
		printk("[gamma set] return because LDI is disabled, input value = %d \n",value);
		return size;
	}

	if(value==1 && on_19gamma==0)
	{
		on_19gamma = 1;
		bl_update_status_19gamma(bd_brightness);
	}
	else if(value==0 && on_19gamma==1)
	{
		on_19gamma = 0;
		bl_update_status_22gamma(bd_brightness);
	}
	else
		printk("\ngammaset_file_cmd_store value(%d) on_19gamma(%d) \n",value,on_19gamma);

	return size;
}

static DEVICE_ATTR(gammaset_file_cmd,0666, gammaset_file_cmd_show, gammaset_file_cmd_store);
#endif

#ifdef ACL_ENABLE 
static ssize_t aclset_file_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	gprintk("called %s \n",__func__);

	return sprintf(buf,"%u\n", acl_enable);
}
static ssize_t aclset_file_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	//printk(KERN_INFO "[acl set] in aclset_file_cmd_store, input value = %d \n", value);

	if(IsLDIEnabled()==0)
	{
		printk(KERN_DEBUG "[acl set] return because LDI is disabled, input value = %d \n",value);
		return size;
	}

	if(value==1 && acl_enable == 0)
	{
		acl_enable = value;
		
		s6e63m0_panel_send_sequence(acl_cutoff_init);
		msleep(20);
	
		if (current_gamma_value ==1)
		{
			s6e63m0_panel_send_sequence(ACL_cutoff_set[0]); //set 0% ACL
			cur_acl = 0;
			//printk(" ACL_cutoff_set Percentage : 0!!\n");
		}
		else if(current_gamma_value ==2)
		{
			s6e63m0_panel_send_sequence(ACL_cutoff_set[1]); //set 12% ACL
			cur_acl = 12;
			//printk(" ACL_cutoff_set Percentage : 12!!\n");
		}
		else if(current_gamma_value ==3)
		{
			s6e63m0_panel_send_sequence(ACL_cutoff_set[2]); //set 22% ACL
			cur_acl = 22;
			//printk(" ACL_cutoff_set Percentage : 22!!\n");
		}
		else if(current_gamma_value ==4)
		{
			s6e63m0_panel_send_sequence(ACL_cutoff_set[3]); //set 30% ACL
			cur_acl = 30;
			//printk(" ACL_cutoff_set Percentage : 30!!\n");
		}
		else if(current_gamma_value ==5)
		{
			s6e63m0_panel_send_sequence(ACL_cutoff_set[4]); //set 35% ACL
			cur_acl = 35;
			//printk(" ACL_cutoff_set Percentage : 35!!\n");
		}
		else
		{
			s6e63m0_panel_send_sequence(ACL_cutoff_set[5]); //set 40% ACL
			cur_acl = 40;
			//printk(" ACL_cutoff_set Percentage : 40!!\n");
		}
	}
	else if(value==0 && acl_enable == 1)
	{
		acl_enable = value;
		
		//ACL Off
		s6e63m0_panel_send_sequence(ACL_cutoff_set[0]); //ACL OFF
		//printk(" ACL_cutoff_set Percentage : 0!!\n");
		cur_acl  = 0;
	}
	else
		printk("\naclset_file_cmd_store value is same : value(%d)\n",value);

	return size;
}

static DEVICE_ATTR(aclset_file_cmd,0666, aclset_file_cmd_show, aclset_file_cmd_store);
#endif




#define MDNIE_TUNINGMODE_FOR_BACKLIGHT

#ifdef MDNIE_TUNINGMODE_FOR_BACKLIGHT
extern void mDNIe_Mode_set_for_backlight(u16 *buf);
extern u16 *pmDNIe_Gamma_set[];
extern int pre_val;
extern int autobrightness_mode;
#endif

static int s5p_bl_update_status(struct backlight_device* bd)
{

	int bl = bd->props.brightness;
	int level = 0;
	int gamma_value = 0;
	int gamma_val_x10 = 0;

	int i = 0;

	for(i=0; i<100; i++)
	{
		gprintk("ldi_enable : %d \n",ldi_enable);

		if(IsLDIEnabled())
			break;
		
		msleep(10);
	};
	
	gprintk("\nupdate status brightness[0~255] : (%d) \n",bd->props.brightness);

	if(IsLDIEnabled())
	{
	#if 0
		if (get_battery_level() <= CRITICAL_BATTERY_LEVEL && !is_charging_enabled())
		{
			if (bl > DIM_BL)
				bl = DIM_BL;
		}
	#endif
	if(bl == 0)
		level = 0;	//lcd off
		else if((bl < MIN_BL) && (bl > 0))
		level = 1;	//dimming
	else
		level = 6;	//normal

	if(level==0)
	{
		msleep(20);
		s6e63m0_panel_send_sequence(s6e63m0_SEQ_DISPLAY_OFF);
		gprintk("Update status brightness[0~255]:(%d) - LCD OFF \n", bl);
		bd_brightness = 0;
		backlight_level = 0;
		current_gamma_value = -1;
		return 0;
	}	

		if (bl >= MIN_BL)
		{
			gamma_val_x10 = 10*(MAX_GAMMA_VALUE-1)*bl/(MAX_BL-MIN_BL) + (10 - 10*(MAX_GAMMA_VALUE-1)*(MIN_BL)/(MAX_BL-MIN_BL)) ;
			gamma_value = (gamma_val_x10+5)/10;
		}	
		else
	{
			gamma_value = 0;
	}

	bd_brightness = bd->props.brightness;
	backlight_level = level;

		if(current_gamma_value == gamma_value)
		{
			return 0;
		}
	gprintk("Update status brightness[0~255]:(%d) gamma_value:(%d) on_19gamma(%d)\n", bl,gamma_value,on_19gamma);
		
 	if(level)
	{
			#ifdef MDNIE_TUNINGMODE_FOR_BACKLIGHT
			if((pre_val==1)&&(gamma_value < 24)&&(autobrightness_mode))
			{
				mDNIe_Mode_set_for_backlight(pmDNIe_Gamma_set[2]);
				gprintk("s5p_bl_update_status - pmDNIe_Gamma_set[2]\n" );
				pre_val = -1;
			}
			#endif
			
		switch(level)
		{
			case  5:
			case  4:
			case  3:
			case  2:
			case  1: //dimming
			{	
				if(on_19gamma)
						s6e63m0_panel_send_sequence(p19Gamma_set[0]);
				else
						s6e63m0_panel_send_sequence(p22Gamma_set[0]);

			
			#ifdef ACL_ENABLE
				if (acl_enable)
				{
					if (cur_acl != 0)
					{
						s6e63m0_panel_send_sequence(ACL_cutoff_set[0]); //set 0% ACL
						gprintk(" ACL_cutoff_set Percentage : 0!!\n");
						cur_acl = 0;
					}
				}
			#endif
				gprintk("call s5p_bl_update_status level : %d\n",level);
				break;
			}
			case  6:
			{								
			#ifdef ACL_ENABLE
				if (acl_enable)
				{
						if (gamma_value ==1)
					{
						if (cur_acl != 0)
						{
							s6e63m0_panel_send_sequence(ACL_cutoff_set[0]); //set 0% ACL
							cur_acl = 0;
							gprintk(" ACL_cutoff_set Percentage : 0!!\n");
						}
					}
						else
						{
							if (cur_acl == 0)
							{
								s6e63m0_panel_send_sequence(acl_cutoff_init);
								msleep(20);
							}
							
							if(gamma_value ==2)
					{
						if (cur_acl != 12)
						{
							s6e63m0_panel_send_sequence(ACL_cutoff_set[1]); //set 12% ACL
							cur_acl = 12;
							gprintk(" ACL_cutoff_set Percentage : 12!!\n");
						}
					}
							else if(gamma_value ==3)
					{
						if (cur_acl != 22)
						{
							s6e63m0_panel_send_sequence(ACL_cutoff_set[2]); //set 22% ACL
							cur_acl = 22;
							gprintk(" ACL_cutoff_set Percentage : 22!!\n");
						}
					}
							else if(gamma_value ==4)
					{
						if (cur_acl != 30)
						{
							s6e63m0_panel_send_sequence(ACL_cutoff_set[3]); //set 30% ACL
							cur_acl = 30;
							gprintk(" ACL_cutoff_set Percentage : 30!!\n");
						}
					}
							else if(gamma_value ==5)
					{
						if (cur_acl != 35)
						{
							s6e63m0_panel_send_sequence(ACL_cutoff_set[4]); //set 35% ACL
							cur_acl = 35;
							gprintk(" ACL_cutoff_set Percentage : 35!!\n");
						}
					}
					else
					{
						if(cur_acl !=40)
						{
							s6e63m0_panel_send_sequence(ACL_cutoff_set[5]); //set 40% ACL
							cur_acl = 40;
							gprintk(" ACL_cutoff_set Percentage : 40!!\n");
						}
					}
						}
					}
				#endif
	
					if(on_19gamma)
						s6e63m0_panel_send_sequence(p19Gamma_set[gamma_value]);
					else
						s6e63m0_panel_send_sequence(p22Gamma_set[gamma_value]);

					gprintk("#################backlight end##########################\n");
					
					break;
				}
			}
			
			current_gamma_value = gamma_value;
		}
	}
	
	return 0;
}

static int s5p_bl_get_brightness(struct backlilght_device* bd)
{
printk("\n reading brightness \n");
	return bd_brightness;
}

static struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
	.get_brightness = s5p_bl_get_brightness,	
};

static int __init tl2796_probe(struct spi_device *spi)
{
	int ret;

	spi->bits_per_word = 9;
	ret = spi_setup(spi);
	lcd.g_spi = spi;
	lcd.lcd_dev = lcd_device_register("s5p_lcd",&spi->dev,&lcd,&s5p_lcd_ops);
	lcd.bl_dev = backlight_device_register("s5p_bl",&spi->dev,&lcd,&s5p_bl_ops);
	lcd.bl_dev->props.max_brightness = 255;
	dev_set_drvdata(&spi->dev,&lcd);

        SetLDIEnabledFlag(1);

#ifdef GAMMASET_CONTROL //for 1.9/2.2 gamma control from platform
	gammaset_class = class_create(THIS_MODULE, "gammaset");
	if (IS_ERR(gammaset_class))
		pr_err("Failed to create class(gammaset_class)!\n");

	switch_gammaset_dev = device_create(gammaset_class, NULL, 0, NULL, "switch_gammaset");
	if (IS_ERR(switch_gammaset_dev))
		pr_err("Failed to create device(switch_gammaset_dev)!\n");

	if (device_create_file(switch_gammaset_dev, &dev_attr_gammaset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gammaset_file_cmd.attr.name);
#endif	

#ifdef ACL_ENABLE //ACL On,Off
	acl_class = class_create(THIS_MODULE, "aclset");
	if (IS_ERR(acl_class))
		pr_err("Failed to create class(acl_class)!\n");

	switch_aclset_dev = device_create(acl_class, NULL, 0, NULL, "switch_aclset");
	if (IS_ERR(switch_aclset_dev))
		pr_err("Failed to create device(switch_aclset_dev)!\n");

	if (device_create_file(switch_aclset_dev, &dev_attr_aclset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_aclset_file_cmd.attr.name);
#endif	

#ifdef CONFIG_FB_S3C_MDNIE
	init_mdnie_class();  //set mDNIe UI mode, Outdoormode
#endif

	if (ret < 0){
		err("%s::%d-> s6e63m0 probe failed Err=%d\n",__func__,__LINE__,ret);
		return 0;
	}
	info("%s::%d->s6e63m0 probed successfuly\n",__func__,__LINE__);
	return ret;
}

#ifdef CONFIG_PM // add by ksoo (2009.09.07)
int tl2796_suspend(struct platform_device *pdev, pm_message_t state)
{
	info("%s::%d->s6e63m0 suspend called\n",__func__,__LINE__);
	tl2796_ldi_disable();
	return 0;
}

int tl2796_resume(struct platform_device *pdev, pm_message_t state)
{
	info("%s::%d -> s6e63m0 resume called\n",__func__,__LINE__);
	tl2796_ldi_init();
	tl2796_ldi_enable();

	return 0;
}
#endif

static struct spi_driver tl2796_driver = {
	.driver = {
		.name	= "tl2796",
		.owner	= THIS_MODULE,
	},
	.probe		= tl2796_probe,
	.remove		= __exit_p(tl2796_remove),
//#ifdef CONFIG_PM
//	.suspend		= tl2796_suspend,
//	.resume		= tl2796_resume,
//#else
	.suspend		= NULL,
	.resume		= NULL,
//#endif
};

static int __init tl2796_init(void)
{
	return spi_register_driver(&tl2796_driver);
}

static void __exit tl2796_exit(void)
{
	spi_unregister_driver(&tl2796_driver);
}


module_init(tl2796_init);
module_exit(tl2796_exit);


MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("s6e63m0 LDI driver");
MODULE_LICENSE("GPL");
