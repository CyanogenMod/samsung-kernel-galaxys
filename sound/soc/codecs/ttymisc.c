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

//#define TTYMISC_DEBUG
#define SUBJECT "ttymisc.c"

#define ttydebug_error(format,...)\
	printk ("["SUBJECT "(%d)] " format "\n", __LINE__, ## __VA_ARGS__);

#ifdef TTYMISC_DEBUG
#define ttydebug(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define ttydebug(format,...)
#endif

#define TTYMISC_OFF 0
#define TTYMISC_ON 1
#define TTYMISC_STATE 3



int ttymisc_probe(void);
//[ hdlnc_bp_ytkwon : 20100315
int ttymisc_state(void);
//] hdlnc_bp_ytkwon : 20100315

static int ttymisc_open(struct inode *inode, struct file *file);
static int ttymisc_release(struct inode *inode, struct file *file);
static int ttymisc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg); 

//[ hdlnc_bp_ytkwon : 20100315
static int ttystate;
//] hdlnc_bp_ytkwon : 20100315


static int ttymisc_open(struct inode *inode, struct file *file)
{
	ttydebug("");
	return 0;
}
static int ttymisc_release(struct inode *inode, struct file *file)
{
	ttydebug("");
	return 0;
}
static int ttymisc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long state;
	
	ttydebug("cmd=%d",cmd);
	switch(cmd)
	{
		case TTYMISC_OFF:
//[ hdlnc_bp_ytkwon : 20100315
			ttystate=TTYMISC_OFF;
//] hdlnc_bp_ytkwon : 20100315
			printk("TTY OFF\n");
			break;
		case TTYMISC_ON:
//[ hdlnc_bp_ytkwon : 20100315
			ttystate=TTYMISC_ON;
//] hdlnc_bp_ytkwon : 20100315
			printk("TTY ON\n");
			break;
		case TTYMISC_STATE:
			state=ttystate;
			copy_to_user((void *)arg,&state,sizeof(state));
		default:
			break;
	}
	
	return 0;
}

//[ hdlnc_bp_ytkwon : 20100315
int ttymisc_state(void)
{
	return ttystate;
}
//] hdlnc_bp_ytkwon : 20100315


static struct file_operations ttymisc_fops = {	
	.owner = THIS_MODULE,	
	.open = ttymisc_open,	
	.ioctl = ttymisc_ioctl,	
	.release = ttymisc_release,
};

static struct miscdevice tty_miscdev = {	
	.minor = MISC_DYNAMIC_MINOR,	
	.name = "tty_miscdev",	
	.fops = &ttymisc_fops,
}; 

int ttymisc_probe(void)
{
	int ret=0;
	ttydebug("");

	ttystate=TTYMISC_OFF;

	ret = misc_register(&tty_miscdev);	
	if(ret)	
	{		
		printk("sensor misc device register failed\n");		
	}

	return ret;
}




