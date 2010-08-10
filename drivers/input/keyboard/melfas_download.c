//------------------------------------------------------------------
//
//	MELFAS Firmware download base code v6 For MCS5080 2008/11/04
//
//------------------------------------------------------------------
#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <asm/gpio.h>
#include "melfas_download.h"

#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>

//============================================================
//
//	Static variables & functions
//
//============================================================

//---------------------------------
//	Downloading functions
//---------------------------------
static int  mcsdl_download(const UINT8 *pData, const UINT16 nLength);

static int  mcsdl_enter_download_mode(void);
static void mcsdl_write_download_mode_signal(void);

static int  mcsdl_i2c_erase_flash(void);
static int  mcsdl_i2c_prepare_erase_flash(void);
static int  mcsdl_i2c_read_flash( UINT8 *pBuffer, UINT16 nAddr_start, UINT8 cLength);
static int  mcsdl_i2c_prepare_program(void);
static int  mcsdl_i2c_program_flash( UINT8 *pData, UINT16 nAddr_start, UINT8 cLength );

//-----------------------------------------
//	Download enable command on Protocol
//-----------------------------------------
#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD
void melfas_send_download_enable_command(void);
#endif

struct i2c_touchkey_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
};
extern struct i2c_touchkey_driver *touchkey_driver;
static spinlock_t		spinlock;
#define I2C_M_WR 0 /* for i2c */
#define uart_printf printk



//---------------------------------
//	I2C Functions
//---------------------------------
static BOOLEAN _i2c_read_( UINT8 slave_addr, UINT8 *pData, UINT8 cLength);
static BOOLEAN _i2c_write_(UINT8 slave_addr, UINT8 *pData, UINT8 cLength);

//---------------------------------
//	Delay functions
//---------------------------------
static void mcsdl_delay(UINT32 nCount);

//---------------------------------
//	For debugging display
//---------------------------------
#if MELFAS_ENABLE_DBG_PRINT
static void mcsdl_print_result(int nRet);
#endif


//////I2C

#define EXT_I2C_SCL_HIGH	TKEY_I2C_SCL_SET_HIGH();
#define EXT_I2C_SCL_LOW		TKEY_I2C_SCL_SET_LOW();
#define EXT_I2C_SDA_HIGH	TKEY_I2C_SDA_SET_HIGH();
#define EXT_I2C_SDA_LOW		TKEY_I2C_SDA_SET_LOW();

static void SCLH_SDAH(u32 delay)
{
	EXT_I2C_SCL_HIGH;
	EXT_I2C_SDA_HIGH;
	udelay(delay);
}

static void SCLH_SDAL(u32 delay)
{
	EXT_I2C_SCL_HIGH;
	EXT_I2C_SDA_LOW;
	udelay(delay);
}

static void SCLL_SDAH(u32 delay)
{
	EXT_I2C_SCL_LOW;
	EXT_I2C_SDA_HIGH;
	udelay(delay);
}

static void SCLL_SDAL(u32 delay)
{
	EXT_I2C_SCL_LOW;
	EXT_I2C_SDA_LOW;
	udelay(delay);
}

static void EXT_I2C_LOW(u32 delay)
{
	SCLL_SDAL(delay);
	SCLH_SDAL(delay);
	SCLH_SDAL(delay);
	SCLL_SDAL(delay);
}

static void EXT_I2C_HIGH(u32 delay)
{
	SCLL_SDAH(delay);
	SCLH_SDAH(delay);
	SCLH_SDAH(delay);
	SCLL_SDAH(delay);
}

static void EXT_I2C_START(u32 delay)
{
	SCLH_SDAH(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLL_SDAL(delay);
}

static void EXT_I2C_RESTART(u32 delay)
{
	SCLL_SDAH(delay);
	SCLH_SDAH(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLL_SDAL(delay);
}

static void EXT_I2C_END(u32 delay)
{
	SCLL_SDAL(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLH_SDAH(delay);
}

static void EXT_I2C_ACK(u32 delay)
{
	u32 ack;

	EXT_I2C_SCL_LOW;
	udelay(delay);
	
	/* SDA -> Input */
	TKEY_I2C_SDA_SET_INPUT();
	
	EXT_I2C_SCL_HIGH;
	udelay(delay);
	ack = gpio_get_value(_3_TOUCH_SDA_28V);
	EXT_I2C_SCL_HIGH;
	udelay(delay);
	
	/* SDA -> Onput Low */
	TKEY_I2C_SDA_SET_LOW();

	EXT_I2C_SCL_LOW;
	udelay(delay);
	
	if (ack)
		printk("EXT_I2C(%d) -> No ACK\n");
}

static void EXT_I2C_NACK(u32 delay)
{
	EXT_I2C_HIGH(delay);
}

static void EXT_I2C_SEND_ACK(u32 delay)
{
	u32 ack;

	EXT_I2C_SCL_LOW;
	udelay(delay);
	
	/* SDA -> Input */
	TKEY_I2C_SDA_SET_INPUT();
	udelay(delay);
	ack = gpio_get_value(_3_TOUCH_SDA_28V);

		/* SDA -> Onput Low */
	TKEY_I2C_SDA_SET_LOW();

	EXT_I2C_SCL_HIGH;
//	udelay(delay);
//	ack = gpio_get_value(_3_TOUCH_SDA_28V);
//	EXT_I2C_SCL_HIGH;
	udelay(delay);
	
	EXT_I2C_SCL_LOW;
	udelay(delay);
	
}

#define EXT_I2C_DELAY	1
//============================================================
//
//	Porting section 6.	I2C function calling
//
//	Connect baseband i2c function
//
//	Warning 1. !!!!  Burst mode is not supported. Transfer 1 byte Only.
//
//		Every i2c packet has to
//			" START > Slave address > One byte > STOP " at download mode.
//
//	Warning 2. !!!!  Check return value of i2c function.
//
//		_i2c_read_(), _i2c_write_() must return
//			TRUE (1) if success,
//			FALSE(0) if failed.
//
//		If baseband i2c function returns different value, convert return value.
//			ex> baseband_return = baseband_i2c_read( slave_addr, pData, cLength );
//				return ( baseband_return == BASEBAND_RETURN_VALUE_SUCCESS );
//
//
//	Warning 3. !!!!  Check Slave address
//
//		Slave address is '0x7F' at download mode. ( Diffrent with Normal touch working mode )
//		'0x7F' is original address,
//			If shift << 1 bit, It becomes '0xFE'
//
//============================================================

static BOOLEAN _i2c_read_( UINT8 SlaveAddr, UINT8 *pData, UINT8 cLength)
{
#if 1
	u32 i;
	int delay_count = 10000;

	//ext_i2c_channel = Channel;

	EXT_I2C_START(EXT_I2C_DELAY);
/*
	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			EXT_I2C_HIGH(EXT_I2C_DELAY);
		else
			EXT_I2C_LOW(EXT_I2C_DELAY);
	}

	EXT_I2C_LOW(EXT_I2C_DELAY);

	EXT_I2C_ACK(EXT_I2C_DELAY);
	
	for (i = 8; i > 0; i--) {
		if ((WordAddr >> (i - 1)) & 0x1)
			EXT_I2C_HIGH(EXT_I2C_DELAY);
		else
			EXT_I2C_LOW(EXT_I2C_DELAY);
	}

	EXT_I2C_ACK(EXT_I2C_DELAY);
	
	EXT_I2C_RESTART(EXT_I2C_DELAY);
*/	
	SlaveAddr = SlaveAddr <<1;
	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			EXT_I2C_HIGH(EXT_I2C_DELAY);
		else
			EXT_I2C_LOW(EXT_I2C_DELAY);
	}

	EXT_I2C_HIGH(EXT_I2C_DELAY);
	
	EXT_I2C_ACK(EXT_I2C_DELAY);

	udelay(60);
	TKEY_I2C_SDA_SET_INPUT();
	TKEY_I2C_SCL_SET_INPUT();
	//while(!gpio_get_value(_3_TOUCH_SCL_28V));
	delay_count = 100000;
	while(delay_count--)
	{
		if(gpio_get_value(_3_TOUCH_SCL_28V))
			break;
		udelay(1);
	}
	//udelay(EXT_I2C_DELAY);
	while(cLength--)
	{
		*pData = 0;
		for (i = 8; i > 0; i--) {
			//EXT_I2C_SCL_LOW;
			udelay(EXT_I2C_DELAY);
			EXT_I2C_SCL_HIGH;
			udelay(EXT_I2C_DELAY);
			*pData |= (!!(gpio_get_value(_3_TOUCH_SDA_28V)) << (i - 1));
			EXT_I2C_SCL_HIGH;
			udelay(EXT_I2C_DELAY);
			EXT_I2C_SCL_LOW;
			udelay(EXT_I2C_DELAY);
		}	

		if(cLength)
		{
			EXT_I2C_SEND_ACK(EXT_I2C_DELAY);
			udelay(60);
			pData++;
			TKEY_I2C_SDA_SET_INPUT();
			TKEY_I2C_SCL_SET_INPUT();
			//while(!gpio_get_value(_3_TOUCH_SCL_28V));
			delay_count = 100000;
			while(delay_count--)
			{
				if(gpio_get_value(_3_TOUCH_SCL_28V))
					break;
				udelay(1);
			}
		}
		else
			EXT_I2C_NACK(EXT_I2C_DELAY);
	}
	
	EXT_I2C_END(EXT_I2C_DELAY);

	return (TRUE );
#else

	BOOLEAN bRet;
	int 	 err;
	#if USE_BASEBAND_I2C_FUNCTION
	struct 	 i2c_msg msg[1];
	if( (touchkey_driver->client == NULL) || (!touchkey_driver->client->adapter) )
	{
		return -ENODEV;
	}
	
	msg->addr 	= MCSDL_I2C_SLAVE_ADDR_ORG;
	msg->flags = I2C_M_RD;
	msg->len   = cLength;
	msg->buf   = pData;
	err = i2c_transfer(touchkey_driver->client->adapter, msg, 1);

	if (err < 0) 
	{
		printk("%s %d i2c transfer error\n", __func__, __LINE__);/* add by inter.park */
		return FALSE;
	}

	#else

	bRet = FALSE;

	#endif

	return (TRUE );
#endif
}

static BOOLEAN _i2c_write_(UINT8 SlaveAddr, UINT8 *pData, UINT8 cLength)
{
#if 1
	u32 i;
	//ext_i2c_channel = Channel;

	EXT_I2C_START(EXT_I2C_DELAY);
/*
	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			EXT_I2C_HIGH(EXT_I2C_DELAY);
		else
			EXT_I2C_LOW(EXT_I2C_DELAY);
	}

	EXT_I2C_LOW(EXT_I2C_DELAY);

	EXT_I2C_ACK(EXT_I2C_DELAY);
*/	SlaveAddr = SlaveAddr <<1;
	for (i = 8; i > 0; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			EXT_I2C_HIGH(EXT_I2C_DELAY);
		else
			EXT_I2C_LOW(EXT_I2C_DELAY);
	}

	EXT_I2C_ACK(EXT_I2C_DELAY);
	
	for (i = 8; i > 0; i--) {
		if ((*pData >> (i - 1)) & 0x1)
			EXT_I2C_HIGH(EXT_I2C_DELAY);
		else
			EXT_I2C_LOW(EXT_I2C_DELAY);
	}

	EXT_I2C_ACK(EXT_I2C_DELAY);

	EXT_I2C_END(EXT_I2C_DELAY);

	return (TRUE );
#else
	BOOLEAN bRet;
	int 	 err;

	#if USE_BASEBAND_I2C_FUNCTION

	struct 	 i2c_msg msg[1];
	if( (touchkey_driver->client == NULL) || (!touchkey_driver->client->adapter) )
	{
		return -ENODEV;
	}
	
	msg->addr 	= MCSDL_I2C_SLAVE_ADDR_ORG;
	msg->flags = I2C_M_WR;
	msg->len   = cLength;
	msg->buf   = pData;
	err = i2c_transfer(touchkey_driver->client->adapter, msg, 1);

	if (err < 0) 
	{
		printk("%s %d i2c transfer error\n", __func__, __LINE__);/* add by inter.park */
		return FALSE;
	}
	#else

	bRet = FALSE;

	#endif

	return (TRUE);
#endif
}

//----------------------------------
// Download enable command
//----------------------------------
#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD

void melfas_send_download_enable_command(void)
{
	// TO DO : Fill this up

}

#endif


//============================================================
//
//	Include MELFAS Binary source code File ( ex> MELFAS_FIRM_bin.c)
//
//	Warning!!!!
//		.c file included at next must not be inserted to Souce project.
//		Just #include One file. !!
//
//============================================================
//#include "MCS5080_SAMPLE_FIRMWARE_R01_V00_bin.c"
//#include "MMH_SVESTA_R00_V02_bin.c"  //1 include bin file
//#include "VTW0008_R04_V03_bin.c"  //1 include bin file
#ifdef CONFIG_S5PC110_KEPLER_BOARD 
extern unsigned int HWREV;

#include "VTW0014_R00_V14_bin.c"
#include "VTW0014_R00_V15_bin.c"
#elif CONFIG_S5PC110_T959_BOARD
extern unsigned int HWREV;
#include "MMH_ST959_R01_V07_bin.c"
#include "VTW0008_R04_V03_bin.c"
#else
#include "VTW0008_R04_V06_bin.c"
#endif 
//============================================================
//
//	main Download furnction
//
//============================================================
#define IRQ_TOUCH_INT S3C_GPIOINT(J4,1)
int get_touchkey_firmware(char * version)
{

	int retry=3;
	while(retry--)
	{
	if(_i2c_read_(TOUCHKEY_ADDRESS, version, 3))
		return 0;
	
	}
	return (-1);

	//printk("%s F/W version: 0x%x, Module version:0x%x\n",__FUNCTION__, version[1],version[2]);
}


int mcsdl_download_binary_data(void)
{
	int ret;
	UINT8 data[3];

	
	_i2c_read_(TOUCHKEY_ADDRESS, data, 3 );
	printk("%s F/W version: 0x%x, Module version:0x%x\n",__FUNCTION__, data[1],data[2]);


	#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD

	melfas_send_download_enable_command();

	mcsdl_delay(MCSDL_DELAY_100US);

	#endif

	MELFAS_DISABLE_BASEBAND_ISR();				// Disable Baseband touch interrupt ISR.
	MELFAS_DISABLE_WATCHDOG_TIMER_RESET();			// Disable Baseband watchdog timer

	//------------------------
	// Run Download
	//------------------------
	#ifdef CONFIG_S5PC110_KEPLER_BOARD
	if(HWREV == 0x08 || HWREV == 0x04 || HWREV == 0x0C || HWREV == 0x02 || HWREV == 0x0A)
		ret = mcsdl_download( (const UINT8*) MELFAS_binary_v14, (const UINT16)MELFAS_binary_nLength_v14 );
	else
		ret = mcsdl_download( (const UINT8*) MELFAS_binary_v15, (const UINT16)MELFAS_binary_nLength_v15 );
	
	#elif CONFIG_S5PC110_T959_BOARD
	if(HWREV < 0x0E )
		ret = mcsdl_download( (const UINT8*) MELFAS_binary_v03, (const UINT16)MELFAS_binary_nLength_v03 );
	else if(HWREV == 0x0E)
		ret = mcsdl_download( (const UINT8*) MELFAS_binary_v07, (const UINT16)MELFAS_binary_nLength_v07 );

	#else
	ret = mcsdl_download( (const UINT8*) MELFAS_binary, (const UINT16)MELFAS_binary_nLength );
	#endif

	MELFAS_DISABLE_WATCHDOG_TIMER_RESET();					// Roll-back Baseband touch interrupt ISR.
	MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET();			// Roll-back Baseband watchdog timer

	#if MELFAS_ENABLE_DBG_PRINT

		//------------------------
		// Show result
		//------------------------

		mcsdl_print_result( ret );

	#endif

	
	gpio_direction_output(_3_GPIO_TOUCH_EN, 1);
	mdelay(1);
	gpio_direction_output(_3_GPIO_TOUCH_CE, 1);
	mdelay(100);
	s3c_gpio_setpull(_3_GPIO_TOUCH_INT, S3C_GPIO_PULL_NONE); 
	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_EDGE_FALLING);

	return ( ret == MCSDL_RET_SUCCESS );
}


#define FILE_PATH "/system/etc/melfas/MELFAS_FIRM.bin"

int mcsdl_download_binary_file(void)
{
	UINT8 data[3];
	int ret;

	UINT8  *pData = NULL;
	UINT16 nBinary_length =0;
	int  nRead;

	//==================================================
	//
	//	Porting section 7. File process
	//
	//	1. Read '.bin file'
	//	2. Run mcsdl_download_binary_data();
	//
	//==================================================

	#if 1
	struct file* filp;
	long l;
	loff_t pos;
	
	mm_segment_t old_fs;
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	
	filp = filp_open(FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(filp))
	{
		printk(KERN_INFO "Unable to load '%s'.\n", FILE_PATH);
		return 0;
	}
	l = filp->f_path.dentry->d_inode->i_size;
	if (l <= 0 || l > 20*1024)
	{
		printk(KERN_INFO "Invalid firmware '%s'\n", FILE_PATH);
		filp_close(filp, current->files);
		return 0;
	}
	pData = vmalloc(l);
	if (pData == NULL)
	{
		printk(KERN_INFO "Out of memory loading '%s'.\n", FILE_PATH);
		filp_close(filp, current->files);
		return 0;
	}
	pos = 0;
	if (vfs_read(filp, pData, l, &pos) != l)
	{
		printk(KERN_INFO "Failed to read '%s'.\n", FILE_PATH);
		vfree(pData);
		filp_close(filp, current->files);
		return 0;
	}
	filp_close(filp, current->files);

	set_fs(old_fs);
	nBinary_length = l;
#if 0	
	int fp;
	mm_segment_t old_fs;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open("/sdcard/sd/MELFAS_FIRM.bin", O_RDONLY, 0777);
	if( fp == NULL ){
		printk("mcsdl_download_binary_file open error \n");
		return MCSDL_RET_FILE_ACCESS_FAILED;
	}	
	nBinary_length = sys_lseek(fp, 0, SEEK_END); //END

	pData = (UINT8*)vmalloc( (int)nBinary_length );

	if( pData == NULL ){
		printk("%s file malloc fail\n", __FUNCTION__);
		return MCSDL_RET_FILE_ACCESS_FAILED;
	}
	nRead = sys_read(fp, pData, nBinary_length);

	if( nRead != (int)nBinary_length ){
		printk("%s file read error\n", __FUNCTION__);
		sys_close(fp);												// Close file

		if( pData != NULL )										// free memory alloced.
			vfree(pData);

		return MCSDL_RET_FILE_ACCESS_FAILED;
	}
	sys_close(fp);
	set_fs(old_fs);
#endif

	#else

		// TO DO : File Process & Get file Size(== Binary size)
		//			This is just a simple sample

		FILE *fp;
		INT  nRead;

		//------------------------------
		// Open a file
		//------------------------------

		if( fopen( fp, "/sdcard/sd/MELFAS_FIRM.bin", "rb" ) == NULL ){
			printk("%s file open error \n", __FUNCTION__);
			return MCSDL_RET_FILE_ACCESS_FAILED;
		}

		//------------------------------
		// Get Binary Size
		//------------------------------

		fseek( fp, 0, SEEK_END );

		nBinary_length = (UINT16)ftell(fp);

		//------------------------------
		// Memory allocation
		//------------------------------

		pData = (UINT8*)malloc( (INT)nBinary_length );

		if( pData == NULL ){
			printk("%s file malloc fail\n", __FUNCTION__);
			return MCSDL_RET_FILE_ACCESS_FAILED;
		}

		//------------------------------
		// Read binary file
		//------------------------------

		fseek( fp, 0, SEEK_SET );

		nRead = fread( pData, 1, (INT)nBinary_length, fp );		// Read binary file

		if( nRead != (INT)nBinary_length ){
			printk("%s file read error\n", __FUNCTION__);
			fclose(fp);												// Close file

			if( pData != NULL )										// free memory alloced.
				free(pData);

			return MCSDL_RET_FILE_ACCESS_FAILED;
		}

		//------------------------------
		// Close file
		//------------------------------

		fclose(fp);

	#endif

	if( pData != NULL && nBinary_length > 0 && nBinary_length < 14*1024 ){

		MELFAS_DISABLE_BASEBAND_ISR();				// Disable Baseband touch interrupt ISR.
		MELFAS_DISABLE_WATCHDOG_TIMER_RESET();			// Disable Baseband watchdog timer

		ret = mcsdl_download( (const UINT8 *)pData, (const UINT16)nBinary_length );

		MELFAS_DISABLE_WATCHDOG_TIMER_RESET();						// Roll-back Baseband touch interrupt ISR.
		MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET();			// Roll-back Baseband watchdog timer

	}else{

		ret = MCSDL_RET_WRONG_PARAMETER;
	}

	#if MELFAS_ENABLE_DBG_PRINT

	mcsdl_print_result( ret );

	#endif

	#if 1
		if( pData != NULL )										// free memory alloced.
			vfree(pData);
	#endif

	gpio_direction_output(_3_GPIO_TOUCH_EN, 1);
	mdelay(1);
	gpio_direction_output(_3_GPIO_TOUCH_CE, 1);
	mdelay(100);
	s3c_gpio_setpull(_3_GPIO_TOUCH_INT, S3C_GPIO_PULL_NONE); 
	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_EDGE_FALLING);

	_i2c_read_(TOUCHKEY_ADDRESS, data, 3 );
	printk("%s F/W version: 0x%x, Module version:0x%x\n",__FUNCTION__, data[1],data[2]);

	return ( ret == MCSDL_RET_SUCCESS );

}

//------------------------------------------------------------------
//
//	Download function
//
//------------------------------------------------------------------

static int mcsdl_download(const UINT8 *pData, const UINT16 nLength )
{
	int		i;
	int		nRet;

	UINT8   cLength;
	UINT16  nStart_address=0;

	UINT8	buffer[MELFAS_TRANSFER_LENGTH];
	UINT8	*pOriginal_data;


	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	uart_printf("Starting download...\n");
	#endif

	//--------------------------------------------------------------
	//
	// Enter Download mode
	//
	//--------------------------------------------------------------
	nRet = mcsdl_enter_download_mode();

	if( nRet != MCSDL_RET_SUCCESS )
		goto MCSDL_DOWNLOAD_FINISH;

	mdelay(MCSDL_DELAY_1MS);					// Delay '1 msec'

	//--------------------------------------------------------------
	//
	// Check H/W Revision
	//
	// Don't download firmware, if Module H/W revision does not match.
	//
	//--------------------------------------------------------------
	#if MELFAS_DISABLE_DOWNLOAD_IF_MODULE_VERSION_DOES_NOT_MATCH

		#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		uart_printf("Checking module revision...\n");
		#endif

		pOriginal_data  = (UINT8 *)pData;

		nRet = mcsdl_i2c_read_flash( buffer, MCSDL_ADDR_MODULE_REVISION, 4 );

		if( nRet != MCSDL_RET_SUCCESS )
			goto MCSDL_DOWNLOAD_FINISH;

		if( 	(pOriginal_data[MCSDL_ADDR_MODULE_REVISION+1] != buffer[1])
			||	(pOriginal_data[MCSDL_ADDR_MODULE_REVISION+2] != buffer[2]) ){

			nRet = MCSDL_RET_WRONG_MODULE_REVISION;
			goto MCSDL_DOWNLOAD_FINISH;
		}

		mdelay(MCSDL_DELAY_1MS);					// Delay '1 msec'

	#endif

	//--------------------------------------------------------------
	//
	// Erase Flash
	//
	//--------------------------------------------------------------

	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	uart_printf("Erasing...\n");
	#endif

	nRet = mcsdl_i2c_prepare_erase_flash();

	if( nRet != MCSDL_RET_SUCCESS ){
		goto MCSDL_DOWNLOAD_FINISH;
	}

	mdelay(MCSDL_DELAY_1MS);					// Delay '1 msec'

	nRet = mcsdl_i2c_erase_flash();

	if( nRet != MCSDL_RET_SUCCESS ){
		goto MCSDL_DOWNLOAD_FINISH;
	}

	mdelay(MCSDL_DELAY_1MS);					// Delay '1 msec'


	//--------------------------------------------------------------
	//
	// Verify erase
	//
	//--------------------------------------------------------------
	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	uart_printf("Verify Erasing...\n");
	#endif

	nRet = mcsdl_i2c_read_flash( buffer, 0x00, 16 );

	if( nRet != MCSDL_RET_SUCCESS )
		goto MCSDL_DOWNLOAD_FINISH;


	for(i=0; i<16; i++){

		if( buffer[i] != 0xFF ){

			nRet = MCSDL_RET_ERASE_VERIFY_FAILED;
			goto MCSDL_DOWNLOAD_FINISH;
		}
	}

	mdelay(MCSDL_DELAY_1MS);					// Delay '1 msec'


	//--------------------------------------------------------------
	//
	// Prepare for Program flash.
	//
	//--------------------------------------------------------------
	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	uart_printf("Program information...\n");
	#endif

	nRet = mcsdl_i2c_prepare_program();

	if( nRet != MCSDL_RET_SUCCESS )
		goto MCSDL_DOWNLOAD_FINISH;


	mdelay(MCSDL_DELAY_1MS);					// Delay '1 msec'


	//--------------------------------------------------------------
   //
   // Program flash
   //
	//--------------------------------------------------------------

	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	uart_printf("Program flash...  ");
	#endif

	pOriginal_data  = (UINT8 *)pData;

	nStart_address = 0;
	cLength  = MELFAS_TRANSFER_LENGTH;

	for( nStart_address = 0; nStart_address < nLength; nStart_address+=cLength ){

		#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		uart_printf("#");
		#endif

		if( ( nLength - nStart_address ) < MELFAS_TRANSFER_LENGTH ){
			cLength  = (UINT8)(nLength - nStart_address);

			cLength += (cLength%2);									// For odd length.
		}

		nRet = mcsdl_i2c_program_flash( pOriginal_data, nStart_address, cLength );

        if( nRet != MCSDL_RET_SUCCESS ){

			#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
			uart_printf("Program flash failed position : 0x%x / nRet : 0x%x ", nStart_address, nRet);
			#endif

            goto MCSDL_DOWNLOAD_FINISH;
		}

		pOriginal_data  += cLength;

		mcsdl_delay(MCSDL_DELAY_500US);					// Delay '500 usec'

	}


	//--------------------------------------------------------------
	//
	// Verify flash
	//
	//--------------------------------------------------------------

	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	uart_printf("\n");
	uart_printf("Verify flash...   ");
	#endif

	pOriginal_data  = (UINT8 *) pData;

	nStart_address = 0;

	cLength  = MELFAS_TRANSFER_LENGTH;

	for( nStart_address = 0; nStart_address < nLength; nStart_address+=cLength ){

		#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		uart_printf("#");
		#endif

		if( ( nLength - nStart_address ) < MELFAS_TRANSFER_LENGTH ){
			cLength = (UINT8)(nLength - nStart_address);

			cLength += (cLength%2);									// For odd length.
		}

		//--------------------
		// Read flash
		//--------------------
		nRet = mcsdl_i2c_read_flash( buffer, nStart_address, cLength );

		//--------------------
		// Comparing
		//--------------------

		for(i=0; i<(int)cLength; i++){


			if( buffer[i] != pOriginal_data[i] ){

				#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
				uart_printf("0x%04X : 0x%02X - 0x%02X\n", nStart_address, pOriginal_data[i], buffer[i] );
                #endif

				nRet = MCSDL_RET_PROGRAM_VERIFY_FAILED;
				goto MCSDL_DOWNLOAD_FINISH;

			}
		}

		pOriginal_data += cLength;


		mcsdl_delay(MCSDL_DELAY_500US);					// Delay '500 usec'
	}

	uart_printf("\n");

	nRet = MCSDL_RET_SUCCESS;


MCSDL_DOWNLOAD_FINISH :

	mdelay(MCSDL_DELAY_1MS);					// Delay '1 msec'

	//---------------------------
	//	Reset command
	//---------------------------
	buffer[0] = MCSDL_ISP_CMD_RESET;

	_i2c_write_( MCSDL_I2C_SLAVE_ADDR_ORG, buffer, 1 );

	TKEY_INTR_SET_INPUT();

    mcsdl_delay(MCSDL_DELAY_45MS);

	return nRet;
}


//------------------------------------------------------------------
//
//   Enter Download mode ( MDS ISP or I2C ISP )
//
//------------------------------------------------------------------
static int mcsdl_enter_download_mode(void)
{
	BOOLEAN	bRet;
	int    	nRet = MCSDL_RET_ENTER_DOWNLOAD_MODE_FAILED;

	UINT8 	cData=0;

	//--------------------------------------------
	// Tkey module reset
	//--------------------------------------------

	TKEY_VDD_SET_LOW();

	TKEY_CE_SET_LOW();
	TKEY_CE_SET_OUTPUT();

	TKEY_I2C_CLOSE();

	TKEY_INTR_SET_LOW();
	TKEY_INTR_SET_OUTPUT();

	TKEY_RESETB_SET_LOW();
	TKEY_RESETB_SET_OUTPUT();

	mcsdl_delay(MCSDL_DELAY_45MS);					// Delay for VDD LOW Stable
	mcsdl_delay(MCSDL_DELAY_45MS);
	
	TKEY_VDD_SET_HIGH();
	TKEY_CE_SET_HIGH();
	TKEY_I2C_SDA_SET_HIGH();

	mdelay(45); 						// Delay '45 msec'

	//-------------------------------
	// Write 1st signal
	//-------------------------------
	mcsdl_write_download_mode_signal();

	mcsdl_delay(MCSDL_DELAY_25MS); 						// Delay '25 msec'

	//-------------------------------
	// Check response
	//-------------------------------

	bRet = _i2c_read_( MCSDL_I2C_SLAVE_ADDR_ORG, &cData, 1 );

	if( bRet != TRUE || cData != MCSDL_I2C_SLAVE_READY_STATUS ){

		uart_printf("mcsdl_enter_download_mode() returns - ret : 0x%x & cData : 0x%x\n", nRet, cData);
		goto MCSDL_ENTER_DOWNLOAD_MODE_FINISH;
	}

	nRet = MCSDL_RET_SUCCESS;

	//-----------------------------------
	// Entering MDS ISP mode finished.
	//-----------------------------------

MCSDL_ENTER_DOWNLOAD_MODE_FINISH:

   return nRet;
}

//--------------------------------------------
//
//   Write ISP Mode entering signal
//
//--------------------------------------------
static void mcsdl_write_download_mode_signal(void)
{
	int    i;

	UINT8 enter_code[14] = { 0, 1, 0, 1, 0, 1, 0, 1,   1, 0, 1, 1, 1, 1 };

	//---------------------------
	// ISP mode signal 0
	//---------------------------

	for(i=0; i<14; i++){

		if( enter_code[i] )	{

			TKEY_RESETB_SET_HIGH();
			TKEY_INTR_SET_HIGH();

		}else{

			TKEY_RESETB_SET_LOW();
			TKEY_INTR_SET_LOW();
		}

		TKEY_I2C_SCL_SET_HIGH();	mcsdl_delay(MCSDL_DELAY_15US);
		TKEY_I2C_SCL_SET_LOW();

		TKEY_RESETB_SET_LOW();
		TKEY_INTR_SET_LOW();

		mcsdl_delay(MCSDL_DELAY_100US);

   }

	TKEY_I2C_SCL_SET_HIGH();

	mcsdl_delay(MCSDL_DELAY_100US);

	TKEY_INTR_SET_HIGH();
	TKEY_RESETB_SET_HIGH();
}


//--------------------------------------------
//
//   Prepare Erase flash
//
//--------------------------------------------
static int mcsdl_i2c_prepare_erase_flash(void)
{
	int   nRet = MCSDL_RET_PREPARE_ERASE_FLASH_FAILED;

	UINT8 i;
	BOOLEAN   bRet;

	UINT8 i2c_buffer[4] = {	MCSDL_ISP_CMD_ERASE_TIMING,
	                        MCSDL_ISP_ERASE_TIMING_VALUE_0,
	                        MCSDL_ISP_ERASE_TIMING_VALUE_1,
	                        MCSDL_ISP_ERASE_TIMING_VALUE_2   };
	UINT8	ucTemp;

   //-----------------------------
   // Send Erase Setting code
   //-----------------------------

   for(i=0; i<4; i++){

		bRet = _i2c_write_(MCSDL_I2C_SLAVE_ADDR_ORG, &i2c_buffer[i], 1 );

		if( !bRet )
			goto MCSDL_I2C_PREPARE_ERASE_FLASH_FINISH;

		mcsdl_delay(MCSDL_DELAY_15US);
   }

   //-----------------------------
   // Read Result
   //-----------------------------

	mcsdl_delay(MCSDL_DELAY_500US);                  // Delay 500usec

	bRet = _i2c_read_(MCSDL_I2C_SLAVE_ADDR_ORG, &ucTemp, 1 );

	if( bRet && ucTemp == MCSDL_ISP_ACK_PREPARE_ERASE_DONE ){

		nRet = MCSDL_RET_SUCCESS;

	}


MCSDL_I2C_PREPARE_ERASE_FLASH_FINISH :

   return nRet;

}


//--------------------------------------------
//
//   Erase flash
//
//--------------------------------------------
static int mcsdl_i2c_erase_flash(void)
{
	int   nRet = MCSDL_RET_ERASE_FLASH_FAILED;

	UINT8 	i;
	BOOLEAN	bRet;

	UINT8 	i2c_buffer[1] = {	MCSDL_ISP_CMD_ERASE};
	UINT8 	ucTemp;

   //-----------------------------
   // Send Erase code
   //-----------------------------

   for(i=0; i<1; i++){

		bRet = _i2c_write_(MCSDL_I2C_SLAVE_ADDR_ORG, &i2c_buffer[i], 1 );

		if( !bRet )
			goto MCSDL_I2C_ERASE_FLASH_FINISH;

		mcsdl_delay(MCSDL_DELAY_15US);
   }

   //-----------------------------
   // Read Result
   //-----------------------------

	mcsdl_delay(MCSDL_DELAY_45MS);                  // Delay 45ms


	bRet = _i2c_read_(MCSDL_I2C_SLAVE_ADDR_ORG, &ucTemp, 1 );

	if( bRet && ucTemp == MCSDL_ISP_ACK_ERASE_DONE ){

		nRet = MCSDL_RET_SUCCESS;

	}


MCSDL_I2C_ERASE_FLASH_FINISH :

   return nRet;

}

//--------------------------------------------
//
//   Read flash
//
//--------------------------------------------
static int mcsdl_i2c_read_flash( UINT8 *pBuffer, UINT16 nAddr_start, UINT8 cLength)
{
	int nRet = MCSDL_RET_READ_FLASH_FAILED;

	int     i;
	BOOLEAN bRet;
	UINT8   cmd[4];
	UINT8   ucTemp;

	//-----------------------------------------------------------------------------
	// Send Read Flash command   [ Read code - address high - address low - size ]
	//-----------------------------------------------------------------------------

	cmd[0] = MCSDL_ISP_CMD_READ_FLASH;
	cmd[1] = (UINT8)((nAddr_start >> 8 ) & 0xFF);
	cmd[2] = (UINT8)((nAddr_start      ) & 0xFF);
	cmd[3] = cLength;

	for(i=0; i<4; i++){

		bRet = _i2c_write_( MCSDL_I2C_SLAVE_ADDR_ORG, &cmd[i], 1 );

		mcsdl_delay(MCSDL_DELAY_15US);

		if( bRet == FALSE )
			goto MCSDL_I2C_READ_FLASH_FINISH;

   }

	//----------------------------------
	// Read 'Result of command'
	//----------------------------------
	bRet = _i2c_read_( MCSDL_I2C_SLAVE_ADDR_ORG, &ucTemp, 1 );

	if( !bRet || ucTemp != MCSDL_MDS_ACK_READ_FLASH){

		goto MCSDL_I2C_READ_FLASH_FINISH;
	}

	//----------------------------------
	// Read Data  [ pCmd[3] == Size ]
	//----------------------------------
	for(i=0; i<(int)cmd[3]; i++){

		mcsdl_delay(MCSDL_DELAY_100US);                  // Delay about 100us

		bRet = _i2c_read_( MCSDL_I2C_SLAVE_ADDR_ORG, pBuffer++, 1 );

		if( bRet == FALSE && i!=(int)(cmd[3]-1) )
			goto MCSDL_I2C_READ_FLASH_FINISH;
	}

	nRet = MCSDL_RET_SUCCESS;


MCSDL_I2C_READ_FLASH_FINISH :

	return nRet;
}


//--------------------------------------------
//
//   Prepare Program
//
//--------------------------------------------
static int mcsdl_i2c_prepare_program(void)
{

	int nRet = MCSDL_RET_PREPARE_PROGRAM_FAILED;

	int i;
	BOOLEAN bRet;

	UINT8 i2c_buffer[4] = { MCSDL_ISP_CMD_PROGRAM_TIMING,
		                    MCSDL_ISP_PROGRAM_TIMING_VALUE_0,
		                    MCSDL_ISP_PROGRAM_TIMING_VALUE_1,
		                    MCSDL_ISP_PROGRAM_TIMING_VALUE_2};

	//------------------------------------------------------
	//   Write Program timing information
	//------------------------------------------------------
	for(i=0; i<4; i++){

			bRet = _i2c_write_( MCSDL_I2C_SLAVE_ADDR_ORG, &i2c_buffer[i], 1 );

			if( bRet == FALSE )
				goto MCSDL_I2C_PREPARE_PROGRAM_FINISH;

			mcsdl_delay(MCSDL_DELAY_15US);
	}

	mcsdl_delay(MCSDL_DELAY_500US);                     // delay about  500us

	//------------------------------------------------------
	//   Read command's result
	//------------------------------------------------------
	bRet = _i2c_read_( MCSDL_I2C_SLAVE_ADDR_ORG, &i2c_buffer[4], 1 );

	if( bRet == FALSE || i2c_buffer[4] != MCSDL_I2C_ACK_PREPARE_PROGRAM)
		goto MCSDL_I2C_PREPARE_PROGRAM_FINISH;

	mcsdl_delay(MCSDL_DELAY_100US);                     // delay about  100us

   nRet = MCSDL_RET_SUCCESS;

MCSDL_I2C_PREPARE_PROGRAM_FINISH :

   return nRet;

}

//--------------------------------------------
//
//   Program Flash
//
//--------------------------------------------

static int mcsdl_i2c_program_flash( UINT8 *pData, UINT16 nAddr_start, UINT8 cLength )
{
	int nRet = MCSDL_RET_PROGRAM_FLASH_FAILED;

	int     i,j;
	BOOLEAN   bRet;
	UINT8    cData;
	UINT8    tmp;
	UINT8 cmd[4];


	//-----------------------------
	// Send Read code
	//-----------------------------

	cmd[0] = MCSDL_ISP_CMD_PROGRAM_FLASH;
	cmd[1] = (UINT8)((nAddr_start >> 8 ) & 0xFF);
	cmd[2] = (UINT8)((nAddr_start      ) & 0xFF);
	cmd[3] = cLength;

	for(i=0; i<4; i++){

		bRet = _i2c_write_(MCSDL_I2C_SLAVE_ADDR_ORG, &cmd[i], 1 );

		mcsdl_delay(MCSDL_DELAY_15US);

		if( bRet == FALSE )
			goto MCSDL_I2C_PROGRAM_FLASH_FINISH;

	}
	//-----------------------------
	// Check command result
	//-----------------------------

	bRet = _i2c_read_( MCSDL_I2C_SLAVE_ADDR_ORG, &cData, 1 );

	if( bRet == FALSE || cData != MCSDL_MDS_ACK_PROGRAM_FLASH ){

		goto MCSDL_I2C_PROGRAM_FLASH_FINISH;
	}


	//-----------------------------
	// Program Data
	//-----------------------------

	mcsdl_delay(MCSDL_DELAY_150US);                  // Delay about 150us

	for(i=0; i<(int)cmd[3]; i+=2){


		bRet = _i2c_write_( MCSDL_I2C_SLAVE_ADDR_ORG, &pData[i+1], 1 );

		mcsdl_delay(MCSDL_DELAY_150US);                  // Delay about 150us

		if( bRet == FALSE )
			goto MCSDL_I2C_PROGRAM_FLASH_FINISH;


		bRet = _i2c_write_( MCSDL_I2C_SLAVE_ADDR_ORG, &pData[i], 1 );

		mcsdl_delay(MCSDL_DELAY_150US);                  // Delay about 150us

		if( bRet == FALSE )
			goto MCSDL_I2C_PROGRAM_FLASH_FINISH;

	}


	nRet = MCSDL_RET_SUCCESS;

MCSDL_I2C_PROGRAM_FLASH_FINISH :

   return nRet;
}


//============================================================
//
//	Delay Function
//
//============================================================
static void mcsdl_delay(UINT32 nCount)
{

	#if 1

		udelay(nCount);			//1 Baseband delay function

	#else

		UINT32 i;

		for(i=0;i<nCount;i++){

		}

	#endif
}


//============================================================
//
//	Debugging print functions.
//
//	Change uart_printf() to Baseband printing function
//
//============================================================

#if MELFAS_ENABLE_DBG_PRINT

static void mcsdl_print_result(int nRet)
{
	if( nRet == MCSDL_RET_SUCCESS ){

		uart_printf(" MELFAS Firmware downloading SUCCESS.\n");

	}else{

		uart_printf(" MELFAS Firmware downloading FAILED  :  ");

		switch( nRet ){

			case MCSDL_RET_SUCCESS                  		:   uart_printf("MCSDL_RET_SUCCESS\n" );                 	break;
			case MCSDL_RET_ENTER_DOWNLOAD_MODE_FAILED   	:   uart_printf("MCSDL_RET_ENTER_ISP_MODE_FAILED\n" );      	break;
			case MCSDL_RET_ERASE_FLASH_FAILED           	:   uart_printf("MCSDL_RET_ERASE_FLASH_FAILED\n" );         	break;
			case MCSDL_RET_READ_FLASH_FAILED				:   uart_printf("MCSDL_RET_READ_FLASH_FAILED\n" );         	break;
			case MCSDL_RET_READ_EEPROM_FAILED           	:   uart_printf("MCSDL_RET_READ_EEPROM_FAILED\n" );         	break;
			case MCSDL_RET_READ_INFORMAION_FAILED        	:   uart_printf("MCSDL_RET_READ_INFORMAION_FAILED\n" );     	break;
			case MCSDL_RET_PROGRAM_FLASH_FAILED				:   uart_printf("MCSDL_RET_PROGRAM_FLASH_FAILED\n" );      	break;
			case MCSDL_RET_PROGRAM_EEPROM_FAILED        	:   uart_printf("MCSDL_RET_PROGRAM_EEPROM_FAILED\n" );      	break;
			case MCSDL_RET_PREPARE_PROGRAM_FAILED    		:   uart_printf("MCSDL_RET_PROGRAM_INFORMAION_FAILED\n" );   break;
			case MCSDL_RET_PROGRAM_VERIFY_FAILED			:   uart_printf("MCSDL_RET_PROGRAM_VERIFY_FAILED\n" );      	break;

			case MCSDL_RET_WRONG_MODE_ERROR             	:   uart_printf("MCSDL_RET_WRONG_MODE_ERROR\n" );         	break;
			case MCSDL_RET_WRONG_SLAVE_SELECTION_ERROR		:   uart_printf("MCSDL_RET_WRONG_SLAVE_SELECTION_ERROR\n" ); break;
			case MCSDL_RET_COMMUNICATION_FAILED				:   uart_printf("MCSDL_RET_COMMUNICATION_FAILED\n" );      	break;
			case MCSDL_RET_READING_HEXFILE_FAILED       	:   uart_printf("MCSDL_RET_READING_HEXFILE_FAILED\n" );      break;
			case MCSDL_RET_WRONG_PARAMETER       			:   uart_printf("MCSDL_RET_WRONG_PARAMETER\n" );      		break;
			case MCSDL_RET_FILE_ACCESS_FAILED       		:   uart_printf("MCSDL_RET_FILE_ACCESS_FAILED\n" );      	break;
			case MCSDL_RET_MELLOC_FAILED     		  		:   uart_printf("MCSDL_RET_MELLOC_FAILED\n" );      			break;
			case MCSDL_RET_WRONG_MODULE_REVISION     		:   uart_printf("MCSDL_RET_WRONG_MODULE_REVISION\n" );      	break;

			default                             			:	uart_printf("UNKNOWN ERROR. [0x%02X].\n", nRet );      	break;
		}

		uart_printf("\n");
	}

}

#endif

//============================================================
//
//	Porting section 4-1. Delay function
//
//	For initial testing of delay and gpio control
//
//	You can confirm GPIO control and delay time by calling this function.
//
//============================================================

#if MELFAS_ENABLE_DELAY_TEST


void mcsdl_delay_test(INT32 nCount)
{
	INT16 i;

	MELFAS_DISABLE_BASEBAND_ISR();// Disable Baseband touch interrupt ISR.
	MELFAS_DISABLE_WATCHDOG_TIMER_RESET();			// Disable Baseband watchdog timer

	TKEY_I2C_SET_OUTPUT();
	TKEY_CE_SET_OUTPUT();
	TKEY_INTR_SET_OUTPUT();
	TKEY_RESETB_SET_OUTPUT();

	//--------------------------------
	//	Repeating 'nCount' times
	//--------------------------------


	for( i=0; i<nCount; i++ ){

		TKEY_I2C_SET_HIGH();						// NORMAL
		TKEY_VDD_SET_HIGH();
		TKEY_CE_SET_HIGH();
		TKEY_RESETB_SET_HIGH();

		mcsdl_delay(MCSDL_DELAY_15US);

		TKEY_VDD_SET_LOW();							// VDD & CE LOW
		TKEY_CE_SET_LOW();
		TKEY_I2C_SCL_SET_LOW();						// SCL LOW

		mcsdl_delay(MCSDL_DELAY_100US)

		TKEY_VDD_SET_HIGH();						// VDD & CE HIGH
		TKEY_CE_SET_HIGH();
		TKEY_I2C_SCL_SET_HIGH();					// SCL HIGH


		TKEY_INTR_SET_LOW();						// INTR & RESETB LOW
		TKEY_RESETB_SET_LOW();
		TKEY_I2C_SCL_SET_LOW();						// SCL LOW

    	mcsdl_delay(MCSDL_DELAY_500US);

		TKEY_INTR_SET_HIGH();						// INTR & RESETB HIGH
		TKEY_RESETB_SET_HIGH();
		TKEY_I2C_SCL_SET_HIGH();					// SCL HIGH

		TKEY_I2C_SCL_SET_LOW();						// SCL LOW

    	mdelay(MCSDL_DELAY_1MS);

		TKEY_I2C_SCL_SET_HIGH();					// SCL HIGH

		TKEY_I2C_SDA_SET_LOW();						// SDA LOW

    	mcsdl_delay(MCSDL_DELAY_25MS);

		TKEY_I2C_SDA_SET_HIGH();					// SDA HIGH
		TKEY_I2C_SCL_SET_LOW();						// SCL LOW

    	mcsdl_delay(MCSDL_DELAY_45MS);

		TKEY_I2C_SCL_SET_HIGH();					// SCL HIGH
	}


    TKEY_INTR_SET_INPUT();

	MELFAS_DISABLE_WATCHDOG_TIMER_RESET();						// Roll-back Baseband touch interrupt ISR.
	MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET();			// Roll-back Baseband watchdog timer
}


#endif




