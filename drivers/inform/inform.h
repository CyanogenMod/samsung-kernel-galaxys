/*
 *  chardev.h - the header file with the ioctl definitions.
 *
 *  The declarations here have to be in a header file, because
 *  they need to be known both to the kernel module
 *  (in chardev.c) and the process calling ioctl (ioctl.c)
 */

#ifndef BTWLAN_GPIO_H
#define BTWLAN_GPIO_H

#include <linux/ioctl.h>

/* 
 * The major device number. We can't rely on dynamic 
 * registration any more, because ioctls need to know 
 * it. 
 */
#define MAJOR_NUM 208

/* 
 * Set the message of the device driver 
 */
#define IOCTL_BTPWRON  	1
#define IOCTL_BTPWROFF 		(0x00900002)
#define IOCTL_BTSUSPENDLINE 	(0x00900003)
#define IOCTL_BTWAKELINE	(0x00900004)
#define IOCTL_BTREADSUSPEND 	(0x00900005)


#define DEVICE_NAME "inform"

#define SUCCESS 0

#endif
