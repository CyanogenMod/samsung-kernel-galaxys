/* linux/arch/arm/mach-s5pc100/mach-smdkc100.c
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
#include <linux/delay.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

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
#include <plat/fb.h>
#include <plat/csis.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s5pc100.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-k0.h>
#include <plat/gpio-bank-a1.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-bank-d.h>
#include <plat/regs-clock.h>
#include <plat/spi.h>

#include <linux/bootmem.h>
#include <linux/android_pmem.h>
#include <plat/media.h>
#include <linux/version.h>



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

#define UCON S3C2410_UCON_DEFAULT | S3C2410_UCON_UCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

extern struct sys_timer s5pc1xx_timer;
extern void s5pc1xx_reserve_bootmem(void);

#if defined(CONFIG_SPI_CNTRLR_0) || defined(CONFIG_SPI_CNTRLR_1)
static void sam_cs_suspend(int pin, pm_message_t pm)
{
	/* Whatever need to be done */
}

static void sam_cs_resume(int pin)
{
	/* Whatever need to be done */
}

static void sam_cs_set(int pin, int lvl)
{
	if(lvl == CS_HIGH)
	   s3c_gpio_setpin(pin, 1);
	else
	   s3c_gpio_setpin(pin, 0);
}

static void sam_cs_config(int pin, int mode, int pull)
{
	s3c_gpio_cfgpin(pin, mode);

	if(pull == CS_HIGH)
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_UP);
	else
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_DOWN);
}
#endif

#if defined(CONFIG_SPI_CNTRLR_0)
static struct sam_spi_pdata sam_slv_pdata_0[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5PC1XX_GPB(3),
		.cs_mode      = S5PC1XX_GPB_OUTPUT(3),
		.cs_set       = sam_cs_set,
		.cs_config    = sam_cs_config,
		.cs_suspend   = sam_cs_suspend,
		.cs_resume    = sam_cs_resume,
	},
	[1] = {	/* Slave-1 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5PC1XX_GPA1(2),
		.cs_mode      = S5PC1XX_GPA1_OUTPUT(2),
		.cs_set       = sam_cs_set,
		.cs_config    = sam_cs_config,
		.cs_suspend   = sam_cs_suspend,
		.cs_resume    = sam_cs_resume,
	},
};
#endif

#if defined(CONFIG_SPI_CNTRLR_1)
static struct sam_spi_pdata sam_slv_pdata_1[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5PC1XX_GPB(7),
		.cs_mode      = S5PC1XX_GPB_OUTPUT(7),
		.cs_set       = sam_cs_set,
		.cs_config    = sam_cs_config,
		.cs_suspend   = sam_cs_suspend,
		.cs_resume    = sam_cs_resume,
	},
	[1] = {	/* Slave-1 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5PC1XX_GPA1(3),
		.cs_mode      = S5PC1XX_GPA1_OUTPUT(3),
		.cs_set       = sam_cs_set,
		.cs_config    = sam_cs_config,
		.cs_suspend   = sam_cs_suspend,
		.cs_resume    = sam_cs_resume,
	},
	[2] = {	/* Slave-2 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5PC1XX_GPD(6),
		.cs_mode      = S5PC1XX_GPD_OUTPUT(6),
		.cs_set       = sam_cs_set,
		.cs_config    = sam_cs_config,
		.cs_suspend   = sam_cs_suspend,
		.cs_resume    = sam_cs_resume,
	},
};
#endif

static struct spi_board_info sam_spi_devs[] __initdata = {
#if defined(CONFIG_SPI_CNTRLR_0)
	[0] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-0 as 1st Slave */
		.bus_num	 = 0,
		.irq		 = IRQ_SPI0,
		.chip_select	 = 0,
	},
	[1] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-0 as 2nd Slave */
		.bus_num	 = 0,
		.irq		 = IRQ_SPI0,
		.chip_select	 = 1,
	},
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	[2] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 1st Slave */
		.bus_num	 = 1,
		.irq		 = IRQ_SPI1,
		.chip_select	 = 0,
	},
	[3] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 2nd Slave */
		.bus_num	 = 1,
		.irq		 = IRQ_SPI1,
		.chip_select	 = 1,
	},
	[4] = {
		.modalias	 = "mmc_spi", /* MMC SPI */
		.mode		 = SPI_MODE_0 | SPI_CS_HIGH,	/* CPOL=0, CPHA=0 & CS is Active High */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 3rd Slave */
		.bus_num	 = 1,
		.irq		 = IRQ_SPI1,
		.chip_select	 = 2,
	},
#endif
};

static struct s3c24xx_uart_clksrc smdkc100_serial_clocks[] = {
#if defined(CONFIG_SERIAL_S5PC1XX_HSUART)
/* HS-UART Clock using SCLK */
        [0] = {
                .name           = "uclk1",
                .divisor        = 1,
                .min_baud       = 0,
                .max_baud       = 0,
        },
#else
        [0] = {
                .name           = "pclk",
                .divisor        = 1,
                .min_baud       = 0,
                .max_baud       = 0,
        },
#endif
};

static struct s3c2410_uartcfg smdkc100_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = smdkc100_serial_clocks,
                .clocks_size = ARRAY_SIZE(smdkc100_serial_clocks),
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = smdkc100_serial_clocks,
                .clocks_size = ARRAY_SIZE(smdkc100_serial_clocks),
	},
        [2] = {
                .hwport      = 2,
                .flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = smdkc100_serial_clocks,
                .clocks_size = ARRAY_SIZE(smdkc100_serial_clocks),
        },
        [3] = {
                .hwport      = 3,
                .flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = smdkc100_serial_clocks,
                .clocks_size = ARRAY_SIZE(smdkc100_serial_clocks),
        },
};

#ifdef CONFIG_ANDROID_PMEM
// sy82.yoon - pmem initialize (2009.05.18)
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

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &pmem_adsp_pdata },
};
#endif

struct map_desc smdkc100_iodesc[] = {};

static struct platform_device *smdkc100_devices[] __initdata = {
	//&s3c_device_fb,
	&s3c_device_lcd,
        &s3c_device_nand,
#if defined(CONFIG_MTD_ONENAND)
        &s3c_device_onenand,
#endif
	&s3c_device_keypad,
	&s3c_device_ts,
	&s3c_device_adc,
        &s3c_device_rtc,
	&s3c_device_smc911x,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
        &s3c_device_usb,
	&s3c_device_usbgadget,
	&s3c_device_usb_otghcd,
        &s3c_device_hsmmc0,
        &s3c_device_hsmmc1,
#ifdef CONFIG_SPI_CNTRLR_0
        &s3c_device_spi0,
#endif
#ifdef CONFIG_SPI_CNTRLR_1
        &s3c_device_spi1,
#endif
#ifdef CONFIG_SPI_CNTRLR_2
	&s3c_device_spi2,
#endif
	&s3c_device_mfc,
	&s3c_device_jpeg,
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_ac97,
	&s3c_device_wdt,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	&s3c_device_g3d,
#endif
	&s3c_device_g2d,
	&s3c_device_rotator,
	&s3c_device_csis,
	&s5p_device_tvout,//mkh uncommented
#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
    &pmem_adsp_device,
#endif
#if defined(CONFIG_HAVE_PWM)
        &s3c_device_timer[0],
        &s3c_device_timer[1],
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

#if defined(CONFIG_HAVE_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
        .pwm_id         = 0,
        .max_brightness = 255,
        .dft_brightness = 255,
        .pwm_period_ns  = 78770,
};

static struct platform_device smdk_backlight_device = {
        .name           = "pwm-backlight",
        .dev            = {
                .parent = &s3c_device_timer[0].dev,
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
#define smdk_backlight_register()       do { } while (0)
#endif

static void __init smdkc100_map_io(void)
{
	s3c_device_nand.name = "s5pc100-nand";
	s5pc1xx_init_io(smdkc100_iodesc, ARRAY_SIZE(smdkc100_iodesc));
	s3c24xx_init_clocks(0);
	s3c24xx_init_uarts(smdkc100_uartcfgs, ARRAY_SIZE(smdkc100_uartcfgs));
	s5pc1xx_reserve_bootmem();
}

static void __init smdkc100_smc911x_set(void)
{
	unsigned int tmp;

	tmp = __raw_readl(S5PC1XX_GPK0CON);
	tmp &=~S5PC1XX_GPK0_3_MASK;
	tmp |=(S5PC1XX_GPK0_3_SROM_CSn3);
	__raw_writel(tmp, S5PC1XX_GPK0CON);

	tmp = __raw_readl(S5PC1XX_SROM_BW);
	tmp &= ~(S5PC1XX_SROM_BW_BYTE_ENABLE3_MASK | S5PC1XX_SROM_BW_WAIT_ENABLE3_MASK |
		S5PC1XX_SROM_BW_ADDR_MODE3_MASK | S5PC1XX_SROM_BW_DATA_WIDTH3_MASK);
	tmp |= S5PC1XX_SROM_BW_DATA_WIDTH3_16BIT;

	__raw_writel(tmp, S5PC1XX_SROM_BW);

	__raw_writel((0x0<<28)|(0x4<<24)|(0xd<<16)|(0x1<<12)|(0x4<<8)|(0x6<<4)|(0x0<<0), S5PC1XX_SROM_BC3);
	
	__raw_writel(S5PC1XX_SROM_BCn_TACS(1) | S5PC1XX_SROM_BCn_TCOS(0) |
			S5PC1XX_SROM_BCn_TACC(27) | S5PC1XX_SROM_BCn_TCOH(0) |
			S5PC1XX_SROM_BCn_TCAH(2) | S5PC1XX_SROM_BCn_TACP(0) |
			S5PC1XX_SROM_BCn_PMC_NORMAL, S5PC1XX_SROM_BC3);
}

// added by sy82.yoon(2009.05.18) to support android pmem
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

static void __init smdkc100_machine_init(void)
{
        s3c_device_nand.dev.platform_data = &s3c_nand_mtd_part_info;
	s3c_device_onenand.dev.platform_data = &s3c_onenand_data;

	smdkc100_smc911x_set();

	s3c_ts_set_platdata(&s3c_ts_platform);
	s3c_adc_set_platdata(&s3c_adc_platform);

	/* i2c */
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	/* spi */
#if defined(CONFIG_SPI_CNTRLR_0)
	samspi_set_slaves(BUSNUM(0), ARRAY_SIZE(sam_slv_pdata_0), sam_slv_pdata_0);
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	samspi_set_slaves(BUSNUM(1), ARRAY_SIZE(sam_slv_pdata_1), sam_slv_pdata_1);
#endif
#if defined(CONFIG_SPI_CNTRLR_2)
	//samspi_set_slaves(BUSNUM(2), ARRAY_SIZE(sam_slv_pdata_2), sam_slv_pdata_2);
#endif
	spi_register_board_info(sam_spi_devs, ARRAY_SIZE(sam_spi_devs));

	/* fimc */
	s3c_fimc0_set_platdata(NULL);
	s3c_fimc1_set_platdata(NULL);
	s3c_fimc2_set_platdata(NULL);


	/* pmem */	
#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();	// added by sy82.yoon(2009.05.18)
#endif
	spi_register_board_info(sam_spi_devs, ARRAY_SIZE(sam_spi_devs)); // append Hyunsoo : for SPI dirver

	/* fb */
#ifdef CONFIG_FB_S3C
//shaju	s3cfb_set_platdata(NULL);
#endif

	/* mipi-csi2 */
	s3c_csis_set_platdata(NULL);

#ifdef CONFIG_VIDEO_FIMC
	s3c_fimc_reset_camera();
#endif
	
	/* Setting up the HS-MMC clock for 133MHz using doutMpll */
	writel((readl(S5P_CLK_DIV3) & ~(0xfff << 0)), S5P_CLK_DIV3);

	platform_add_devices(smdkc100_devices, ARRAY_SIZE(smdkc100_devices));

#if defined(CONFIG_PM)
	s5pc1xx_pm_init();
#endif
        smdk_backlight_register();
}

MACHINE_START(SMDKC100, "SMDKC100")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5PC1XX_PA_SDRAM + 0x100,

	.init_irq	= s5pc100_init_irq,
	.map_io		= smdkc100_map_io,
	.init_machine	= smdkc100_machine_init,
	.timer		= &s5pc1xx_timer,
MACHINE_END


#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void) {
        writel(readl(S5P_OTHERS)|S5P_OTHERS_USB_SIG_MASK, S5P_OTHERS);
        writel(0x0, S3C_USBOTG_PHYPWR);         /* Power up */
        writel(OTGH_PHY_CLK_VALUE, S3C_USBOTG_PHYCLK);
        writel(0x7, S3C_USBOTG_RSTCON);

        udelay(50);
        writel(0x0, S3C_USBOTG_RSTCON);
        udelay(50);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) {
        writel(readl(S3C_USBOTG_PHYCLK) | (0X1 << 4), S3C_USBOTG_PHYCLK);
        writel(readl(S5P_OTHERS)&~S5P_OTHERS_USB_SIG_MASK, S5P_OTHERS);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_clk_en(void) {
	struct clk *otg_clk;

        switch (S3C_USB_CLKSRC) {
        case 0: /* epll clk */
                /* Setting the epll clk to 48 MHz, P=3, M=96, S=3 */
                writel((readl(S5P_EPLL_CON) & ~(S5P_EPLL_MASK)) | (S5P_EPLL_EN \
						| S5P_EPLLVAL(96,3,3)), S5P_EPLL_CON);
                writel((readl(S5P_CLK_SRC0) | S5P_CLKSRC0_EPLL_MASK), S5P_CLK_SRC0);
                writel((readl(S5P_CLK_SRC1)& ~S5P_CLKSRC1_UHOST_MASK), S5P_CLK_SRC1);

                /* USB host clock divider ratio is 1 */
                writel((readl(S5P_CLK_DIV2)& ~S5P_CLKDIV2_UHOST_MASK), S5P_CLK_DIV2);
                break;

	case 1: /* oscillator 12M clk */
		otg_clk = clk_get(NULL, "otg");
		clk_enable(otg_clk);
		otg_phy_init();
		writel((readl(S5P_CLK_SRC1) | S5P_CLKSRC1_CLK48M_MASK) \
						| S5P_CLKSRC1_UHOST_MASK, S5P_CLK_SRC1);

		//USB host colock divider ratio is 1
		writel(readl(S5P_CLK_DIV2)& ~S5P_CLKDIV2_UHOST_MASK, S5P_CLK_DIV2);
		break;
	/* Add other clock sources here */

        default:
                printk(KERN_INFO "Unknown USB Host Clock Source\n");
                BUG();
                break;
        }

        writel(readl(S5P_CLKGATE_D10)|S5P_CLKGATE_D10_USBHOST, S5P_CLKGATE_D10);
        writel(readl(S5P_SCLKGATE0)|S5P_CLKGATE_SCLK0_USBHOST, S5P_SCLKGATE0);

}

EXPORT_SYMBOL(usb_host_clk_en);
#endif

#if defined(CONFIG_KEYPAD_S3C) || defined (CONFIG_KEYPAD_S3C_MODULE)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	end = S5PC1XX_GPH3(rows);

	/* Set all the necessary GPH2 pins to special-function 0 */
	for (gpio = S5PC1XX_GPH3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	end = S5PC1XX_GPH2(columns);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = S5PC1XX_GPH2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif
