#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#include "inform.h"


int inform_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t inform_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t inform_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

int inform_release (struct inode *inode, struct file *filp)
{
	return 0;
}

int inform_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_num,  unsigned long arg)
{
	unsigned char arg_data; 
	int err = 0;
	void __iomem * INFORM4;


	void __user *argp = (void __user *)arg;

	INFORM4 = ioremap_nocache(0xe010f010,0x1000);
	 printk("[DRV]ioctl_num:%x ,INFORM:%x\n",ioctl_num,*(volatile unsigned *) INFORM4);
	switch( ioctl_num )
	{
		case IOCTL_BTPWRON :
			{
				 copy_to_user( argp, (volatile unsigned *) INFORM4, sizeof(int) ); 
			 }
			break;
		

		default : 
			break;
	}

	return err;

}




struct file_operations Fops = {
	.owner   = THIS_MODULE,
	.ioctl 	 = inform_ioctl,
	.read    = inform_read,
	.write   = inform_write,
	.open 	 = inform_open,
	.release = inform_release,	
};

static int __init inform_init(void)
{
	int ret_val;
	ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

	if (ret_val < 0) {
		printk(KERN_ALERT "%s failed with %d\n",
		       "Registering the BT_GPO device ", ret_val);
		return ret_val;
	}

	printk(KERN_INFO "\n\nREBOOT INFORM Driver \n\n");
	printk(KERN_DEBUG "%s The major device number is %d.\n", "Registering 'BT GPIO is a success' ", MAJOR_NUM);
	return 0;
}


static void __exit inform_exit(void)
{
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
	printk (KERN_DEBUG "The char device is unregistered\n");
}


module_init( inform_init );
module_exit( inform_exit );

MODULE_AUTHOR("Throughc");
MODULE_DESCRIPTION("GRANDPRIX accelerometer driver for SMB380");
MODULE_LICENSE("GPL");
