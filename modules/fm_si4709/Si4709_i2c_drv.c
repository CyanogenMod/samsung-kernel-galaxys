
#include <linux/kernel.h>
#include <linux/i2c.h>

#include "Si4709_dev.h"
#include "Si4709_common.h"

/*extern functions*/
int Si4709_i2c_drv_init(void);
void Si4709_i2c_drv_exit(void);

/*static functions*/
static int Si4709_probe (struct i2c_client *);
static int Si4709_remove(struct i2c_client *);
static int Si4709_suspend(struct i2c_client *, pm_message_t mesg);
static int Si4709_resume(struct i2c_client *);
static int si4709_i2c_probe(struct i2c_adapter *, int, int);


/*I2C Setting*/
#define SI4709_I2C_ADDRESS      0x20

static unsigned short Si4709_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short Si4709_ignore[] = { I2C_CLIENT_END };
#if !(defined CONFIG_JUPITER_VER_B4) 
static unsigned short Si4709_i2c_probe[] = { 8, SI4709_I2C_ADDRESS >> 1, I2C_CLIENT_END };
#else
static unsigned short Si4709_i2c_probe[] = { 5, SI4709_I2C_ADDRESS >> 1, I2C_CLIENT_END };
#endif
 
//static struct i2c_client Si4709_i2c_client;
static struct i2c_driver Si4709_i2c_driver;

static struct i2c_client_address_data Si4709_addr_data = {
	.normal_i2c = Si4709_normal_i2c,
	.ignore     = Si4709_ignore,
	.probe      = Si4709_i2c_probe,
};

static int Si4709_i2c_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &Si4709_addr_data, si4709_i2c_probe);
}

static int Si4709_i2c_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	Si4709_remove(client);

	return 0;
}

static struct i2c_driver Si4709_i2c_driver =
{
    .driver = {
        .name = "Si4709_driver",
    },
	
	.attach_adapter = &Si4709_i2c_attach,
	.detach_client =  &Si4709_i2c_detach,

    .suspend = &Si4709_suspend,
    .resume = &Si4709_resume,
};

static struct i2c_client Si4709_i2c_client = {
    .name =   "Si4709",
	.driver = &Si4709_i2c_driver,
};

static int Si4709_probe (struct i2c_client *client)
{
    int ret = 0;

    debug("Si4709 i2c driver Si4709_probe called"); 

    if( strcmp(client->name, "Si4709") != 0 )
    {
        ret = -1;
        debug("Si4709_probe: device not supported");
    }
    else if( (ret = Si4709_dev_init(client)) < 0 )
    {
        debug("Si4709_dev_init failed");
    }

    return ret;
}

static int si4709_i2c_probe(struct i2c_adapter *adapter, int address, int kind)
{
	int err = 0;         

	printk("----- %s %d\n", __func__, __LINE__);

	Si4709_i2c_client.addr = address; 
	Si4709_i2c_client.adapter = adapter;
	Si4709_i2c_client.driver = &Si4709_i2c_driver;

	if ((err = i2c_attach_client(&Si4709_i2c_client)))
		return err;

	Si4709_probe(&Si4709_i2c_client);

	return 0;
}

static int Si4709_remove(struct i2c_client *client)
{
    int ret = 0;

    debug("Si4709 i2c driver Si4709_remove called"); 

    if( strcmp(client->name, "Si4709") != 0 )
    {
        ret = -1;
        debug("Si4709_remove: device not supported");
    }
    else if( (ret = Si4709_dev_exit()) < 0 )
    {
        debug("Si4709_dev_exit failed");
    }

    return ret;
}

static int Si4709_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret = 0;
	   
    debug("Si4709 i2c driver Si4709_suspend called"); 

    if( strcmp(client->name, "Si4709") != 0 )
    {
        ret = -1;
        debug("Si4709_suspend: device not supported");
    }
    else if( (ret = Si4709_dev_suspend()) < 0 )
    {
        debug("Si4709_dev_disable failed");
    }

    return 0;
}

static int Si4709_resume(struct i2c_client *client)
{
    int ret = 0;
	   
//    debug("Si4709 i2c driver Si4709_resume called"); 

    if( strcmp(client->name, "Si4709") != 0 )
    {
        ret = -1;
        debug("Si4709_resume: device not supported");
    }
    else if( (ret = Si4709_dev_resume()) < 0 )
    {
        debug("Si4709_dev_enable failed");
    }
 
    return 0;
}

int Si4709_i2c_drv_init(void)
{	
    int ret = 0;

    debug("Si4709 i2c driver Si4709_i2c_driver_init called"); 

    if ( (ret = i2c_add_driver(&Si4709_i2c_driver) < 0) ) 
    {
        error("Si4709 i2c_add_driver failed");
    }

    return ret;
}

void Si4709_i2c_drv_exit(void)
{
    debug("Si4709 i2c driver Si4709_i2c_driver_exit called"); 

    i2c_del_driver(&Si4709_i2c_driver);
}


