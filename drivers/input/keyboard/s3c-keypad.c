/* drivers/input/keyboard/s3c-keypad.c
 *
 * Driver core for Samsung SoC onboard UARTs.
 *
 * Kim Kyoungil, Copyright (c) 2006-2009 Samsung Electronics
 *      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/irq.h>

#include <linux/io.h>
#include <mach/hardware.h>
#include <asm/delay.h>
#include <asm/irq.h>

#include <plat/regs-gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-keypad.h>
#ifdef CONFIG_CPU_FREQ
#include <plat/s5pc11x-dvfs.h>
#endif
 
#include "s3c-keypad.h"

#ifdef CONFIG_KERNEL_DEBUG_SEC
#include <linux/kernel_sec_common.h>
#endif

//NAGSM_Android_SEL_Kernel_Aakash_20100319
struct class *s3ckeypad_class;
struct device *s3ckeypad_dev;


static int s3ckeypad_evt_enable_status = 1;
static ssize_t s3ckeypad_evt_status_show(struct device *dev, struct device_attribute *attr, char *sysfsbuf)
{	


	return sprintf(sysfsbuf, "%d\n", s3ckeypad_evt_enable_status);

}

static ssize_t s3ckeypad_evt_status_store(struct device *dev, struct device_attribute *attr,const char *sysfsbuf, size_t size)
{

	sscanf(sysfsbuf, "%d", &s3ckeypad_evt_enable_status);
	return size;
}

static DEVICE_ATTR(s3ckeypadevtcntrl, S_IRUGO | S_IWUSR, s3ckeypad_evt_status_show, s3ckeypad_evt_status_store);

//NAGSM_Android_SEL_Kernel_Aakash_20100319

#define USE_PERF_LEVEL_KEYPAD 1 
#undef S3C_KEYPAD_DEBUG 
//#define S3C_KEYPAD_DEBUG 

#ifdef S3C_KEYPAD_DEBUG
#define DPRINTK(x...) printk("S3C-Keypad " x)
#else
#define DPRINTK(x...)		/* !!!! */
#endif

#define DEVICE_NAME "s3c-keypad"

#define TRUE 1
#define FALSE 0
#define	SUBJECT	"s3c_keypad.c"
#define P(format,...)\
    printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#define FI \
    printk ("[ "SUBJECT " (%s,%d) ] " "%s - IN" "\n", __func__, __LINE__, __func__);
#define FO \
    printk ("[ "SUBJECT " (%s,%d) ] " "%s - OUT" "\n", __func__, __LINE__, __func__);

static struct timer_list keypad_timer;
static int is_timer_on = FALSE;
static struct clk *keypad_clock;


static u32 keymask[KEYPAD_COLUMNS];
static u32 prevmask[KEYPAD_COLUMNS];

#if defined (CONFIG_MACH_S5PC110_P1P2)
static int keypad_scan(void)
{

	u32 col,rval,gpio;

	DPRINTK("H3C %x J2C %x, J3c %x J4c%x \n",readl(S5PC11X_GPH3CON),readl(S5PC11X_GPJ2CON),
		readl(S5PC11X_GPJ3CON), readl(S5PC11X_GPJ4CON));
	DPRINTK("keypad_scan() is called\n");

	DPRINTK("row val = %x",readl(key_base + S3C_KEYIFROW));

	for (gpio = S5PC11X_GPJ2(7); gpio <= S5PC11X_GPJ4(4); gpio++)
		s3c_gpio_setpin(gpio, 1);
		
	for (col=0,gpio = S5PC11X_GPJ2(7); col < KEYPAD_COLUMNS; col++,gpio++) {

		if(s3c_gpio_setpin(gpio, 0) < 0)
			s3c_gpio_setpin(++gpio, 0);

		//udelay(KEYPAD_DELAY);
		udelay(100);

		//rval = ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1) ;
		rval = ~(readl(S5PC11X_GPH3DAT)) & ((1<<KEYPAD_ROWS)-1) ;
		

		keymask[col] = rval; 

		s3c_gpio_setpin(gpio,1);
	}

	for (gpio = S5PC11X_GPJ2(7); gpio <= S5PC11X_GPJ4(4); gpio++)
		s3c_gpio_setpin(gpio, 0);

	return 0;
}
#else

static int keypad_scan(void)
{

	u32 col,cval,rval,gpio;

	DPRINTK("H3C %x H2C %x \n",readl(S5PC11X_GPH3CON),readl(S5PC11X_GPH2CON));
	DPRINTK("keypad_scan() is called\n");

	DPRINTK("row val = %x",readl(key_base + S3C_KEYIFROW));

	for (col=0; col < KEYPAD_COLUMNS; col++) {

		cval = KEYCOL_DMASK & ~((1 << col) | (1 << col+ 8)); // clear that column number and 

		writel(cval, key_base+S3C_KEYIFCOL);             // make that Normal output.
								 // others shuld be High-Z output.
		udelay(KEYPAD_DELAY);

		rval = ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1) ;
		keymask[col] = rval; 
	}

	writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);

	return 0;
}

#endif

#ifdef CONFIG_KERNEL_DEBUG_SEC	
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)|| defined(CONFIG_T959_VER_B5)
#define	DUMP_SEQUENCE_COUNT	6
#if defined(CONFIG_T959_VER_B5)
#define PRESSED_UP	4
#define PRESSED_DOWN	5
#else
#define PRESSED_UP	5
#define PRESSED_DOWN	4
#endif

int forced_dump_sequence[DUMP_SEQUENCE_COUNT] = 
		{PRESSED_UP, PRESSED_UP, PRESSED_DOWN, PRESSED_UP, PRESSED_DOWN, PRESSED_DOWN};	// dump sequence
int dump_mode_key_sequence[DUMP_SEQUENCE_COUNT] = {0};	// pressed key sequence
int dump_mode_key_count = 0;	// count for dump_mode_key_sequence array
bool is_first_key_down = 0;		// will check dump mode when down key is pressed twice
int dump_compare_count = 0;		// count for forced_dump_sequence array
int is_dump_sequence = 0;		// if this is 6, means pressed key sequence is same as dump sequence
unsigned long long dump_mode_key_pressed_prev=0,dump_mode_key_pressed;
bool forced_dump_time_limit=0;
#endif
#endif

extern unsigned int HWREV;

static void keypad_timer_handler(unsigned long data)
{
	u32 press_mask;
	u32 release_mask;
	u32 restart_timer = 0;
	int i,col;
	struct s3c_keypad *pdata = (struct s3c_keypad *)data;
	struct input_dev *dev = pdata->dev;

	keypad_scan();


	for(col=0; col < KEYPAD_COLUMNS; col++) {
		press_mask = ((keymask[col] ^ prevmask[col]) & keymask[col]); 
		release_mask = ((keymask[col] ^ prevmask[col]) & prevmask[col]); 

#ifdef CONFIG_CPU_FREQ
#if USE_PERF_LEVEL_KEYPAD
		if (press_mask || release_mask)
			set_dvfs_perf_level();
#endif
#endif
		i = col * KEYPAD_ROWS;

		while (press_mask) {
			if (press_mask & 1) {
				if(s3ckeypad_evt_enable_status){		//NAGSM_Android_SEL_Kernel_Aakash_20100319

					#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_KEPLER_VER_B4) || defined(CONFIG_T959_VER_B5)
						if((i != 1) && (i !=2)){
							input_report_key(dev,pdata->keycodes[i],1);
							printk("\nkey Pressed  : key %d map %d\n",i, pdata->keycodes[i]);
						}
					#else					
				input_report_key(dev,pdata->keycodes[i],1);
					DPRINTK("\nkey Pressed  : key %d map %d\n",i, pdata->keycodes[i]);
					#endif
				}
			}
			press_mask >>= 1;
			i++;
		}

		i = col * KEYPAD_ROWS;

		while (release_mask) {
			if (release_mask & 1) {
				if(s3ckeypad_evt_enable_status){		//NAGSM_Android_SEL_Kernel_Aakash_20100319
					#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_KEPLER_VER_B4) || defined(CONFIG_T959_VER_B5)
						if((i != 1) && (i !=2)){
							input_report_key(dev,pdata->keycodes[i],0);
							printk("\nkey Released : %d  map %d\n",i,pdata->keycodes[i]);
						}
					#else					
				input_report_key(dev,pdata->keycodes[i],0);
					DPRINTK("\nkey Released : %d  map %d\n",i,pdata->keycodes[i]);
					#endif
				}
				
#ifdef CONFIG_KERNEL_DEBUG_SEC
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD) || defined(CONFIG_T959_VER_B5)

	#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_KEPLER_VER_B4) || defined(CONFIG_T959_VER_B5)
		if((i != 1) && (i !=2)){
			dump_mode_key_sequence[dump_mode_key_count%DUMP_SEQUENCE_COUNT] = i;
			dump_mode_key_count++;
		}
	#endif
	#if defined(CONFIG_KEPLER_VER_B0)
	dump_mode_key_sequence[dump_mode_key_count%DUMP_SEQUENCE_COUNT] = i;
	dump_mode_key_count++;
	#endif

       {
            // check time inverval between force upload key sequence input
		int this_cpu;
		this_cpu = smp_processor_id();
		unsigned long nanosec_rem;
						
		dump_mode_key_pressed = cpu_clock(this_cpu);
		nanosec_rem = do_div(dump_mode_key_pressed, 1000000000);

		if ((dump_mode_key_pressed-dump_mode_key_pressed_prev>=2) && dump_mode_key_pressed_prev!=0)
                {
			forced_dump_time_limit=1;
              printk("force upload long time : %ul\n ",
			dump_mode_key_pressed-dump_mode_key_pressed_prev);
                 }
	
		dump_mode_key_pressed_prev=dump_mode_key_pressed;
						
	}

	if(i == PRESSED_DOWN){	
		if(is_first_key_down == 1)
		{	// when down key is pressed twice, will check whether pressed keys are same as dump seqence 
			is_dump_sequence = 0;

			//for(dump_compare_count=0; dump_compare_count<DUMP_SEQUENCE_COUNT; dump_compare_count++)
				//printk("dump_mode_key_sequence[%d] : %d\n",(dump_mode_key_count + dump_compare_count)%DUMP_SEQUENCE_COUNT, dump_mode_key_sequence[(dump_mode_key_count + dump_compare_count)%DUMP_SEQUENCE_COUNT]);
			
			for(dump_compare_count=0; dump_compare_count<DUMP_SEQUENCE_COUNT; dump_compare_count++)
			{			
				//printk("forced_dump_sequence[%d] : %d, dump_mode_key_sequence[%d] : %d\n",dump_compare_count, forced_dump_sequence[dump_compare_count],(dump_mode_key_count + dump_compare_count)%DUMP_SEQUENCE_COUNT, dump_mode_key_sequence[(dump_mode_key_count + dump_compare_count)%DUMP_SEQUENCE_COUNT]);
				if(forced_dump_sequence[dump_compare_count] == dump_mode_key_sequence[((dump_mode_key_count + dump_compare_count) % DUMP_SEQUENCE_COUNT)]){
					is_dump_sequence++;
					continue;
				}else
				{	
					//printk("This is not dump sequence.\n");
					is_dump_sequence = 0;
					break;
				}
			}
			
			if(is_dump_sequence == DUMP_SEQUENCE_COUNT)
			{		
                  if (forced_dump_time_limit==1)
                                {
                                        // case of long time between force upload key sequence
                       forced_dump_time_limit=0;
                       is_dump_sequence=0;
                       dump_mode_key_pressed_prev=0;
                       printk("Dump Mode Fail\n");
                                }
                  else	
                                {	

						if( (KERNEL_SEC_DEBUG_LEVEL_MID == kernel_sec_get_debug_level()) || 
							  (KERNEL_SEC_DEBUG_LEVEL_HIGH == kernel_sec_get_debug_level()) )	
							{
								
					printk("Now going to Dump Mode\n");
					if (kernel_sec_viraddr_wdt_reset_reg)
					{
						kernel_sec_set_cp_upload();
						kernel_sec_save_final_context(); // Save theh final context.
						kernel_sec_set_upload_cause(UPLOAD_CAUSE_FORCED_UPLOAD);
						kernel_sec_hw_reset(FALSE); 	 // Reboot.
					}
							}
					
                                }
			}
		}else{
			is_first_key_down = 1;
		}
	}else{
		#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_KEPLER_VER_B4) || defined(CONFIG_T959_VER_B5)
			if((i != 1) && (i !=2)){
				is_first_key_down = 0;
			}
		#endif
		#if defined(CONFIG_KEPLER_VER_B0)
		is_first_key_down = 0;
		#endif			
	}				
#endif
#endif
			}
			release_mask >>= 1;
			i++;
		}
		prevmask[col] = keymask[col];

		restart_timer |= keymask[col];
	}


	if (restart_timer) {
		mod_timer(&keypad_timer,jiffies + HZ/10);
	} else {
		writel(KEYIFCON_INIT, key_base+S3C_KEYIFCON);
		is_timer_on = FALSE;
	}

}


static irqreturn_t s3c_keypad_isr(int irq, void *dev_id)
{

	/* disable keypad interrupt and schedule for keypad timer handler */
	writel(readl(key_base+S3C_KEYIFCON) & ~(INT_F_EN|INT_R_EN), key_base+S3C_KEYIFCON);

	keypad_timer.expires = jiffies;
	if ( is_timer_on == FALSE) {
		add_timer(&keypad_timer);
		is_timer_on = TRUE;
	} else {
		mod_timer(&keypad_timer,keypad_timer.expires);
	}
	/*Clear the keypad interrupt status*/
	writel(KEYIFSTSCLR_CLEAR, key_base+S3C_KEYIFSTSCLR);
	
	return IRQ_HANDLED;
}



static irqreturn_t s3c_keygpio_isr(int irq, void *dev_id)
{
	unsigned int key_status;
	struct s3c_keypad *pdata = (struct s3c_keypad *)dev_id;
	struct input_dev *dev = pdata->dev;

	
	key_status = (readl(S5PC11X_GPH2DAT)) & (1 << 6);
	
	if(s3ckeypad_evt_enable_status)		//NAGSM_Android_SEL_Kernel_Aakash_20100319
	{
	if(key_status)
		input_report_key(dev,26,0);
	else
		input_report_key(dev,26,1);
	}									//NAGSM_Android_SEL_Kernel_Aakash_20100319

	printk(KERN_DEBUG "s3c_keygpio_isr key_status =%d,\n", key_status);
	//printk(KERN_DEBUG "eint 2 irq status s3c_keygpio_isr PEND= %d, MASK = %d\n", readl(S5PC11X_EINTPEND(2)), readl(S5PC11X_EINTMASK(2)));

        return IRQ_HANDLED;
}

static irqreturn_t s3c_keygpio_vol_up_isr(int irq, void *dev_id)
{
	unsigned int key_status;
	struct s3c_keypad *pdata = (struct s3c_keypad *)dev_id;
	struct input_dev *dev = pdata->dev;

	//we cannot check HWREV 0xb and 0xc, we check 2 hw key
	key_status = (readl(S5PC11X_GPH3DAT)) & ((1 << 3));
	
	if(s3ckeypad_evt_enable_status)		//NAGSM_Android_SEL_Kernel_Aakash_20100319
	{
	if(key_status == ((1 << 3)))
		input_report_key(dev,42,0);
	else
		input_report_key(dev,42,1);
	}									//NAGSM_Android_SEL_Kernel_Aakash_20100319
       printk(KERN_DEBUG "s3c_keygpio_vol_up_isr key_status =%d,\n", key_status);

        return IRQ_HANDLED;
}

#if 0
static irqreturn_t s3c_keygpio_vol_down_isr(int irq, void *dev_id)
{
	unsigned int key_status;
	struct s3c_keypad *pdata = (struct s3c_keypad *)dev_id;
	struct input_dev *dev = pdata->dev;

	
	key_status = (readl(S5PC11X_GPH3DAT)) & (1 << 1);
	
	if(key_status)
		input_report_key(dev,58,0);
	else
		input_report_key(dev,58,1);

        return IRQ_HANDLED;
}
#endif

static irqreturn_t s3c_keygpio_ok_isr(int irq, void *dev_id)
{
	unsigned int key_status;
	struct s3c_keypad *pdata = (struct s3c_keypad *)dev_id;
	struct input_dev *dev = pdata->dev;

	//we cannot check HWREV 0xb and 0xc, we check 2 hw key
	key_status = (readl(S5PC11X_GPH3DAT)) & ((1 << 5));
	
	if(s3ckeypad_evt_enable_status)		//NAGSM_Android_SEL_Kernel_Aakash_20100319
	{	
	if(key_status == ((1 << 5)))
#ifdef CONFIG_S5PC110_FLEMING_BOARD
		input_report_key(dev,28,0);
#else
		input_report_key(dev,50,0);
#endif
	else
#ifdef CONFIG_S5PC110_FLEMING_BOARD
		input_report_key(dev,28,1);
#else
		input_report_key(dev,50,1);
#endif 
	}									//NAGSM_Android_SEL_Kernel_Aakash_20100319
        printk(KERN_DEBUG "s3c_keygpio_ok_isr key_status =%d,\n", key_status);

        return IRQ_HANDLED;
}
static int s3c_keygpio_isr_setup(void *pdev)
{
	int ret;

#if defined(CONFIG_BEHOLD3_VER_B0)
	//volume up
	s3c_gpio_setpull(S5PC11X_GPH3(3), S3C_GPIO_PULL_UP);
	set_irq_type(IRQ_EINT(27), IRQ_TYPE_EDGE_BOTH);
		ret = request_irq(IRQ_EINT(27), s3c_keygpio_vol_up_isr, IRQF_SAMPLE_RANDOM,
				"key vol up", (void *) pdev);
		if (ret) {
				printk("request_irq failed (IRQ_KEYPAD (key vol up)) !!!\n");
				ret = -EIO;
		  return ret;
		}
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
#elif defined(CONFIG_T959_VER_B0) || defined(CONFIG_T959_VER_B5)

		//ok key
		s3c_gpio_setpull(S5PC11X_GPH3(5), S3C_GPIO_PULL_UP);
		set_irq_type(IRQ_EINT(29), IRQ_TYPE_EDGE_BOTH);
	        ret = request_irq(IRQ_EINT(29), s3c_keygpio_ok_isr, IRQF_SAMPLE_RANDOM,
	                "key ok", (void *) pdev);
	        if (ret) {
	                printk("request_irq failed (IRQ_KEYPAD (key ok)) !!!\n");
	                ret = -EIO;
			  return ret;
	  }
	        
#elif defined(CONFIG_S5PC110_KEPLER_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)
#elif defined(CONFIG_S5PC110_FLEMING_BOARD)
  //ok key
  s3c_gpio_setpull(S5PC11X_GPH3(5), S3C_GPIO_PULL_UP);
  set_irq_type(IRQ_EINT(29), IRQ_TYPE_EDGE_BOTH);
	  ret = request_irq(IRQ_EINT(29), s3c_keygpio_ok_isr, IRQF_SAMPLE_RANDOM,
			  "key ok", (void *) pdev);
	  if (ret) {
			  printk("request_irq failed (IRQ_KEYPAD (key ok)) !!!\n");
			  ret = -EIO;
		return ret;
}

#else

	if(HWREV >= 0xB)
	{
		#if 0
		//volume down
		s3c_gpio_setpull(S5PC11X_GPH3(1), S3C_GPIO_PULL_UP);
		set_irq_type(IRQ_EINT(25), IRQ_TYPE_EDGE_BOTH);
	        ret = request_irq(IRQ_EINT(25), s3c_keygpio_vol_down_isr, IRQF_SAMPLE_RANDOM,
	                "key vol down", (void *) pdev);
	        if (ret) {
	                printk("request_irq failed (IRQ_KEYPAD (key vol down)) !!!\n");
	                ret = -EIO;
			  return ret;
	        }
		
		//volume up
		s3c_gpio_setpull(S5PC11X_GPH3(2), S3C_GPIO_PULL_UP);
		set_irq_type(IRQ_EINT(26), IRQ_TYPE_EDGE_BOTH);
	        ret = request_irq(IRQ_EINT(26), s3c_keygpio_vol_up_isr, IRQF_SAMPLE_RANDOM,
	                "key vol up", (void *) pdev);
	        if (ret) {
	                printk("request_irq failed (IRQ_KEYPAD (key vol up)) !!!\n");
	                ret = -EIO;
			  return ret;
	        }
		#endif
		
		//volume up
		s3c_gpio_setpull(S5PC11X_GPH3(3), S3C_GPIO_PULL_UP);
		set_irq_type(IRQ_EINT(27), IRQ_TYPE_EDGE_BOTH);
	        ret = request_irq(IRQ_EINT(27), s3c_keygpio_vol_up_isr, IRQF_SAMPLE_RANDOM,
	                "key vol up", (void *) pdev);
	        if (ret) {
	                printk("request_irq failed (IRQ_KEYPAD (key vol up)) !!!\n");
	                ret = -EIO;
			  return ret;
	        }

		//ok key
		s3c_gpio_setpull(S5PC11X_GPH3(5), S3C_GPIO_PULL_UP);
		set_irq_type(IRQ_EINT(29), IRQ_TYPE_EDGE_BOTH);
	        ret = request_irq(IRQ_EINT(29), s3c_keygpio_ok_isr, IRQF_SAMPLE_RANDOM,
	                "key ok", (void *) pdev);
	        if (ret) {
	                printk("request_irq failed (IRQ_KEYPAD (key ok)) !!!\n");
	                ret = -EIO;
			  return ret;
	        }

			
		//ok key
		#if 0
		s3c_gpio_setpull(S5PC11X_GPH3(0), S3C_GPIO_PULL_UP);
		set_irq_type(IRQ_EINT(24), IRQ_TYPE_EDGE_BOTH);
	        ret = request_irq(IRQ_EINT(24), s3c_keygpio_ok_isr, IRQF_SAMPLE_RANDOM,
	                "key ok", (void *) pdev);
	        if (ret) {
	                printk("request_irq failed (IRQ_KEYPAD (key ok)) !!!\n");
	                ret = -EIO;
			  return ret;
	        }
		 #endif
	}
#endif

	//PWR key
	s3c_gpio_setpull(S5PC11X_GPH2(6), S3C_GPIO_PULL_UP);

	set_irq_type(IRQ_EINT(22), IRQ_TYPE_EDGE_BOTH);
  
        ret = request_irq(IRQ_EINT(22), s3c_keygpio_isr, IRQF_SAMPLE_RANDOM,
                "key gpio", (void *) pdev);
        if (ret) {
                printk("request_irq failed (IRQ_KEYPAD (gpio)) !!!\n");
                ret = -EIO;
        }

	return ret;

}


static ssize_t keyshort_test(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;
	
#if defined (CONFIG_S5PC110_KEPLER_BOARD) || defined (CONFIG_T959_VER_B5)
	if(!gpio_get_value(GPIO_KBR0)||!gpio_get_value(GPIO_KBR1) || !gpio_get_value(GPIO_KBR2) || !gpio_get_value(GPIO_nPOWER))
#elif defined CONFIG_S5PC110_T959_BOARD && !defined (CONFIG_T959_VER_B5)
	if(!gpio_get_value(GPIO_KBR0) || !gpio_get_value(GPIO_KBR1) || !gpio_get_value(GPIO_KBR2) || !gpio_get_value(GPIO_nPOWER)  || !gpio_get_value(S5PC11X_GPH3(5)))
#else
       if(!gpio_get_value(GPIO_KBR0) || !gpio_get_value(GPIO_KBR1) || !gpio_get_value(GPIO_KBR2) || !gpio_get_value(GPIO_nPOWER)  || !gpio_get_value(S5PC11X_GPH3(5)))
#endif
	{
		count = sprintf(buf,"PRESS\n");
              printk("keyshort_test: PRESS\n");
	}
	else
	{
		count = sprintf(buf,"RELEASE\n");
              printk("keyshort_test: RELEASE\n");
	}	

	return count;
}
static DEVICE_ATTR(key_pressed, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, keyshort_test, NULL);

static int __init s3c_keypad_probe(struct platform_device *pdev)
{
	struct resource *res, *keypad_mem, *keypad_irq;
	struct input_dev *input_dev;
	struct s3c_keypad *s3c_keypad;
	int ret, size;
	int key, code;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev,"no memory resource specified\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;

	keypad_mem = request_mem_region(res->start, size, pdev->name);
	if (keypad_mem == NULL) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req;
	}

	key_base = ioremap(res->start, size);
	if (key_base == NULL) {
		printk(KERN_ERR "Failed to remap register block\n");
		ret = -ENOMEM;
		goto err_map;
	}

	keypad_clock = clk_get(&pdev->dev, "keypad");
	if (IS_ERR(keypad_clock)) {
		dev_err(&pdev->dev, "failed to find keypad clock source\n");
		ret = PTR_ERR(keypad_clock);
		goto err_clk;
	}

	clk_enable(keypad_clock);
	
	s3c_keypad = kzalloc(sizeof(struct s3c_keypad), GFP_KERNEL);
	input_dev = input_allocate_device();

	if (!s3c_keypad || !input_dev) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	platform_set_drvdata(pdev, s3c_keypad);
	s3c_keypad->dev = input_dev;
	
	writel(KEYIFCON_INIT, key_base+S3C_KEYIFCON);
	writel(KEYIFFC_DIV, key_base+S3C_KEYIFFC);

	/* Set GPIO Port for keypad mode and pull-up disable*/
	s3c_setup_keypad_cfg_gpio(KEYPAD_ROWS, KEYPAD_COLUMNS);

	writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);

	/* create and register the input driver */
	set_bit(EV_KEY, input_dev->evbit);
	/*Commenting the generation of repeat events*/
	//set_bit(EV_REP, input_dev->evbit);
	s3c_keypad->nr_rows = KEYPAD_ROWS;
	s3c_keypad->no_cols = KEYPAD_COLUMNS;
	s3c_keypad->total_keys = MAX_KEYPAD_NR;

	for(key = 0; key < s3c_keypad->total_keys; key++){
		code = s3c_keypad->keycodes[key] = keypad_keycode[key];
		if(code<=0)
			continue;
		set_bit(code & KEY_MAX, input_dev->keybit);
	}

      #if defined(CONFIG_ARIES_VER_B3)  
      set_bit(26 & KEY_MAX, input_dev->keybit);
      #endif
      
	input_dev->name = DEVICE_NAME;
	input_dev->phys = "s3c-keypad/input0";
	
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0001;

	input_dev->keycode = keypad_keycode;

	ret = input_register_device(input_dev);
	if (ret) {
		printk("Unable to register s3c-keypad input device!!!\n");
		goto out;
	}

	/* Scan timer init */
	init_timer(&keypad_timer);
	keypad_timer.function = keypad_timer_handler;
	keypad_timer.data = (unsigned long)s3c_keypad;

	/* For IRQ_KEYPAD */
	keypad_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (keypad_irq == NULL) {
		dev_err(&pdev->dev, "no irq resource specified\n");
		ret = -ENOENT;
		goto err_irq;
	}
	ret = request_irq(keypad_irq->start, s3c_keypad_isr, IRQF_SAMPLE_RANDOM,
		DEVICE_NAME, (void *) pdev);
	if (ret) {
		printk("request_irq failed (IRQ_KEYPAD) !!!\n");
		ret = -EIO;
		goto err_irq;
	}

	keypad_timer.expires = jiffies + (HZ/10);

	if (is_timer_on == FALSE) {
		add_timer(&keypad_timer);
		is_timer_on = TRUE;
	} else {
		mod_timer(&keypad_timer,keypad_timer.expires);
	}

	s3c_keygpio_isr_setup((void *)s3c_keypad);
	printk( DEVICE_NAME " Initialized\n");
	
	if (device_create_file(&pdev->dev, &dev_attr_key_pressed) < 0)
	{
		printk("%s s3c_keypad_probe\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_key_pressed.attr.name);
	}
	
	return 0;

out:
	free_irq(keypad_irq->start, input_dev);
	free_irq(keypad_irq->end, input_dev);

err_irq:
	input_free_device(input_dev);
	kfree(s3c_keypad);
	
err_alloc:
	clk_disable(keypad_clock);
	clk_put(keypad_clock);

err_clk:
	iounmap(key_base);

err_map:
	release_resource(keypad_mem);
	kfree(keypad_mem);

err_req:
	return ret;
}

static int s3c_keypad_remove(struct platform_device *pdev)
{
	struct input_dev *input_dev = platform_get_drvdata(pdev);
	writel(KEYIFCON_CLEAR, key_base+S3C_KEYIFCON);

	if(keypad_clock) {
		clk_disable(keypad_clock);
		clk_put(keypad_clock);
		keypad_clock = NULL;
	}

	input_unregister_device(input_dev);
	iounmap(key_base);
	kfree(pdev->dev.platform_data);
	free_irq(IRQ_KEYPAD, (void *) pdev);

	del_timer(&keypad_timer);	
	printk(DEVICE_NAME " Removed.\n");
	return 0;
}

#ifdef CONFIG_PM
#include <plat/pm.h>

static struct sleep_save s3c_keypad_save[] = {
	SAVE_ITEM(KEYPAD_ROW_GPIOCON),
	SAVE_ITEM(KEYPAD_COL_GPIOCON),
	SAVE_ITEM(KEYPAD_ROW_GPIOPUD),
	SAVE_ITEM(KEYPAD_COL_GPIOPUD),
};

static unsigned int keyifcon, keyiffc;
static int s3c_keypad_suspend(struct platform_device *dev, pm_message_t state)
{
	keyifcon = readl(key_base+S3C_KEYIFCON);
	keyiffc = readl(key_base+S3C_KEYIFFC);

	s5pc11x_pm_do_save(s3c_keypad_save, ARRAY_SIZE(s3c_keypad_save));
	
	//writel(~(0xfffffff), KEYPAD_ROW_GPIOCON);
	//writel(~(0xfffffff), KEYPAD_COL_GPIOCON);

	disable_irq(IRQ_KEYPAD);

	clk_disable(keypad_clock);

	return 0;
}


static int s3c_keypad_resume(struct platform_device *dev)
{
	struct s3c_keypad          *s3c_keypad = (struct s3c_keypad *) platform_get_drvdata(dev);
      struct input_dev           *iDev = s3c_keypad->dev;
	unsigned int key_temp_data=0;
	
	printk(KERN_DEBUG "++++ %s\n", __FUNCTION__ );

	clk_enable(keypad_clock);

	writel(KEYIFCON_INIT, key_base+S3C_KEYIFCON);
	writel(keyiffc, key_base+S3C_KEYIFFC);
	writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);

#if 0
	key_temp_data = readl(key_base+S3C_KEYIFROW) & 0x01;
	if (!key_temp_data){
		input_report_key(iDev, 50, 1);
		printk("key data is %d \n", key_temp_data);		
		input_report_key(iDev, 50, 0);
		}
	else {
		/*send some event to android to start the full resume*/
		input_report_key(iDev, KEYCODE_UNKNOWN, 1);//ENDCALL up event
		udelay(5);
		input_report_key(iDev, KEYCODE_UNKNOWN, 0);//ENDCALL down event
		}

	//printk("H3C %x H2C %x \n",readl(S5PC11X_GPH3CON),readl(S5PC11X_GPH2CON));
#endif
	s5pc11x_pm_do_restore(s3c_keypad_save, ARRAY_SIZE(s3c_keypad_save));

	enable_irq(IRQ_KEYPAD);
	printk(KERN_DEBUG "---- %s\n", __FUNCTION__ );
	return 0;
}
#else
#define s3c_keypad_suspend NULL
#define s3c_keypad_resume  NULL
#endif /* CONFIG_PM */

static struct platform_driver s3c_keypad_driver = {
	.probe		= s3c_keypad_probe,
	.remove		= s3c_keypad_remove,
	.suspend	= s3c_keypad_suspend,
	.resume		= s3c_keypad_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-keypad",
	},
};

static int __init s3c_keypad_init(void)
{
	int ret;

//NAGSM_Android_SEL_Kernel_Aakash_20100319
	s3ckeypad_class= class_create(THIS_MODULE, "s3ckeypadevtcntrl");
	if (IS_ERR(s3ckeypad_class))
		pr_err("Failed to create class(s3ckeypadevtcntrl)!\n");

	s3ckeypad_dev= device_create(s3ckeypad_class, NULL, 0, NULL, "s3ckeypad_control");
	if (IS_ERR(s3ckeypad_dev))
		pr_err("Failed to create device(s3ckeypad_control)!\n");
	if (device_create_file(s3ckeypad_dev, &dev_attr_s3ckeypadevtcntrl) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_s3ckeypadevtcntrl.attr.name);
//NAGSM_Android_SEL_Kernel_Aakash_20100319


	ret = platform_driver_register(&s3c_keypad_driver);
	
	if(!ret)
	   printk(KERN_INFO "S3C Keypad Driver\n");

	return ret;
}

static void __exit s3c_keypad_exit(void)
{
	platform_driver_unregister(&s3c_keypad_driver);
}

module_init(s3c_keypad_init);
module_exit(s3c_keypad_exit);

MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("KeyPad interface for Samsung S3C");
