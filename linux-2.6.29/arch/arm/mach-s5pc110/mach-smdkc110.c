/* linux/arch/arm/mach-s5pc110/mach-smdkc110.c
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


#ifdef CONFIG_SMDKC110_BOARD
#include <linux/regulator/max8698.h>
#endif
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
#ifdef CONFIG_USB_SUPPORT
#include <plat/regs-otg.h>
#include <plat/pll.h>
#include <linux/usb/ch9.h>

/* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
#define S3C_USB_CLKSRC	1
#define OTGH_PHY_CLK_VALUE      (0x22)  /* UTMI Interface, otg_phy input clk 12Mhz Oscillator */
#endif

#if defined(CONFIG_PM)
#include <plat/pm.h>
#endif

#define UCON S3C2410_UCON_DEFAULT | S3C_UCON_PCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

#ifndef CONFIG_HIGH_RES_TIMERS
extern struct sys_timer s5pc11x_timer;
#else
extern struct sys_timer sec_timer;
#endif

extern void s5pc11x_reserve_bootmem(void);

static struct s3c24xx_uart_clksrc smdkc110_serial_clocks[] = {
#if defined(CONFIG_SERIAL_S5PC11X_HSUART)
/* HS-UART Clock using SCLK */
	[0] = {
		.name		= "uclk1",
		.divisor	= 1,
		.min_baud	= 0,
		.max_baud	= 0,
	},
#else
	[0] = {
		.name		= "pclk",
		.divisor	= 1,
		.min_baud	= 0,
		.max_baud	= 0,
	},
#endif
};

static struct s3c2410_uartcfg smdkc110_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
		.clocks      = smdkc110_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdkc110_serial_clocks),
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
		.clocks      = smdkc110_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdkc110_serial_clocks),
	},
	[2] = {
		.hwport      = 2,
		.flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
		.clocks      = smdkc110_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdkc110_serial_clocks),
	},
	[3] = {
		.hwport      = 3,
		.flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
		.clocks      = smdkc110_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdkc110_serial_clocks),
	},
};


#ifdef CONFIG_SMDKC110_BOARD
/* PMIC */
static struct regulator_consumer_supply dcdc1_consumers[] = {
	{
		.supply		= "vddarm",
	},
};

static struct regulator_init_data max8698_dcdc1_data = {
	.constraints	= {
		.name		= "VCC_ARM",
		.min_uV		=  750000,
		.max_uV		= 1500000,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= ARRAY_SIZE(dcdc1_consumers),
	.consumer_supplies	= dcdc1_consumers,
};

static struct regulator_init_data max8698_dcdc2_data = {
	.constraints	= {
		.name		= "VCC_INTERNAL",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
	},
};

static struct max8698_subdev_data universal_regulators[] = {
	{ MAX8698_DCDC1, &max8698_dcdc1_data },
};


static struct max8698_platform_data max8698_platform_data = {
	.num_regulators	= ARRAY_SIZE(universal_regulators),
	.regulators	= universal_regulators,
};

/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
	{
		/* The address is 0xCC used since SRAD = 0 */
		I2C_BOARD_INFO("max8698", (0xCC >> 1)),
		.platform_data = &max8698_platform_data,
	},
};
struct platform_device s3c_device_8698consumer = {
        .name             = "max8698-consumer",
        .id               = 0,
  	.dev = { .platform_data = &max8698_platform_data },
};

#endif



#ifdef CONFIG_ANDROID_PMEM
// pmem 
static struct android_pmem_platform_data pmem_pdata = {
   .name = "pmem",
	 .no_allocator = 1,
   .cached = 1,
   .start = 0, // will be set during proving pmem driver.
   .size = 0 // will be set during proving pmem driver.
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
   .name = "pmem_gpu1",
   .no_allocator = 1,
   .cached = 1,
   .buffered = 1,
   .start = 0,
   .size = 0,
};

static struct android_pmem_platform_data pmem_adsp_pdata = {
   .name = "pmem_adsp",
   .no_allocator = 1,
   .cached = 1,
   .buffered = 1,
   .start = 0,
   .size = 0,
};

static struct platform_device pmem_device = {
   .name = "android_pmem",
   .id = 0,
   .dev = { .platform_data = &pmem_pdata },
};

#if! defined (CONFIG_PWM_TIMER)
static struct platform_device sec_device_backlight = {
        .name           = "smdk-backlight",
        .id            = -1,
};
#endif

static struct platform_device pmem_gpu1_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &pmem_gpu1_pdata },
};

#if defined(CONFIG_TOUCHSCREEN_QT602240)
static struct platform_device sec_device_ts = {
	.name = "qt602240-ts",
	.id = -1,
};
#endif

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &pmem_adsp_pdata },
};
#endif

#ifdef CONFIG_FB_S3C_LTE480WV
static void lte480wv_cfg_gpio(struct platform_device *pdev)
{
	int i;

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
	writel(0xffffffff, S5PC11X_GPF0_BASE + 0xc);
	writel(0xffffffff, S5PC11X_GPF1_BASE + 0xc);
	writel(0xffffffff, S5PC11X_GPF2_BASE + 0xc);
	writel(0x000000ff, S5PC11X_GPF3_BASE + 0xc);
}

static int lte480wv_backlight_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPD0(3), 1);
	gpio_free(S5PC11X_GPD0(3));

	return 0;
}

static int lte480wv_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPH0(6), "GPH0");
	if (err) {
		printk(KERN_ERR "failed to request GPH0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPH0(6), 1);
	mdelay(100);

	gpio_set_value(S5PC11X_GPH0(6), 0);
	mdelay(10);

	gpio_set_value(S5PC11X_GPH0(6), 1);
	mdelay(10);

	gpio_free(S5PC11X_GPH0(6));

	return 0;
}

static struct s3c_platform_fb lte480wv_data __initdata = {
	.hw_ver	= 0x60,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,

	.cfg_gpio = lte480wv_cfg_gpio,
	.backlight_on = lte480wv_backlight_on,
	.reset_lcd = lte480wv_reset_lcd,
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




#ifdef CONFIG_FB_S3C_TL2796
#if defined(CONFIG_FB_S3C_TL2796_DUAL)
extern void s5pc110_sublcd_cfg_gpio(void);
extern int s5pc110_sublcd_on(void);
extern int s5pc110_sublcd_reset(void);
#endif

static void tl2796_cfg_gpio(struct platform_device *pdev)
{
	int i;

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

#if 1
	s3c_gpio_cfgpin(S5PC11X_GPB(4), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(5), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(6), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(7), S3C_GPIO_SFN(1));
#else
	/* why the followings do not work? */
	gpio_request(S5PC11X_GPB(4), "GPB");
	gpio_request(S5PC11X_GPB(5), "GPB");
	gpio_request(S5PC11X_GPB(6), "GPB");
	gpio_request(S5PC11X_GPB(7), "GPB");
	gpio_direction_output(S5PC11X_GPB(4), 0);
	gpio_direction_output(S5PC11X_GPB(5), 0);
	gpio_direction_output(S5PC11X_GPB(6), 0);
	gpio_direction_output(S5PC11X_GPB(7), 0);
#endif
	s3c_gpio_setpull(S5PC11X_GPB(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(6), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(7), S3C_GPIO_PULL_NONE);

	gpio_request(S5PC11X_GPH0(5), "GPH0");
	gpio_direction_output(S5PC11X_GPH0(5), 1);
}

static int tl2796_backlight_on(struct platform_device *pdev)
{

	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPD0(3), 1);
	gpio_free(S5PC11X_GPD0(3));

	return 0;
}

static int tl2796_reset_lcd(struct platform_device *pdev)
{

	int err;

	err = gpio_request(S5PC11X_GPH0(6), "GPH0");
	if (err) {
		printk(KERN_ERR "failed to request GPH0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPH0(6), 1);
	mdelay(100);

	gpio_set_value(S5PC11X_GPH0(6), 0);
	mdelay(10);

	gpio_set_value(S5PC11X_GPH0(6), 1);
	mdelay(10);

	gpio_free(S5PC11X_GPH0(6));


	return 0;
}


static struct s3c_platform_fb tl2796_data __initdata = {
	.hw_ver	= 0x60,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,

	.cfg_gpio = tl2796_cfg_gpio,
	.backlight_on = tl2796_backlight_on,
	.reset_lcd = tl2796_reset_lcd,
};

#define LCD_BUS_NUM 	3

#define DISPLAY_CS	S5PC11X_GPB(5)
#define DISPLAY_CLK	S5PC11X_GPB(4)
#define DISPLAY_SI	S5PC11X_GPB(7)


static struct spi_board_info spi_board_info[] __initdata = {
    	{
	    	.modalias	= "tl2796",
		.platform_data	= NULL,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,
	.num_chipselect	= 1,
};

static struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tl2796_spi_gpio_data,
	},
};
#endif

#if defined(CONFIG_SPI_CNTRLR_0) || defined(CONFIG_SPI_CNTRLR_1) || defined(CONFIG_SPI_CNTRLR_2)
static void s3c_cs_suspend(int pin, pm_message_t pm)
{
        /* Whatever need to be done */
}

static void s3c_cs_resume(int pin)
{
        /* Whatever need to be done */
}

static void s3c_cs_set(int pin, int lvl)
{
        if(lvl == CS_HIGH)
           s3c_gpio_setpin(pin, 1);
        else
           s3c_gpio_setpin(pin, 0);
}

static void s3c_cs_config(int pin, int mode, int pull)
{
        s3c_gpio_cfgpin(pin, mode);

        if(pull == CS_HIGH)
           s3c_gpio_setpull(pin, S3C_GPIO_PULL_UP);
        else
           s3c_gpio_setpull(pin, S3C_GPIO_PULL_DOWN);
}
#endif

#if defined(CONFIG_SPI_CNTRLR_0)
static struct s3c_spi_pdata s3c_slv_pdata_0[] __initdata = {
        [0] = { /* Slave-0 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPB(1),
                .cs_mode      = S5PC11X_GPB_OUTPUT(1),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
        [1] = { /* Slave-1 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPA1(1),
                .cs_mode      = S5PC11X_GPA1_OUTPUT(1),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
};
#endif

#if defined(CONFIG_SPI_CNTRLR_1)
static struct s3c_spi_pdata s3c_slv_pdata_1[] __initdata = {
        [0] = { /* Slave-0 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPB(5),
                .cs_mode      = S5PC11X_GPB_OUTPUT(5),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
        [1] = { /* Slave-1 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPA1(3),
                .cs_mode      = S5PC11X_GPA1_OUTPUT(3),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
};
#endif

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


struct map_desc smdkc110_iodesc[] = {};

static struct platform_device *smdkc110_devices[] __initdata = {
	&s3c_device_fb,
	&s3c_device_dm9000,
	&s3c_device_mfc,
#ifdef CONFIG_FB_S3C_TL2796
	&s3c_device_spi_gpio,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
        &s3c_device_hsmmc0,
#endif        

#ifdef CONFIG_S3C_DEV_HSMMC1
        &s3c_device_hsmmc1,
#endif        
        
#ifdef CONFIG_S3C_DEV_HSMMC2
        &s3c_device_hsmmc2,
#endif        
        
#ifdef CONFIG_S3C_DEV_HSMMC3
        &s3c_device_hsmmc3,        
#endif

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
#if defined(CONFIG_TOUCHSCREEN_QT602240)
	&sec_device_ts,
#else
	&s3c_device_ts,
#endif
	&s3c_device_adc,
	&s3c_device_cfcon,
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
#ifdef CONFIG_SMDKC110_BOARD
	&s3c_device_8698consumer,
#endif
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
#define S5K4BA_ENABLED

#undef S5K3BA_ENABLED
#undef S5K4EA_ENABLED
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

#if defined(CONFIG_HAVE_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
	.pwm_id         = 3,
	.max_brightness = 255,
	.dft_brightness = 255,
	.pwm_period_ns  = 78770,
};

static struct platform_device smdk_backlight_device = {
	.name           = "pwm-backlight",
	.dev            = {
		.parent = &s3c_device_timer[3].dev,
		.platform_data = &smdk_backlight_data,
	},
};
static void __init smdk_backlight_register(void)
{
	int ret = platform_device_register(&smdk_backlight_device);
	if (ret)
		printk(KERN_ERR "smdk: failed to register backlight device: %d\n", ret);
}
#else
#define smdk_backlight_register()	do { } while (0)
#endif

static void __init smdkc110_map_io(void)
{
	s3c_device_nand.name = "s5pc100-nand";
	s5pc11x_init_io(smdkc110_iodesc, ARRAY_SIZE(smdkc110_iodesc));
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdkc110_uartcfgs, ARRAY_SIZE(smdkc110_uartcfgs));
#if defined(CONFIG_SERIAL_S5PC11X_HSUART)
	/* got to have a high enough uart source clock for higher speeds */
	writel((readl(S5P_CLK_DIV4) & ~(0xffff0000)) | 0x44440000, S5P_CLK_DIV4);
#endif
	s5pc11x_reserve_bootmem();
}

static void __init smdkc110_dm9000_set(void)
{

}

// pmem
static void __init android_pmem_set_platdata(void)
{
	#ifdef CONFIG_ANDROID_PMEM
	pmem_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM);
	pmem_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM);

	pmem_gpu1_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM_GPU1);
	pmem_gpu1_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM_GPU1);	

	pmem_adsp_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM_ADSP);
	pmem_adsp_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM_ADSP);	
	#endif
}

void s3c_config_sleep_gpio(void)
{

}

EXPORT_SYMBOL(s3c_config_sleep_gpio);






static void __init smdkc110_machine_init(void)
{
	smdkc110_dm9000_set();

	/* i2c */
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(NULL);
	//s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#ifdef CONFIG_SMDKC110_BOARD
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
#endif


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
#ifdef CONFIG_FB_S3C_LTE480WV
	s3cfb_set_platdata(&lte480wv_data);
#endif
#if defined (CONFIG_TOUCHSCREEN_S3C)
	s3c_ts_set_platdata(&s3c_ts_platform);
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif


#ifdef CONFIG_FB_S3C_TL2796
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&tl2796_data);
#endif
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


}

static void __init smdkc110_fixup(struct machine_desc *desc,
					struct tag *tags, char **cmdline,
					struct meminfo *mi)
{
#if defined(CONFIG_S5PC110_H_TYPE)
	mi->bank[0].start = 0x30000000;
	mi->bank[0].size = 128 * SZ_1M;
	mi->bank[0].node = 0;
#elif defined(CONFIG_S5PC110_AC_TYPE)
	mi->bank[0].start = 0x30000000;
	mi->bank[0].size = 80 * SZ_1M;
	mi->bank[0].node = 0;
#endif

	mi->bank[1].start = 0x40000000;
	mi->bank[1].size = 128 * SZ_1M;
	mi->bank[1].node = 1;

	mi->nr_banks = 2;

}

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


#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void) {
	writel(readl(S5P_USB_PHY_CONTROL)|(0x1<<0), S5P_USB_PHY_CONTROL); /*USB PHY0 Enable */
	writel((readl(S3C_USBOTG_PHYPWR)&~(0x3<<3)&~(0x1<<0))|(0x1<<5), S3C_USBOTG_PHYPWR);
	writel((readl(S3C_USBOTG_PHYCLK)&~(0x5<<2))|(0x3<<0), S3C_USBOTG_PHYCLK);
	writel((readl(S3C_USBOTG_RSTCON)&~(0x3<<1))|(0x1<<0), S3C_USBOTG_RSTCON);
	udelay(10);
	writel(readl(S3C_USBOTG_RSTCON)&~(0x7<<0), S3C_USBOTG_RSTCON);
	udelay(10);

}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) {
	writel(readl(S3C_USBOTG_PHYPWR)|(0x3<<3), S3C_USBOTG_PHYPWR);
	writel(readl(S5P_USB_PHY_CONTROL)&~(1<<0), S5P_USB_PHY_CONTROL);

}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_clk_en(void) {

}

EXPORT_SYMBOL(usb_host_clk_en);
#endif

#if defined(CONFIG_RTC_DRV_S3C)
/* RTC common Function for samsung APs*/
unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val)
{
        writeb(val, base + offset);

        return 0;
}

unsigned int s3c_rtc_read_alarm_status(void __iomem *base)
{
        return 1;
}

void s3c_rtc_set_pie(void __iomem *base, uint to)
{
        unsigned int tmp;

        tmp = readw(base + S3C2410_RTCCON) & ~S3C_RTCCON_TICEN;

        if (to)
                tmp |= S3C_RTCCON_TICEN;

        writew(tmp, base + S3C2410_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
        unsigned int tmp;

        tmp = readw(base + S3C2410_RTCCON) & (S3C_RTCCON_TICEN | S3C2410_RTCCON_RTCEN );
        writew(tmp, base + S3C2410_RTCCON);
        s3c_freq = freq;
        tmp = (32768 / freq)-1;
        writel(tmp, base + S3C2410_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev,void __iomem *base, int en)
{
        unsigned int tmp;

        if (!en) {
                tmp = readw(base + S3C2410_RTCCON);
                writew(tmp & ~ (S3C2410_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C2410_RTCCON);
        } else {
                /* re-enable the device, and check it is ok */
                if ((readw(base+S3C2410_RTCCON) & S3C2410_RTCCON_RTCEN) == 0){
                        dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

                        tmp = readw(base + S3C2410_RTCCON);
                        writew(tmp|S3C2410_RTCCON_RTCEN, base+S3C2410_RTCCON);
                }

                if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CNTSEL)){
                        dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

                        tmp = readw(base + S3C2410_RTCCON);
                        writew(tmp& ~S3C2410_RTCCON_CNTSEL, base+S3C2410_RTCCON);
                }

                if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CLKRST)){
                        dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

                        tmp = readw(base + S3C2410_RTCCON);
                        writew(tmp & ~S3C2410_RTCCON_CLKRST, base+S3C2410_RTCCON);
                }
        }
}
#endif

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
	}

	end = S5PC11X_GPH2(columns);

	/* Set all the necessary GPH pins to special-function 0 */
	for (gpio = S5PC11X_GPH2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	}
}

//void	s5pc11x_config_gpio_table ( int array_size, int **gpio_table

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	switch(port)
	{
	case 0:
		s3c_gpio_cfgpin(S5PC11X_GPA0(0), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA0(0), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5PC11X_GPA0(1), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA0(1), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5PC11X_GPA0(2), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA0(2), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5PC11X_GPA0(3), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA0(3), S3C_GPIO_PULL_NONE);
		break;
	case 1:
		s3c_gpio_cfgpin(S5PC11X_GPA0(4), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA0(4), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5PC11X_GPA0(5), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA0(5), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5PC11X_GPA0(6), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA0(6), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5PC11X_GPA0(7), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA0(7), S3C_GPIO_PULL_NONE);
		break;
	case 2:
		s3c_gpio_cfgpin(S5PC11X_GPA1(0), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA1(0), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5PC11X_GPA1(1), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA1(1), S3C_GPIO_PULL_NONE);
		break;
	case 3:
		s3c_gpio_cfgpin(S5PC11X_GPA1(2), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA1(2), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5PC11X_GPA1(3), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPA1(3), S3C_GPIO_PULL_NONE);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);

