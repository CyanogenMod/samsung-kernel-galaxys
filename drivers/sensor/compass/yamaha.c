 
 /*****************************************************************************
 *
 * COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2015 ALL RIGHTS RESERVED
 *
 *****************************************************************************/
 


#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>

#define YAMAHA_GSENSOR_TRANSFORMATION_EMUL    \
    { { -1,  0,  0}, \
      { 0,  1,  0}, \
      { 0,  0,  1} }

#define YAMAHA_MSENSOR_TRANSFORMATION_EMUL    \
    { { 1,  0,  0}, \
      { 0,  1,   0}, \
      { 0 , 0 ,  1} }

//Real target      

#define YAMAHA_MSENSOR_TRANSFORMATION_00    \
    { { 0,  -1,  0}, \
      { -1,  0,   0}, \
      { 0 , 0 ,  -1} }

#define YAMAHA_GSENSOR_TRANSFORMATION    \
    { { -1,  0,  0}, \
      { 0,  -1,  0}, \
      { 0,  0,  -1} }

#define YAMAHA_MSENSOR_TRANSFORMATION    \
    { { -1,  0,  0}, \
      { 0,  1,   0}, \
      { 0 , 0 ,  -1} }

#if defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)
#define YAMAHA_GSENSOR_TRANSFORMATION_KEPLER    \
    { { 0,  1,  0}, \
      { -1,  0,  0}, \
      { 0,  0,  -1} }
#define YAMAHA_MSENSOR_TRANSFORMATION_KEPLER    \
    { { 0,  -1,  0}, \
      { -1,  0,   0}, \
      { 0 , 0 ,  -1} }

#define YAMAHA_MSENSOR_TRANSFORMATION_KEPLER_B5     \
    { { 0,  1,  0}, \
      { -1,  0,  0}, \
      { 0 , 0 , 1} }



#elif defined(CONFIG_S5PC110_T959_BOARD)
#define YAMAHA_GSENSOR_TRANSFORMATION_KEPLER    \
    { { -1,  0,  0}, \
      { 0,  -1,  0}, \
      { 0,  0,  -1} }

#define YAMAHA_MSENSOR_TRANSFORMATION_KEPLER    \
    { { -1,  0,  0}, \
      { 0,  1,   0}, \
      { 0 , 0 ,  -1} }


#endif

#define YAMAHA_IOCTL_GET_MARRAY            _IOR('Y', 0x01, char[9])
#define YAMAHA_IOCTL_GET_GARRAY            _IOR('Y', 0x02, char[9])

extern unsigned int HWREV;

static int yamaha_open(struct inode *inode, struct file *file)
{
	//printk(" yamaha_open() entered\n");
	return 0;
}

static int 
yamaha_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
{
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)
	signed char marray[3][3] = YAMAHA_MSENSOR_TRANSFORMATION_KEPLER;
	signed char garray[3][3] = YAMAHA_GSENSOR_TRANSFORMATION_KEPLER;
	signed char marray_B5[3][3] = YAMAHA_MSENSOR_TRANSFORMATION_KEPLER_B5;

#elif  defined(CONFIG_S5PC110_T959_BOARD)
	signed char marray[3][3] = YAMAHA_MSENSOR_TRANSFORMATION_KEPLER;
	signed char garray[3][3] = YAMAHA_GSENSOR_TRANSFORMATION_KEPLER;
#else
	signed char marray[3][3] = YAMAHA_MSENSOR_TRANSFORMATION;
	signed char garray[3][3] = YAMAHA_GSENSOR_TRANSFORMATION;
	signed char marray_emul[3][3] = YAMAHA_MSENSOR_TRANSFORMATION_EMUL;
	signed char garray_emul[3][3] = YAMAHA_GSENSOR_TRANSFORMATION_EMUL;
        signed char marray_00[3][3] = YAMAHA_MSENSOR_TRANSFORMATION_00;
#endif

	switch (cmd) {
		case YAMAHA_IOCTL_GET_MARRAY:

#if defined(CONFIG_S5PC110_KEPLER_BOARD) ||defined(CONFIG_S5PC110_FLEMING_BOARD)
				if(!(HWREV == 0x08 || HWREV == 0x04 || HWREV == 0x0C || HWREV == 0x02 || HWREV == 0x0A))  // 
				{
					if (copy_to_user((void *)arg, marray_B5, sizeof(marray_B5))) 
					{
						printk("YAMAHA_GSENSOR_TRANSFORMATION_EMUL copy failed\n");
						return -EFAULT;
					}

				}
				else
				{
				if (copy_to_user((void *)arg, marray, sizeof(marray))) 
				{
					printk("YAMAHA_GSENSOR_TRANSFORMATION_EMUL copy failed\n");
					return -EFAULT;
				}
				}
			//	printk("YAMAHA_GSENSOR_TRANSFORMATION_EMUL copy\n");
#elif  defined(CONFIG_S5PC110_T959_BOARD)

				if (copy_to_user((void *)arg, marray, sizeof(marray))) 
				{
					printk("YAMAHA_GSENSOR_TRANSFORMATION_EMUL copy failed\n");
					return -EFAULT;
				}


#else			
                     if(HWREV>=0xb)
			{
				if (copy_to_user((void *)arg, marray, sizeof(marray))) 
				{
					printk("YAMAHA_MSENSOR_TRANSFORMATION copy failed\n");
					return -EFAULT;
				}
			}
			else if((HWREV%2) ||(HWREV>=0xa))
			{
				if (copy_to_user((void *)arg, marray_00, sizeof(marray))) 
				{
					printk("YAMAHA_MSENSOR_TRANSFORMATION copy failed\n");
					return -EFAULT;
				}
			}
			else
			{
				if (copy_to_user((void *)arg, marray_emul, sizeof(marray))) 
				{
					printk("YAMAHA_MSENSOR_TRANSFORMATION_EMUL copy failed\n");
					return -EFAULT;
				}
			}
#endif 
			break;
		case YAMAHA_IOCTL_GET_GARRAY:
			
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD) || defined(CONFIG_S5PC110_T959_BOARD)
			if (copy_to_user((void *)arg, garray, sizeof(garray))) 
			{
				printk("YAMAHA_GSENSOR_TRANSFORMATION copy failed\n");
				return -EFAULT;
			}
			
#else			
			if((HWREV%2) ||(HWREV>=0xa))
			{
				if (copy_to_user((void *)arg, garray, sizeof(garray))) 
				{
					printk("YAMAHA_GSENSOR_TRANSFORMATION copy failed\n");
					return -EFAULT;
				}
			}
			else
			{
				if (copy_to_user((void *)arg, garray_emul, sizeof(garray))) 
				{
					printk("YAMAHA_GSENSOR_TRANSFORMATION_EMUL copy failed\n");
					return -EFAULT;
				}
			}
#endif
			break;
		default:
			return -ENOTTY;
	}

	return 0;
}

/*
static ssize_t
yamaha_read(struct file *file, char __user * buffer,
		size_t size, loff_t * pos)
{
	signed char marray[3][3] = YAMAHA_MSENSOR_TRANSFORMATION;
	signed char garray[3][3] = YAMAHA_GSENSOR_TRANSFORMATION;
	signed char marray_emul[3][3] = YAMAHA_MSENSOR_TRANSFORMATION_EMUL;
	signed char garray_emul[3][3] = YAMAHA_GSENSOR_TRANSFORMATION_EMUL;

	printk(" yamaha_read() entered\n");

	if (copy_to_user(buffer, marray, sizeof(unsigned int))) {
		return -EFAULT;
	}
	return sizeof(unsigned int);
}
*/
static int yamaha_release(struct inode *inode, struct file *file)
{
	//printk(" yamaha_release() entered\n");
	return 0;
}

static struct file_operations yamaha_fops = {
	.owner = THIS_MODULE,
	.open = yamaha_open,
	.release = yamaha_release,
	.ioctl = yamaha_ioctl,
};


static struct miscdevice yamaha_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "yamaha_compass",
	.fops = &yamaha_fops,
};

static int __init yamaha_init(void)
{
	int err;
	//printk("yamaha_init\n");
	err = misc_register(&yamaha_device);
	if (err) {
		printk("yamaha_init Cannot register miscdev \n",err);
	}
	return err;
}

static void __exit yamaha_exit(void)
{
	misc_deregister(&yamaha_device);
	printk("__exit\n");
}

module_init(yamaha_init);
module_exit(yamaha_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("yamaha compass driver");
MODULE_LICENSE("GPL");
