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
#include <linux/delay.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/videodev2.h>
/* pmem */
#include <linux/android_pmem.h>

#include <mach/gpio-core.h>

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
#ifdef CONFIG_S5PC110_T959_BOARD
		.ufcon		 = S3C2410_UFCON_FIFOMODE | S3C2440_UFCON_TXTRIG10 | S3C2440_UFCON_RXTRIG4, // -> RX trigger leve : 8byte.
#else
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
#endif 
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


static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &pmem_adsp_pdata },
};
#endif








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

struct map_desc smdkc110_iodesc[] = {};

static void __init smdkc110_map_io(void)
{
	s3c_device_nand.name = "s5pc100-nand";
	s5pc11x_init_io(smdkc110_iodesc, ARRAY_SIZE(smdkc110_iodesc));
	s5pc11x_gpiolib_init();
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdkc110_uartcfgs, ARRAY_SIZE(smdkc110_uartcfgs));
#if defined(CONFIG_SERIAL_S5PC11X_HSUART)
	/* got to have a high enough uart source clock for higher speeds */
	writel((readl(S5P_CLK_DIV4) & ~(0xffff0000)) | 0x44440000, S5P_CLK_DIV4);
#endif
	s5pc11x_reserve_bootmem();
}



#ifdef CONFIG_ANDROID_PMEM
// pmem
static void __init android_pmem_set_platdata(void)
{
	pmem_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM);
	pmem_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM);

	pmem_gpu1_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM_GPU1);
	pmem_gpu1_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM_GPU1);	

	pmem_adsp_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM_ADSP);
	pmem_adsp_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM_ADSP);	
}
#endif





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

#if defined (CONFIG_S5PC110_JUPITER_BOARD)
	mi->bank[1].start = 0x40000000;
        //mi->bank[1].size = 256 * SZ_1M;
        mi->bank[1].size = 256 * SZ_1M; /* this value wil be changed to 256MB */
        mi->bank[1].node = 1;
#else
	mi->bank[1].start = 0x40000000;
	mi->bank[1].size = 128 * SZ_1M ; 
	mi->bank[1].node = 1;
#endif

#ifdef CONFIG_DDR_RAM_3G
	mi->bank[2].start = 0x50000000;
        mi->bank[2].size = 128 * SZ_1M;
        mi->bank[2].node = 2;
        mi->nr_banks = 3;
#else
	mi->nr_banks = 2;
#endif

}



#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void) 
{
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

// Y.J Jung fix (5/15/2010)
//struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(64)));
int guard_dma_usb_ctrl __attribute__((aligned(64)));

EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) 
{
	writel(readl(S3C_USBOTG_PHYPWR)|(0x3<<3), S3C_USBOTG_PHYPWR);
	writel(readl(S5P_USB_PHY_CONTROL)&~(1<<0), S5P_USB_PHY_CONTROL);

}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_clk_en(void) 
{

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


