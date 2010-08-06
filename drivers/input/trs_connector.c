/* drivers/input/trs_connector.c 
 *
 * headset driver for jupitor by manoj
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/switch.h>
#include <linux/workqueue.h>



static struct workqueue_struct* s5p_trs_workqueue;
static void s5p_trs_detection_work(struct work_struct* work);
static DECLARE_WORK(s5p_trs_work,s5p_trs_detection_work);

static struct switch_dev sdev;

static int irq_num;
static int state = 0;


//mkh: work queue to send uevent to user space
static void s5p_trs_detection_work(struct work_struct* work)
{

	if(gpio_get_value(S5PC11X_GPH0(6))){
     		
		printk("\n Headset inserted\n");
		if(!state)
       		switch_set_state(&sdev,1);//mkh : this crashes as it is trying to allocate memory. So moved it to bottom half.
      
	 	state = 1;
		set_irq_type(irq_num,IRQ_TYPE_EDGE_FALLING);
        }

        else{
       		printk("\n Headset removed\n");
		if(state)
        	switch_set_state(&sdev,0); //-do-
      	
		state = 0;
		set_irq_type(irq_num, IRQ_TYPE_EDGE_RISING);
        }


}


static irqreturn_t trs_connector_irq(int irq,struct platform_device* dev)
{
	queue_work(s5p_trs_workqueue, &s5p_trs_work);
	return IRQ_HANDLED;
}



static int trs_connector_probe(struct platform_device* pdev)
{

	int ret;

	s5p_trs_workqueue = create_workqueue("s5p_trs_connector");
	if(s5p_trs_workqueue == NULL){
	printk("\n ----------------headset connector work queue creation failed \n");
	return -ENODEV;
	}


	sdev.name = "h2w"; //mkh: android headsetobserver gives this name
	ret = switch_dev_register(&sdev);
	if(ret < 0){
	printk("\n !!!switch register failed \n");
	return -ENODEV;
	}

	irq_num = platform_get_irq(pdev,0);
	if(irq_num < 0)
	{
	printk("\n++++++HEADPHONE FAIL TO GET IRQ RESOURCE\n");
	return -ENODEV;
	}	
	

	ret = request_irq(irq_num,trs_connector_irq,0,"s5p_trs",pdev);
	if(ret){
	printk("\n TRS IRQ REQUEST FAIL \n");
	return -ENODEV;
	}

	queue_work(s5p_trs_workqueue, &s5p_trs_work); //mkh:initial status of headset
	printk("\n TRSCONNECTOR PROBED\n");
	return 0;

}


static int trs_connector_remove(struct platform_device* pdev)
{

	free_irq(irq_num,pdev);
	destroy_workqueue(s5p_trs_workqueue);
	return 0;
}


int trs_connector_suspend()
{
	//nothing to do here...?
	return 0;
}

int trs_connector_resume()
{
	queue_work(s5p_trs_workqueue, &s5p_trs_work);
	return 0;
}

static struct platform_driver s5p_trs_connector_driver = {

		.probe = trs_connector_probe,
		.remove = trs_connector_remove,
		.suspend = trs_connector_suspend,
		.resume = trs_connector_resume,
		.driver = {
		.name = "s5p_trs",
		.owner = THIS_MODULE,
		}



};


int __init s5p_trs_init(void)
{
	
	int ret;
	
	ret = platform_driver_register(&s5p_trs_connector_driver);
	
	if(ret)
	{
		printk("\n>>>>>>>>>>>>>>>>>>>>> trs plat-device registration failed>>>>>>>>>>>>>>>>>>>>>>>\n");
		return ret;
	}

		
	return ret;
}


static void __exit s5p_trs_exit(void)
{

platform_driver_unregister(&s5p_trs_connector_driver);

}







module_init(s5p_trs_init);
module_exit(s5p_trs_exit);


