/* linux/arch/arm/mach-s5pc110/mach-p1p2.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>

#include <linux/i2c-gpio.h>

#include <linux/delay.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/videodev2.h>
#include <media/s5k3ba_platform.h>
#include <media/s5k4ba_platform.h>
#include <media/s5k6aa_platform.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/setup.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/gpio.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/regs-rtc.h>
#include <plat/iic.h>
#include <plat/fimc.h>
#include <plat/csis.h>
#include <plat/fb.h>
#include <plat/fimc-ipc.h>
#include <plat/spi.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s5pc110.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>
#include <plat/regs-fimc.h>
/* pmem */
#include <linux/android_pmem.h>
#include <plat/media.h>





struct class *sec_class;
EXPORT_SYMBOL(sec_class);


extern void s5pc11x_reserve_bootmem(void);
extern void universal_sdhci2_set_platdata(void);



#include "mach-common.c"




#if defined(CONFIG_TOUCHSCREEN_QT602240)
static struct platform_device s3c_device_qtts = {
	.name = "qt602240-ts",
	.id = -1,
};
#endif

#ifdef CONFIG_FB_S3C_LVDS

static void lvds_cfg_gpio(struct platform_device *pdev)
{
	int i,err;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
	writel(0x2, S5P_MDNIE_SEL);

	/* drive strength to max */
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x12c);
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x14c);
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x16c);
	writel(0x000000ff, S5PC11X_VA_GPIO + 0x18c);

        err = gpio_request(S5PC11X_MP04(1), "LVDS_RST");
        if (err) {
                printk(KERN_ERR "failed to request MP04(1) for "
                        "lcd reset control\n");
                return err;
        }

        gpio_direction_output(S5PC11X_MP04(1), 1);
        mdelay(100);

        gpio_set_value(S5PC11X_MP04(1), 0);
        mdelay(10);

}


static void lvds_backlight_start()
{
        gpio_direction_output(S5PC11X_MP05(1), 1);
        udelay(10);

        gpio_set_value(S5PC11X_MP05(1), 1);
        udelay(500);
}

static void lvds_backlight_setaddress(int j)
{        
        while(j>0) {
        	gpio_set_value(S5PC11X_MP05(1), 0);
        	udelay(10);

		gpio_set_value(S5PC11X_MP05(1), 1);
        	udelay(10);
       
        	j--;
        }

}

static void lvds_backlight_setdata(int j)
{
        while(j>0) {
        	gpio_set_value(S5PC11X_MP05(1), 0);
        	udelay(10);

        	gpio_set_value(S5PC11X_MP05(1), 1);
        	udelay(10);
   
         	j--;
        }
}

static int lvds_backlight_on(struct platform_device *pdev)
{
	int err;
	err = gpio_request(S5PC11X_GPJ1(3), "MLCD_ON");

	if (err) {
		printk(KERN_ERR "failed to request GPJ3 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPJ1(3), 1);
      	gpio_set_value(S5PC11X_GPJ1(3), 1);
      	mdelay(500);
	return 0;
}

static int lvds_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_MP05(5), "MLCD_RST");
        if (err) {
                printk(KERN_ERR "failed to request MP0(5) for "
                        "lcd reset control\n");
                return err;
        }

        gpio_direction_output(S5PC11X_MP05(5), 1);
        mdelay(100);

        gpio_set_value(S5PC11X_MP05(5), 0);
        mdelay(10);

        gpio_set_value(S5PC11X_MP05(5), 1);
        mdelay(10);

        gpio_set_value(S5PC11X_MP04(1), 1);
        mdelay(10);

        
	err = gpio_request(S5PC11X_MP05(1), "LCD_BACKLIGHT_ON");
        if (err) {
                printk(KERN_ERR "failed to request MP05(1) for "
                        "back light enable\n");
                return err;
        }

       	lvds_backlight_start();
       	lvds_backlight_setaddress(17);
       	lvds_backlight_start();
       	lvds_backlight_setdata(2);
 
       	lvds_backlight_start();
       	lvds_backlight_setaddress(18);
       	lvds_backlight_start();
       	lvds_backlight_setdata(2);

       	lvds_backlight_start();
       	lvds_backlight_setaddress(19);
       	lvds_backlight_start();
       	lvds_backlight_setdata(4);

       	lvds_backlight_start();
       	lvds_backlight_setaddress(20);
       	lvds_backlight_start();
       	lvds_backlight_setdata(4);

       	lvds_backlight_start();
       	lvds_backlight_setaddress(21);
       	lvds_backlight_start();

	return 0;
}
static struct s3c_platform_fb lvds_data __initdata = {
	.hw_ver	= 0x60,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,

	.cfg_gpio = lvds_cfg_gpio,
	.backlight_on = lvds_backlight_on,
	.reset_lcd = lvds_reset_lcd,
};
#endif

void universal_wm8994_init(void)
{
 	int err;
	u32 temp;
	temp = __raw_readl(S5P_CLK_OUT);

	temp &= 0xFFFE0FFF; //clear bits 12~16
	temp |= (0x11 << 12); //crystall
	//temp |= (0x02 << 12); // epll
	printk("CLKOUT reg is %x\n",temp);
	 __raw_writel(temp, S5P_CLK_OUT);
	temp = __raw_readl(S5P_CLK_OUT);
	printk("CLKOUT reg is %x\n",temp);


	err = gpio_request( S5PC11X_MP03(6), "CODEC_LDO_EN");

	if (err) {
		printk(KERN_ERR "failed to request MP03(6) for "
			"codec control\n");
		return err;
	}
	udelay(50);
	gpio_direction_output( S5PC11X_MP03(6) , 1);
        gpio_set_value(S5PC11X_MP03(6), 1);
	udelay(50);
        gpio_set_value(S5PC11X_MP03(6), 0);
	udelay(50);
        gpio_set_value(S5PC11X_MP03(6), 1);



}





#if defined(CONFIG_SPI_CNTRLR_2)
static struct s3c_spi_pdata s3c_slv_pdata_2[] __initdata = {
        [0] = { /* Slave-0 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPH1(0),
                .cs_mode      = S5PC11X_GPH1_OUTPUT(0),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
};
#endif

static struct spi_board_info s3c_spi_devs[] __initdata = {
#if defined(CONFIG_SPI_CNTRLR_0)
        [0] = {
                .modalias        = "spidev", /* Test Interface */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 100000,
                /* Connected to SPI-0 as 1st Slave */
                .bus_num         = 0,
                .irq             = IRQ_SPI0,
                .chip_select     = 0,
        },
        [1] = {
                .modalias        = "spidev", /* Test Interface */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 100000,
                /* Connected to SPI-0 as 2nd Slave */
                .bus_num         = 0,
                .irq             = IRQ_SPI0,
                .chip_select     = 1,
        },
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
        [2] = {
                .modalias        = "spidev", /* Test Interface */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 100000,
                /* Connected to SPI-1 as 1st Slave */
                .bus_num         = 1,
                .irq             = IRQ_SPI1,
                .chip_select     = 0,
        },
        [3] = {
                .modalias        = "spidev", /* Test Interface */
                .mode            = SPI_MODE_0 | SPI_CS_HIGH,    /* CPOL=0, CPHA=0 & CS is Active High */
                .max_speed_hz    = 100000,
                /* Connected to SPI-1 as 3rd Slave */
                .bus_num         = 1,
                .irq             = IRQ_SPI1,
                .chip_select     = 1,
        },
#endif

#if defined(CONFIG_SPI_CNTRLR_2)
/* For MMC-SPI using GPIO BitBanging. MMC connected to SPI CNTRL 2 as slave 0. */
#if defined (CONFIG_MMC_SPI_GPIO)
#define SPI_GPIO_DEVNUM 4
#define SPI_GPIO_BUSNUM 2

        [SPI_GPIO_DEVNUM] = {
                .modalias        = "mmc_spi", /* MMC SPI */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 400000,
                /* Connected to SPI-0 as 1st Slave */
                .bus_num         = SPI_GPIO_BUSNUM,
                .chip_select     = 0,
                .controller_data = S5PC11X_GPH1(0),
        },
#else
        [4] = {
                .modalias        = "mmc_spi", 	/* MMC SPI */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 400000,
                /* Connected to SPI-2 as 1st Slave */
                .bus_num         = 2,
                .irq             = IRQ_SPI2,
                .chip_select     = 0,
        },
#endif
#endif	
};

#if defined(CONFIG_MMC_SPI_GPIO)
#define SPI_GPIO_ID 2
struct spi_gpio_platform_data s3c_spigpio_pdata = {
        .sck = S5PC11X_GPG2(0),
        .miso = S5PC11X_GPG2(2),
        .mosi = S5PC11X_GPG2(3),
        .num_chipselect = 1,
};

/* Generic GPIO Bitbanging contoller */
struct platform_device s3c_device_spi_bitbang = {
        .name           = "spi_gpio",
        .id             = SPI_GPIO_ID,
        .dev            = {
                .platform_data = &s3c_spigpio_pdata,
        }
};
#endif

// i2c gpio for wm8994

static	struct	i2c_gpio_platform_data	i2c4_platdata = {
	.sda_pin		= AP_I2C_SDA,
	.scl_pin		= AP_I2C_SCL,
	.udelay			= 6,	/* 250KHz */		
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c4 = {
	.name				= "i2c-gpio",
	.id					= 4,
	.dev.platform_data	= &i2c4_platdata,
};




static struct platform_device *smdkc110_devices[] __initdata = {
	&s3c_device_fb,
	&s3c_device_mfc,
        &s3c_device_hsmmc2,

#ifdef CONFIG_S3C2410_WATCHDOG
	&s3c_device_wdt,
#endif
#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
    	&pmem_adsp_device,
#endif
#ifdef CONFIG_RTC_DRV_S3C
	&s3c_device_rtc,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],

#endif
#ifdef CONFIG_SPI_CNTRLR_0
        &s3c_device_spi0,
#endif
#ifdef CONFIG_SPI_CNTRLR_1
        &s3c_device_spi1,
#endif
#ifdef CONFIG_MMC_SPI_GPIO	//for mmc-spi gpio bitbanging
      	&s3c_device_spi_bitbang,
#elif defined(CONFIG_SPI_CNTRLR_2)
	&s3c_device_spi2,
#endif
	&s3c_device_usbgadget,
	&s3c_device_keypad,
	&s3c_device_qtts,
	&s3c_device_adc,
	&s5p_device_tvout,
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_csis,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_i2c2,
	//&s3c_device_i2c3,
	&s3c_device_ipc,
	&s3c_device_jpeg,
	&s3c_device_i2c4,
};

static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000,
	.presc 			= 49,
	.oversampling_shift	= 2,
	.resol_bit 		= 12,
	.s3c_adc_con		= ADC_TYPE_2,
};

static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
        /* s5pc100 supports 12-bit resolution */
        .delay  = 10000,
        .presc  = 49,
        .resolution = 12,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },
};

	

/*
 * External camera reset
 * Because the most of cameras take i2c bus signal, so that
 * you have to reset at the boot time for other i2c slave devices.
 * This function also called at fimc_init_camera()
 * Do optimization for cameras on your platform.
*/
static int smdkc110_cam0_power(int onoff)
{
	/* Camera A */
	gpio_request(S5PC11X_GPH0(2), "GPH0");
	s3c_gpio_setpull(S5PC11X_GPH0(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PC11X_GPH0(2), 0);
	gpio_direction_output(S5PC11X_GPH0(2), 1);
	gpio_free(S5PC11X_GPH0(2));

	return 0;
}

static int smdkc110_cam1_power(int onoff)
{
	/* Camera B */
	gpio_request(S5PC11X_GPH0(3), "GPH0");
	s3c_gpio_setpull(S5PC11X_GPH0(3), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PC11X_GPH0(3), 0);
	gpio_direction_output(S5PC11X_GPH0(3), 1);
	gpio_free(S5PC11X_GPH0(3));

	return 0;
}

static int smdkc110_mipi_cam_power(int onoff)
{
	gpio_request(S5PC11X_GPH0(3), "GPH0");
	s3c_gpio_setpull(S5PC11X_GPH0(3), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PC11X_GPH0(3), 0);
	gpio_direction_output(S5PC11X_GPH0(3), 1);
	gpio_free(S5PC11X_GPH0(3));

	return 0;
}

/*
 * Guide for Camera Configuration for SMDKC110
 * ITU channel must be set as A or B
 * ITU CAM CH A: S5K3BA only
 * ITU CAM CH B: one of S5K3BA and S5K4BA
 * MIPI: one of S5K4EA and S5K6AA
 *
 * NOTE1: if the S5K4EA is enabled, all other cameras must be disabled
 * NOTE2: currently, only 1 MIPI camera must be enabled
 * NOTE3: it is possible to use both one ITU cam and one MIPI cam except for S5K4EA case
 * 
*/
#undef CAM_ITU_CH_A
#undef S5K3BA_ENABLED
#define S5K4BA_ENABLED
#undef S5K4EA_ENABLED
#define S5K6AA_ENABLED
#undef S5K6AA_ENABLED

/* External camera module setting */
/* 2 ITU Cameras */
#ifdef S5K3BA_ENABLED
static struct s5k3ba_platform_data s5k3ba_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_VYUY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  __initdata s5k3ba_i2c_info = {
	I2C_BOARD_INFO("S5K3BA", 0x2d),
	.platform_data = &s5k3ba_plat,
};

static struct s3c_platform_camera __initdata s5k3ba = {
#ifdef CAM_ITU_CH_A
	.id		= CAMERA_PAR_A,
#else
	.id		= CAMERA_PAR_B,
#endif
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CRYCBY,
	.i2c_busnum	= 0,
	.info		= &s5k3ba_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_VYUY,
	.srclk_name	= "mout_epll",
#ifdef CAM_ITU_CH_A
	.clk_name	= "sclk_cam0",
#else
	.clk_name	= "sclk_cam1",
#endif
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
#ifdef CAM_ITU_CH_A
	.cam_power	= smdkc110_cam0_power,
#else
	.cam_power	= smdkc110_cam1_power,
#endif
};
#endif

#ifdef S5K4BA_ENABLED
static struct s5k4ba_platform_data s5k4ba_plat = {
	.default_width = 800,
	.default_height = 600,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 44000000,
	.is_mipi = 0,
};

static struct i2c_board_info  __initdata s5k4ba_i2c_info = {
	I2C_BOARD_INFO("S5K4BA", 0x2d),
	.platform_data = &s5k4ba_plat,
};

static struct s3c_platform_camera __initdata s5k4ba = {
#ifdef CAM_ITU_CH_A
	.id		= CAMERA_PAR_A,
#else
	.id		= CAMERA_PAR_B,
#endif
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5k4ba_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "mout_mpll",
#ifdef CAM_ITU_CH_A
	.clk_name	= "sclk_cam0",
#else
	.clk_name	= "sclk_cam1",
#endif
	.clk_rate	= 44000000,
	.line_length	= 1920,
	.width		= 800,
	.height		= 600,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 800,
		.height	= 600,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
#ifdef CAM_ITU_CH_A
	.cam_power	= smdkc110_cam0_power,
#else
	.cam_power	= smdkc110_cam1_power,
#endif
};
#endif

/* 2 MIPI Cameras */
#ifdef S5K4EA_ENABLED
static struct s5k6aa_platform_data s5k4ea_plat = {
	.default_width = 1920,
	.default_height = 1080,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info  __initdata s5k4ea_i2c_info = {
	I2C_BOARD_INFO("S5K4EA", 0x2d),
	.platform_data = &s5k4ea_plat,
};

static struct s3c_platform_camera __initdata s5k4ea = {
	.id		= CAMERA_CSI_C,
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5k4ea_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "mout_epll",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 1920,
	.height		= 1080,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1920,
		.height	= 1080,
	},

	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= smdkc110_mipi_cam_power,
};
#endif

#ifdef S5K6AA_ENABLED
static struct s5k6aa_platform_data s5k6aa_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info  __initdata s5k6aa_i2c_info = {
	I2C_BOARD_INFO("S5K6AA", 0x3c),
	.platform_data = &s5k6aa_plat,
};

static struct s3c_platform_camera __initdata s5k6aa = {
	.id		= CAMERA_CSI_C,
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5k6aa_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "mout_epll",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	/* default resol for preview kind of thing */
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= smdkc110_mipi_cam_power,
};
#endif

/* Interface setting */
static struct s3c_platform_fimc __initdata fimc_plat = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_fimc",
	.clk_rate	= 166000000,
#if defined(S5K4EA_ENABLED) || defined(S5K6AA_ENABLED)
	.default_cam	= CAMERA_CSI_C,
#else

#ifdef CAM_ITU_CH_A
	.default_cam	= CAMERA_PAR_A,
#else
	.default_cam	= CAMERA_PAR_B,
#endif

#endif
	.camera		= {
#ifdef S5K3BA_ENABLED
		&s5k3ba,
#endif
#ifdef S5K4BA_ENABLED
		&s5k4ba,
#endif
#ifdef S5K4EA_ENABLED
		&s5k4ea,
#endif
#ifdef S5K6AA_ENABLED
		&s5k6aa,
#endif
	}
};


void s3c_config_sleep_gpio(void)
{

}

EXPORT_SYMBOL(s3c_config_sleep_gpio);


static void __init smdkc110_machine_init(void)
{
	/* i2c */
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(NULL);
	s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

        /* spi */
#if defined(CONFIG_SPI_CNTRLR_0)
        s3cspi_set_slaves(BUSNUM(0), ARRAY_SIZE(s3c_slv_pdata_0), s3c_slv_pdata_0);
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
        s3cspi_set_slaves(BUSNUM(1), ARRAY_SIZE(s3c_slv_pdata_1), s3c_slv_pdata_1);
#endif
#if defined(CONFIG_SPI_CNTRLR_2)
        s3cspi_set_slaves(BUSNUM(2), ARRAY_SIZE(s3c_slv_pdata_2), s3c_slv_pdata_2);
#endif
        spi_register_board_info(s3c_spi_devs, ARRAY_SIZE(s3c_spi_devs));

	universal_wm8994_init();
	universal_sdhci2_set_platdata();



#ifdef CONFIG_FB_S3C_LVDS
	s3cfb_set_platdata(&lvds_data);
#endif
	/* pmem */	
#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();	
#endif
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);

	s3c_csis_set_platdata(NULL);
	smdkc110_cam0_power(1);
	smdkc110_cam1_power(1);
	smdkc110_mipi_cam_power(1);

	/* ipc */
	s3c_ipc_set_platdata(NULL);
	
	platform_add_devices(smdkc110_devices, ARRAY_SIZE(smdkc110_devices));

#if defined(CONFIG_PM)
	s5pc11x_pm_init();
#endif

#if defined(CONFIG_HAVE_PWM)
	smdk_backlight_register();
#endif

	s3c_gpio_cfgpin( AP_I2C_SCL_28V, 1 );
	s3c_gpio_setpull( AP_I2C_SCL_28V, S3C_GPIO_PULL_UP); 
	s3c_gpio_cfgpin( AP_I2C_SDA_28V, 1 );
	s3c_gpio_setpull( AP_I2C_SDA_28V, S3C_GPIO_PULL_UP); 

	sec_class = class_create(THIS_MODULE, "sec");

}




#if defined(CONFIG_KEYPAD_S3C) || defined (CONFIG_KEYPAD_S3C_MODULE)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	end = S5PC11X_GPH3(rows);

	/* Set all the necessary GPH2 pins to special-function 0 */
	for (gpio = S5PC11X_GPH3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		//s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	}

	end = S5PC11X_GPJ4(4);
	for (gpio = S5PC11X_GPJ2(7); gpio <= end; gpio++) {
		//s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_DOWN);
		
		s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
		//s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		//s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s3c_gpio_setpin(gpio, 0);
	}
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);

#endif

MACHINE_START(SMDKC110, "SMDKC110")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5PC11X_PA_SDRAM + 0x100,
	.fixup		= smdkc110_fixup,
	.init_irq	= s5pc110_init_irq,
	.map_io		= smdkc110_map_io,
	.init_machine	= smdkc110_machine_init,
	#ifndef CONFIG_HIGH_RES_TIMERS
	.timer		= &s5pc11x_timer,
	#else
	.timer		=&sec_timer,
	#endif
MACHINE_END

