#ifndef __VIBETONZ_H__
#define __VIBETONZ_H__


#include <linux/fcntl.h>


/****************************************************************************
** debugging routine
*****************************************************************************/

static int VIBETONZ_DEBUG_LEVEL = 3;	// Debug Level

#define VIBETONZ_DEBUG
#ifdef VIBETONZ_DEBUG
#define DPRINTK(N, X...) if (N <= VIBETONZ_DEBUG_LEVEL) \
		do { printk("%s()[%d]: ", __FUNCTION__,__LINE__); printk(X); } while(0)
#else
#define DPRINTK(N, x...)
#endif

/****************************************************************************
** definitions
*****************************************************************************/

#define ERROR_SUCCESS				0
#define TRUE						1

// device node
#define VIBETONZ_DEV_NAME 			"vibrator"
#define VIBETONZ_DEV_MAJOR 			247


/****************************************************************************
** structures
*****************************************************************************/
//khoonk add a data
typedef struct {
	int	f_disable;
	int	level;
} VIBRATOR_DATA;

#define VIBRATOR_ENABLE		1
#define VIBRATOR_DISABLE	0

#define IOC_SEC_MAGIC		(0xF0)

//#define	IOCTL_SET_VIBETONZ		_IOR(IOC_SEC_MAGIC, 0xD2, VIBRATOR_DATA)
//it will be changed to other side~~
#define IOCTL_SET_VIBETONZ		_IOW (IOC_SEC_MAGIC, 0xD2, int)

#endif // __VIBETONZ_H__
