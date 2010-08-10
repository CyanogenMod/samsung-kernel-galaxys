/* Slave address */
#define MAX17040_SLAVE_ADDR	0x6D

/* Register address */
#define VCELL0_REG			0x02
#define VCELL1_REG			0x03
#define SOC0_REG			0x04
#define SOC1_REG			0x05
#define MODE0_REG			0x06
#define MODE1_REG			0x07
#define RCOMP0_REG			0x0C
#define RCOMP1_REG			0x0D
#define CMD0_REG			0xFE
#define CMD1_REG			0xFF

static struct i2c_driver fg_i2c_driver;
static struct i2c_client *fg_i2c_client = NULL;

static unsigned short fg_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short fg_ignore[] = { I2C_CLIENT_END };
static unsigned short fg_probe[] = { 9, (MAX17040_SLAVE_ADDR >> 1), I2C_CLIENT_END };


static struct i2c_client_address_data fg_addr_data = {
	.normal_i2c	= fg_normal_i2c,
	.ignore		= fg_ignore,
	.probe		= fg_probe,
};

static int is_reset_soc = 0;
// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
static int rcomp_status;
static int ce_for_fuelgauge = 0;

static int fg_i2c_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

static int fg_i2c_write(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[3];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = *data;
	buf[2] = *(data + 1);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 3;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}

unsigned int fg_read_vcell(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 vcell = 0;

	if (fg_i2c_read(client, VCELL0_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read VCELL0\n", __func__);
		return -1;
	}
	if (fg_i2c_read(client, VCELL1_REG, &data[1]) < 0) {
		pr_err("%s: Failed to read VCELL1\n", __func__);
		return -1;
	}
	vcell = ((((data[0] << 4) & 0xFF0) | ((data[1] >> 4) & 0xF)) * 125)/100;
	//pr_info("%s: VCELL=%d\n", __func__, vcell);
	return vcell;
}

#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)
unsigned int fg_read_soc(void)
{
	 struct i2c_client *client = fg_i2c_client;
	 u8 data[2];
	 int FGPureSOC = 0;
	 int FGAdjustSOC = 0;
	 int FGSOC = 0;

	 if(fg_i2c_client==NULL)
		  return -1;
	 if (fg_i2c_read(client, SOC0_REG, &data[0]) < 0) {
		  pr_err("%s: Failed to read SOC0\n", __func__);
		  return -1;
	 }
	 if (fg_i2c_read(client, SOC1_REG, &data[1]) < 0) {
		  pr_err("%s: Failed to read SOC1\n", __func__);
		  return -1;
	 }
 
	 // calculating soc
	 FGPureSOC = data[0]*100+((data[1]*100)/256);
	 	 
	 if(ce_for_fuelgauge){
		 FGAdjustSOC = ((FGPureSOC - 130)*10000)/9720; // (FGPureSOC-EMPTY(1.2))/(FULL-EMPTY(?))*100
	 }
	 else{
		 FGAdjustSOC = ((FGPureSOC - 130)*10000)/9430; // (FGPureSOC-EMPTY(1.2))/(FULL-EMPTY(?))*100
	 }

//	 printk("\n[FUEL] PSOC = %d, ASOC = %d\n", FGPureSOC, FGAdjustSOC);
	 
	 // rounding off and Changing to percentage.
	 FGSOC=FGAdjustSOC/100;

	if((FGSOC==4)&&FGAdjustSOC%100 >= 80){
		FGSOC+=1;
	 	}
	 if((FGSOC== 0)&&(FGAdjustSOC>0)){
	       FGSOC = 1;  
	 	}
	 if(FGAdjustSOC <= 0){
	       FGSOC = 0;  	 	
	 	}
	 if(FGSOC>=100)
	 {
		  FGSOC=100;
	 }
	 
 return FGSOC;
 
}

#else

unsigned int fg_read_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	unsigned int FGPureSOC = 0;
	unsigned int FGAdjustSOC = 0;
	unsigned int FGSOC = 0;

	if(fg_i2c_client==NULL)
		return -1;

	if (fg_i2c_read(client, SOC0_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read SOC0\n", __func__);
		return -1;
	}
	if (fg_i2c_read(client, SOC1_REG, &data[1]) < 0) {
		pr_err("%s: Failed to read SOC1\n", __func__);
		return -1;
	}
	
//	pr_info("%s: data[0]=%d, data[1]=%d\n", __func__, data[0], ((data[1]*100)/256));
	
	// calculating soc
	FGPureSOC = data[0]*100+((data[1]*100)/256);

	if(FGPureSOC >= 100)
	{
		FGAdjustSOC = FGPureSOC;
	}
	else
	{
		if(FGPureSOC >= 70)
			FGAdjustSOC = 100; //1%
		else
			FGAdjustSOC = 0; //0%
	}

	// rounding off and Changing to percentage.
	FGSOC=FGAdjustSOC/100;

	if(FGAdjustSOC%100 >= 50 )
	{
		FGSOC+=1;
	}

	if(FGSOC>=26)
	{
		FGSOC+=4;
	}
	else
	{
		FGSOC=(30*FGAdjustSOC)/26/100;
	}

	if(FGSOC>=100)
	{
		FGSOC=100;
	}

	return FGSOC;
	
}
#endif

unsigned int fg_reset_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 rst_cmd[2];
	s32 ret = 0;

	is_reset_soc = 1;
	/* Quick-start */
	rst_cmd[0] = 0x40;
	rst_cmd[1] = 0x00;

	ret = fg_i2c_write(client, MODE0_REG, rst_cmd);
	if (ret)
		pr_info("%s: failed reset SOC(%d)\n", __func__, ret);

	msleep(500);
	is_reset_soc = 0;
	return ret;
}

// [[junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504
void fuel_gauge_rcomp(int mode)
{
	struct i2c_client *client = fg_i2c_client;
	u8 rst_cmd[2];
	s32 ret = 0;

	printk("fuel_gauge_rcomp %d\n",mode);
	
#if defined(CONFIG_S5PC110_KEPLER_BOARD) || (defined CONFIG_S5PC110_T959_BOARD) || defined(CONFIG_S5PC110_FLEMING_BOARD)	
	#if defined(CONFIG_KEPLER_VER_B2) || defined(CONFIG_T959_VER_B5)
		switch (mode) {
			case FUEL_INT_1ST:
				rst_cmd[0] = 0xD0;
				rst_cmd[1] = 0x10; // 15%
				rcomp_status = 0;
				break;		
			case FUEL_INT_2ND:
				rst_cmd[0] = 0xD0;
				rst_cmd[1] = 0x1A; // 5%
				rcomp_status = 1;
				break;
			case FUEL_INT_3RD:
				rst_cmd[0] = 0xD0;
				rst_cmd[1] = 0x1E; // 1%
				rcomp_status = 2;
				break;
// [[ junghyunseok edit for exception code 20100511		
			default:
				rst_cmd[0] = 0xD0;
				rst_cmd[1] = 0x1E; // 1%
				rcomp_status = 2;				
				break;		
// ]] junghyunseok edit for exception code 20100511
		}
	#else	
		rst_cmd[0] = 0xD0;
		rst_cmd[1] = 0x00;
	#endif	
#else
	rst_cmd[0] = 0xB0;
	rst_cmd[1] = 0x00;
#endif

	ret = fg_i2c_write(client, RCOMP0_REG, rst_cmd);
	if (ret)
		pr_info("%s: failed fuel_gauge_rcomp(%d)\n", __func__, ret);
	
	//msleep(500);
}
// ]]junghyunseok edit for fuel_int interrupt control of fuel_gauge 20100504

static int fg_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;

	pr_info("%s\n", __func__);

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));

	strncpy(c->name, fg_i2c_driver.driver.name, I2C_NAME_SIZE);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &fg_i2c_driver;

	if ((ret = i2c_attach_client(c)))
		goto error;

	fg_i2c_client = c;

error:
	return ret;
}

static int fg_attach_adapter(struct i2c_adapter *adap)
{
	//pr_info("%s\n", __func__);
	return i2c_probe(adap, &fg_addr_data, fg_attach);
}

static int fg_detach_client(struct i2c_client *client)
{
	pr_info("%s\n", __func__);
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver fg_i2c_driver = {
	.driver = {
		.name = "Fuel Gauge I2C",
		.owner = THIS_MODULE,
	},
	.id 		= 0,
	.attach_adapter	= fg_attach_adapter,
	.detach_client	= fg_detach_client,
	.command	= NULL,
};
