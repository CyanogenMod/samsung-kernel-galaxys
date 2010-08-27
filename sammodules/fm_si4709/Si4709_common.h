#ifndef _COMMON_H
#define _COMMON_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <mach/gpio-jupiter.h>

//#define Si4709_DEBUG

#define error(fmt,arg...) printk(KERN_CRIT fmt "\n",## arg)

#ifdef Si4709_DEBUG
#define debug(fmt,arg...) printk(KERN_CRIT "--------" fmt "\n",## arg)
#else
#define debug(fmt,arg...)
#endif

#if !(defined CONFIG_JUPITER_VER_B4) 
#define FM_RESET			GPIO_FM_RST
#else
#define FM_RESET			GPIO_FM_BUS_nRST
#endif

/*VNVS:28-OCT'09---- For testing FM tune and seek operation status*/
#define TEST_FM   

#define YES  1
#define NO  0

#endif

