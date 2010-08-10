#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>

#include <mach/gpio-jupiter.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 


#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/freezer.h>

//#define HAC_DEBUG
#define SUBJECT "HAC.c"

#define hacdebug_error(format,...)\
	printk ("["SUBJECT "(%d)] " format "\n", __LINE__, ## __VA_ARGS__);

#ifdef HAC_DEBUG
#define hacdebug(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define hacdebug(format,...)
#endif

#define HAC_OFF 0
#define HAC_ON 1
#define HAC_STATE 3



int hac_probe(void);
int hac_state(void);

static int HAC_open(struct inode *inode, struct file *file);
static int HAC_release(struct inode *inode, struct file *file);
static int HAC_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg); 

static int hacstate;


static int HAC_open(struct inode *inode, struct file *file)
{
	hacdebug("");
	return 0;
}
static int HAC_release(struct inode *inode, struct file *file)
{
	hacdebug("");
	return 0;
}
static int HAC_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long state;
	
	hacdebug("cmd=%d",cmd);
	switch(cmd)
	{
		case HAC_OFF:
			hacstate=HAC_OFF;
			printk("HAC OFF\n");
			break;
		case HAC_ON:
			hacstate=HAC_ON;
			printk("HAC ON\n");
			break;
		case HAC_STATE:
			state=hacstate;
			copy_to_user((void *)arg,&state,sizeof(state));
		default:
			break;
	}
	
	return 0;
}

int hac_state(void)
{
	return hacstate;
}


static struct file_operations HAC_fops = {	
	.owner = THIS_MODULE,	
	.open = HAC_open,	
	.ioctl = HAC_ioctl,	
	.release = HAC_release,
};

static struct miscdevice hac_miscdev = {	
	.minor = MISC_DYNAMIC_MINOR,	
	.name = "hac_miscdev",	
	.fops = &HAC_fops,
}; 

int hac_probe(void)
{
	int ret=0;
	hacdebug("");

	hacstate=HAC_OFF;

	ret = misc_register(&hac_miscdev);	
	if(ret)	
	{		
		printk("sensor misc device register failed\n");		
	}

	return ret;
}




