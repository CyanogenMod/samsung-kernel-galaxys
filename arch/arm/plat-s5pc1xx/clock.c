/* linux/arch/arm/plat-s5pc1xx/clock.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5PC1XX Base clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>

struct clk clk_27m = {
	.name		= "clk_27m",
	.id		= -1,
	.rate		= 27000000,
};

static int clk_48m_ctrl(struct clk *clk, int enable)
{
	unsigned long flags;
	u32 val;

	local_irq_save(flags);

	val = __raw_readl(S5P_CLK_SRC1);
	if (enable)
		val |= S5P_CLKSRC1_CLK48M_MASK;
	else
		val &= ~S5P_CLKSRC1_CLK48M_MASK;

	__raw_writel(val, S5P_CLK_SRC1);
	local_irq_restore(flags);

	return 0;
}

struct clk clk_48m = {
	.name		= "clk_48m",
	.id		= -1,
	.rate		= 48000000,
	.enable		= clk_48m_ctrl,
};

struct clk clk_54m = {
	.name		= "clk_54m",
	.id		= -1,
	.rate		= 54000000,
};

static int inline s5pc1xx_clk_gate(void __iomem *reg,
				struct clk *clk,
				int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	u32 con;

	con = __raw_readl(reg);

	if (enable)
		con |= ctrlbit;
	else
		con &= ~ctrlbit;

	__raw_writel(con, reg);
	return 0;
}

static int s5pc1xx_setrate_sclk_cam(struct clk *clk, unsigned long rate)
{
	u32 shift = 24;
	u32 cam_div, cfg;
	unsigned long src_clk = clk_get_rate(clk->parent);

	cam_div = src_clk / rate;

	if (cam_div > 32)
		cam_div = 32;

	cfg = __raw_readl(S5P_CLK_DIV1);
	cfg &= ~(0x1f << shift);
	cfg |= ((cam_div - 1) << shift);
	__raw_writel(cfg, S5P_CLK_DIV1);

	printk("parent clock for camera: %ld.%03ld MHz, divisor: %d\n", \
		print_mhz(src_clk), cam_div);

	return 0;
}

static int s5pc1xx_clk_d00_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D00, clk, enable);
}

static int s5pc1xx_clk_d01_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D01, clk, enable);
}

static int s5pc1xx_clk_d02_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D02, clk, enable);
}

static int s5pc1xx_clk_d10_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D10, clk, enable);
}

static int s5pc1xx_clk_d11_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D11, clk, enable);
}

static int s5pc1xx_clk_d12_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D12, clk, enable);
}

static int s5pc1xx_clk_d13_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D13, clk, enable);
}

static int s5pc1xx_clk_d14_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D14, clk, enable);
}

static int s5pc1xx_clk_d15_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D15, clk, enable);
}

int s5pc1xx_clk_d20_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_CLKGATE_D20, clk, enable);
}

int s5pc1xx_sclk0_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_SCLKGATE0, clk, enable);
}

int s5pc1xx_sclk1_ctrl(struct clk *clk, int enable)
{
	return s5pc1xx_clk_gate(S5P_SCLKGATE1, clk, enable);
}

static struct clk init_clocks_disable[] = {
	{
		.name		= "mipi-dsim",
		.id		= -1,
		.parent		= &clk_27m,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_DSI,
	}, {
		.name		= "mipi-csis",
		.id		= -1,
		.parent		= &clk_27m,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_CSI,
	}, {
		.name		= "ccan",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_CCAN0,
	}, {
		.name		= "ccan",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_CCAN1,
	}, {
		.name		= "keypad",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_KEYIF,
	}, {
		.name		= "hclkd2",
		.id		= -1,
		.parent		= NULL,
		.enable		= s5pc1xx_clk_d20_ctrl,
		.ctrlbit	= S5P_CLKGATE_D20_HCLKD2,
	}, {
		.name		= "otg",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d10_ctrl,
		.ctrlbit	= S5P_CLKGATE_D10_USBOTG,
	},
};

static struct clk init_clocks[] = {
	/* System1 (D0_0) devices */
	{
		.name		= "intc",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d00_ctrl,
		.ctrlbit	= S5P_CLKGATE_D00_INTC,
	}, {
		.name		= "tzic",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d00_ctrl,
		.ctrlbit	= S5P_CLKGATE_D00_TZIC,
	}, {
		.name		= "cfcon",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d00_ctrl,
		.ctrlbit	= S5P_CLKGATE_D00_CFCON,
	}, {
		.name		= "mdma",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d00_ctrl,
		.ctrlbit	= S5P_CLKGATE_D00_MDMA,
	}, {
		.name		= "g2d",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d00_ctrl,
		.ctrlbit	= S5P_CLKGATE_D00_G2D,
	}, {
		.name		= "secss",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d00_ctrl,
		.ctrlbit	= S5P_CLKGATE_D00_SECSS,
	}, {
		.name		= "cssys",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d00_ctrl,
		.ctrlbit	= S5P_CLKGATE_D00_CSSYS,
	},

	/* Memory (D0_1) devices */
	{
		.name		= "dmc",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d01_ctrl,
		.ctrlbit	= S5P_CLKGATE_D01_DMC,
	}, {
		.name		= "sromc",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d01_ctrl,
		.ctrlbit	= S5P_CLKGATE_D01_SROMC,
	}, {
		.name		= "onenand",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d01_ctrl,
		.ctrlbit	= S5P_CLKGATE_D01_ONENAND,
	}, {
		.name		= "nand",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d01_ctrl,
		.ctrlbit	= S5P_CLKGATE_D01_NFCON,
	}, {
		.name		= "intmem",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d01_ctrl,
		.ctrlbit	= S5P_CLKGATE_D01_INTMEM,
	}, {
		.name		= "ebi",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d01_ctrl,
		.ctrlbit	= S5P_CLKGATE_D01_EBI,
	},

	/* System2 (D0_2) devices */
	{
		.name		= "seckey",
		.id		= -1,
		.parent		= &clk_pd0,
		.enable		= s5pc1xx_clk_d02_ctrl,
		.ctrlbit	= S5P_CLKGATE_D02_SECKEY,
	}, {
		.name		= "sdm",
		.id		= -1,
		.parent		= &clk_hd0,
		.enable		= s5pc1xx_clk_d02_ctrl,
		.ctrlbit	= S5P_CLKGATE_D02_SDM,
	},

	/* File (D1_0) devices */
	{
		.name		= "pdma0",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d10_ctrl,
		.ctrlbit	= S5P_CLKGATE_D10_PDMA0,
	}, {
		.name		= "pdma1",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d10_ctrl,
		.ctrlbit	= S5P_CLKGATE_D10_PDMA1,
	}, {
		.name		= "usb-host",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d10_ctrl,
		.ctrlbit	= S5P_CLKGATE_D10_USBHOST,
	}, {
		.name		= "modem",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d10_ctrl,
		.ctrlbit	= S5P_CLKGATE_D10_MODEMIF,
	}, {
		.name		= "hsmmc",
		.id		= 0,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d10_ctrl,
		.ctrlbit	= S5P_CLKGATE_D10_HSMMC0,
	}, {
		.name		= "hsmmc",
		.id		= 1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d10_ctrl,
		.ctrlbit	= S5P_CLKGATE_D10_HSMMC1,
	}, {
		.name		= "hsmmc",
		.id		= 2,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d10_ctrl,
		.ctrlbit	= S5P_CLKGATE_D10_HSMMC2,
	},

	/* Multimedia1 (D1_1) devices */
	{
		.name		= "lcd",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_LCD,
	}, {
		.name		= "rotator",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_ROTATOR,
	}, {
		.name		= "fimc0",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_FIMC0,
	}, {
		.name		= "fimc1",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_FIMC1,
	}, {
		.name		= "fimc2",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_FIMC2,
	}, {
		.name		= "jpeg",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_JPEG,
	}, {
		.name		= "g3d",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_G3D,
	}, {
		.name		= "rot",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d11_ctrl,
		.ctrlbit	= S5P_CLKGATE_D11_ROTATOR,
	},

	/* Multimedia2 (D1_2) devices */
	{
		.name		= "tv",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d12_ctrl,
		.ctrlbit	= S5P_CLKGATE_D12_TV,
	}, {
		.name		= "vp",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d12_ctrl,
		.ctrlbit	= S5P_CLKGATE_D12_VP,
	}, {
		.name		= "mixer",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d12_ctrl,
		.ctrlbit	= S5P_CLKGATE_D12_MIXER,
	}, {
		.name		= "hdmi",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d12_ctrl,
		.ctrlbit	= S5P_CLKGATE_D12_HDMI,
	}, {
		.name		= "mfc",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s5pc1xx_clk_d12_ctrl,
		.ctrlbit	= S5P_CLKGATE_D12_MFC,
	},

	/* System (D1_3) devices */
	{
		.name		= "chipid",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d13_ctrl,
		.ctrlbit	= S5P_CLKGATE_D13_CHIPID,
	}, {
		.name		= "gpio",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d13_ctrl,
		.ctrlbit	= S5P_CLKGATE_D13_GPIO,
	}, {
		.name		= "apc",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d13_ctrl,
		.ctrlbit	= S5P_CLKGATE_D13_APC,
	}, {
		.name		= "iec",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d13_ctrl,
		.ctrlbit	= S5P_CLKGATE_D13_IEC,
	}, {
		.name		= "timers",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d13_ctrl,
		.ctrlbit	= S5P_CLKGATE_D13_PWM,
	}, {
		.name		= "systimer",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d13_ctrl,
		.ctrlbit	= S5P_CLKGATE_D13_SYSTIMER,
	}, {
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d13_ctrl,
		.ctrlbit	= S5P_CLKGATE_D13_WDT,
	}, {
		.name		= "rtc",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d13_ctrl,
		.ctrlbit	= S5P_CLKGATE_D13_RTC,
	},

	/* Connectivity (D1_4) devices */
	{
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_UART0,
	}, {
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_UART1,
	}, {
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_UART2,
	}, {
		.name		= "uart",
		.id		= 3,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_UART3,
	}, {
		.name		= "i2c",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_IIC,
	}, {
		.name		= "hdmi-i2c",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_HDMI_IIC,
	}, {
		.name		= "spi",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_SPI0,
	}, {
		.name		= "spi",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_SPI1,
	}, {
		.name		= "spi",
		.id		= 2,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_SPI2,
	}, {
		.name		= "irda",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_IRDA,
	}, {
		.name		= "hsitx",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_HSITX,
	}, {
		.name		= "hsirx",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d14_ctrl,
		.ctrlbit	= S5P_CLKGATE_D14_HSIRX,
	},

	/* Audio (D1_5) devices */
	{
		.name		= "i2s_v50",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_IIS0, /* I2S0 is v5.0 */
	}, {
		.name		= "i2s_v32",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_IIS1, /* I2S1 is v3.2 */
	}, {
		.name		= "i2s_v32",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_IIS2, /* I2S2 is v3.2 */
	}, {
		.name		= "ac97",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_AC97,
	}, {
		.name		= "pcm",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_PCM0,
	}, {
		.name		= "pcm",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_PCM1,
	}, {
		.name		= "spdif",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_SPDIF,
	}, {
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_TSADC,
	}, {
		.name		= "keyif",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_KEYIF,
	}, {
		.name		= "cg",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc1xx_clk_d15_ctrl,
		.ctrlbit	= S5P_CLKGATE_D15_CG,
	},

	/* Audio (D2_0) devices: all disabled */

	/* Special Clocks 1 */
	{
		.name		= "sclk_hpm",
		.id		= -1,
		.parent		= NULL,
		.enable		= s5pc1xx_sclk0_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK0_HPM,
	}, {
		.name		= "sclk_onenand",
		.id		= -1,
		.parent		= NULL,
		.enable		= s5pc1xx_sclk0_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK0_ONENAND,
	}, {
		.name		= "sclk_spi_48",
		.id		= 0,
		.parent		= &clk_48m,
		.enable		= s5pc1xx_sclk0_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK0_SPI0_48,
	}, {
		.name		= "sclk_spi_48",
		.id		= 1,
		.parent		= &clk_48m,
		.enable		= s5pc1xx_sclk0_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK0_SPI1_48,
	}, {
		.name		= "sclk_spi_48",
		.id		= 2,
		.parent		= &clk_48m,
		.enable		= s5pc1xx_sclk0_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK0_SPI2_48,
	}, {
		.name		= "sclk_mmc_48",
		.id		= 0,
		.parent		= &clk_48m,
		.enable		= s5pc1xx_sclk0_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK0_MMC0_48,
	}, {
		.name		= "sclk_mmc_48",
		.id		= 1,
		.parent		= &clk_48m,
		.enable		= s5pc1xx_sclk0_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK0_MMC1_48,
	}, {
		.name		= "sclk_mmc_48",
		.id		= 2,
		.parent		= &clk_48m,
		.enable		= s5pc1xx_sclk0_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK0_MMC2_48,
	},

	/* Special Clocks 2 */
	{
		.name		= "sclk_tv_54",
		.id		= -1,
		.parent		= &clk_54m,
		.enable		= s5pc1xx_sclk1_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK1_TV54,
	}, {
		.name		= "sclk_vdac_54",
		.id		= -1,
		.parent		= &clk_54m,
		.enable		= s5pc1xx_sclk1_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK1_VDAC54,
	}, {
		.name		= "sclk_spdif",
		.id		= -1,
		.parent		= NULL,
		.enable		= s5pc1xx_sclk1_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK1_SPDIF,
	}, {
		.name		= "sclk_cam",
		.id		= -1,
		.parent		= &clk_dout_mpll2,
		.enable		= s5pc1xx_sclk1_ctrl,
		.ctrlbit	= S5P_CLKGATE_SCLK1_CAM,
		.set_rate	= s5pc1xx_setrate_sclk_cam,
	},
};

static struct clk *clks[] __initdata = {
	&clk_ext,
	&clk_epll,
	&clk_27m,
	&clk_48m,
	&clk_54m,
};

void __init s5pc1xx_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));

	clkp = init_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {

		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}

		(clkp->enable)(clkp, 0);
	}
}
