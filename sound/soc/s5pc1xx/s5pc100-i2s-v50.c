/*
 * s3c-i2s.c  --  ALSA Soc Audio Layer
 *
 * (c) 2006 Wolfson Microelectronics PLC.
 * Graeme Gregory graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * (c) 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Ryu Euiyoul <ryu.real@gmail.com>
 *
 * Copyright (C) 2008, SungJun Bae<june.bae@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *
 *  Revision history
 *    11th Dec 2006   Merged with Simtec driver
 *    10th Nov 2006   Initial version.
 *    1st  Dec 2008   Initial version from s3c64xx-i2s.c.
 *    23rd Jun 2009   Modification as per kernel v 2.6.29
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/timer.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/dma.h>

#include <mach/map.h>
#include <mach/gpio.h>
#include <plat/regs-iis.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-g3.h>
#include <plat/regs-clock.h>

#include <mach/audio.h>
#include <mach/dma.h>

#include <plat/regs-clock.h>

#include "s5pc1xx-i2s.h"
#include "s5pc1xx-pcm.h"

#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

/* used to disable sysclk if external crystal is used */
static int extclk = 0;
module_param(extclk, int, 0);
MODULE_PARM_DESC(extclk, "set to 1 to disable s3c24XX i2s sysclk");

static struct s3c2410_dma_client s5pc1xx_dma_client_out = {
	.name = "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s5pc1xx_dma_client_in = {
	.name = "I2S PCM Stereo in"
};

static struct s3c24xx_pcm_dma_params s5pc1xx_i2s_pcm_stereo_out = {
	.client		= &s5pc1xx_dma_client_out,
	.channel	= DMACH_I2S_V50_OUT,
	.dma_addr	= S3C_PA_IIS + S3C64XX_IISFIFO,
	.dma_size	= 4,
};

static struct s3c24xx_pcm_dma_params s5pc1xx_i2s_pcm_stereo_in = {
	.client		= &s5pc1xx_dma_client_in,
	.channel	= DMACH_I2S_V50_IN,
	.dma_addr	= S3C_PA_IIS + S3C64XX_IISFIFORX,
	.dma_size	= 4,
};

struct s5pc1xx_i2s_info {
	void __iomem	*regs;
	struct clk	*iis_clk;
	int master;
};
static struct s5pc1xx_i2s_info s5pc1xx_i2s;

static void s5pc1xx_snd_txctrl(int on)
{
	u32 iiscon;
	u32 iismod;

	s3cdbg("Entered %s : on = %d \n", __FUNCTION__, on);

	iiscon  = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
	iismod  = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

	s3cdbg("r: IISCON: %x IISMOD: %x\n", iiscon, iismod);

	if (on) {
		iiscon |= S3C64XX_IIS0CON_I2SACTIVE;

		writel(iismod,  s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);
		writel(iiscon,  s5pc1xx_i2s.regs + S3C64XX_IIS0CON);

	} else {
		/* note, we have to disable the FIFOs otherwise bad things
		 * seem to happen when the DMA stops. According to the
		 * Samsung supplied kernel, this should allow the DMA
		 * engine and FIFOs to reset. If this isn't allowed, the
		 * DMA engine will simply freeze randomly.
		 */
		iiscon &= ~(S3C64XX_IIS0CON_I2SACTIVE);
		iismod &= ~S3C64XX_IIS0MOD_TXMODE;

		writel(iiscon,  s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
		writel(iismod,  s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);
	}

	s3cdbg("w: IISCON: %x IISMOD: %x\n", iiscon, iismod);
}

static void s5pc1xx_snd_rxctrl(int on)
{
	u32 iisfcon;
	u32 iiscon;
	u32 iismod;

	s3cdbg("Entered %s: on = %d\n", __FUNCTION__, on);

	iisfcon = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0FIC);
	iiscon  = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
	iismod  = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

	s3cdbg("r: IISCON: %x IISMOD: %x IISFCON: %x\n", iiscon, iismod, iisfcon);

	if (on) {
		iiscon |= S3C64XX_IIS0CON_I2SACTIVE;

		writel(iismod,  s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);
		writel(iisfcon, s5pc1xx_i2s.regs + S3C64XX_IIS0FIC);
		writel(iiscon,  s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
	} else {
		/* note, we have to disable the FIFOs otherwise bad things
		 * seem to happen when the DMA stops. According to the
		 * Samsung supplied kernel, this should allow the DMA
		 * engine and FIFOs to reset. If this isn't allowed, the
		 * DMA engine will simply freeze randomly.
		 */

		iiscon &= ~S3C64XX_IIS0CON_I2SACTIVE;
		iismod &= ~S3C64XX_IIS0MOD_RXMODE;

		writel(iisfcon, s5pc1xx_i2s.regs + S3C64XX_IIS0FIC);
		writel(iiscon,  s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
		writel(iismod,  s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

	}
	s3cdbg("w: IISCON: %x IISMOD: %x IISFCON: %x\n", iiscon, iismod, iisfcon);
}

/*
 * Wait for the LR signal to allow synchronisation to the L/R clock
 * from the codec. May only be needed for slave mode.
 */
static int s3c24xx_snd_lrsync(void)
{
	u32 iiscon;
	unsigned long timeout = jiffies + msecs_to_jiffies(5);

	s3cdbg("Entered %s\n", __FUNCTION__);

	while (1) {
		iiscon = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON);

		if (iiscon & S3C64XX_IISCON_LRINDEX)
			break;

		if (timeout < jiffies) 
			return -ETIMEDOUT;
		
	}

	return 0;
}

/*
 * Check whether CPU is the master or slave
 */
static inline int s3c24xx_snd_is_clkmaster(void)
{
	s3cdbg("Entered %s\n", __FUNCTION__);
	
//	writel(readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD)|~(S3C64XX_IISMOD_SLAVE),s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

//	return (readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD) & S3C64XX_IISMOD_SLAVE) ? 0:1;
	return (readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD) & S3C64XX_IIS0MOD_IMS_SLAVE) ? 0:1;
}

/*
 * Set S3C24xx I2S DAI format
 */
static int s3c_i2s_v50_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	u32 iismod;

	s3cdbg("Entered %s: fmt = %d\n", __FUNCTION__, fmt);

	iismod = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

	return 0;

}

//trb : Param 'dai' added - as per v 2.6.29
static int s3c_i2s_v50_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, 
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;

	unsigned long iiscon;
	unsigned long iismod;
	unsigned long iisfcon;
	
	s3cdbg("Entered %s\n", __FUNCTION__);

	s5pc1xx_i2s.master = 1;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		rtd->dai->cpu_dai->dma_data = &s5pc1xx_i2s_pcm_stereo_out;
	} else {
		rtd->dai->cpu_dai->dma_data = &s5pc1xx_i2s_pcm_stereo_in;
	}

	/* Working copies of registers */
	iiscon = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
	iismod = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);
	iisfcon = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0FIC);

	/* is port used by another stream */
	if (!(iiscon & S3C64XX_IIS0CON_I2SACTIVE)) {

		/* Clear BFS field [2:1] */
		iismod &= ~(0x3<<1);
		iismod |= S3C64XX_IIS0MOD_32FS | S3C64XX_IIS0MOD_INTERNAL_CLK;

		if (!s5pc1xx_i2s.master)
			iismod |= S3C64XX_IISMOD_SLAVE;
		else
			iismod |= S3C64XX_IIS0MOD_IMS_EXTERNAL_MASTER;
	}

	iiscon |= S3C64XX_IISCON_FTXURINTEN;
	iiscon |= S3C64XX_IIS0CON_TXDMACTIVE;
	iiscon |= S3C64XX_IIS0CON_RXDMACTIVE;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		iismod |= S3C64XX_IIS0MOD_TXMODE;
		iisfcon |= S3C64XX_IIS_TX_FLUSH;
	} else {
		iismod |= S3C64XX_IIS0MOD_RXMODE;
		iisfcon |= S3C64XX_IIS_RX_FLUSH;
	}

	/* Multi channel enable */
	iismod &= ~S3C64XX_IIS0MOD_DCE_MASK;
	switch (params_channels(params)) {
	case 6:
		printk("s3c i2s: 5.1channel\n");
		iismod |= S3C64XX_IIS0MOD_DCE_SD2;
		iismod |= S3C64XX_IIS0MOD_DCE_SD2;
		break;
	case 4:
		printk("s3c i2s: 4 channel\n");
		iismod |= S3C64XX_IIS0MOD_DCE_SD2;
		break;
	case 2:
		printk("s3c i2s: 2 channel\n");
		break;
	default:
		printk(KERN_ERR "s3c-i2s-v50: %d channels unsupported\n",
		       params_channels(params));
		return -EINVAL;
	}

	/* Set the bit rate */
	iismod &= ~0x6000;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		iismod &= ~S3C64XX_IIS0MOD_FS_MASK;
		iismod |= S3C64XX_IIS0MOD_256FS | S3C64XX_IIS0MOD_32FS;
		iismod &= ~(0x3<<13);
		iismod |= S3C64XX_IIS0MOD_16BIT;
		break;
	case SNDRV_PCM_FORMAT_S8:
		iismod |= S3C64XX_IIS0MOD_8BIT;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		iismod &= ~S3C64XX_IIS0MOD_FS_MASK;
		iismod |= S3C64XX_IIS0MOD_384FS | S3C64XX_IIS0MOD_48FS;
		iismod &= ~(0x3<<13);
		iismod |= S3C64XX_IIS0MOD_24BIT;
		break;
	default:
		return -EINVAL;
	}

	writel(iisfcon, s5pc1xx_i2s.regs + S3C64XX_IIS0FIC);
	writel(iiscon, s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
	writel(iismod, s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

	// Tx, Rx fifo flush bit clear
	iisfcon  &= ~(S3C64XX_IIS_TX_FLUSH | S3C64XX_IIS_RX_FLUSH);
	writel(iisfcon, s5pc1xx_i2s.regs + S3C64XX_IIS0FIC);

	s3cdbg("s3c iis mode: 0x%08x\n", readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD));
	s3cdbg("s3c: params_channels %d\n", params_channels(params));
	s3cdbg("s3c: params_format %d\n", params_format(params));
	s3cdbg("s3c: params_subformat %d\n", params_subformat(params));
	s3cdbg("s3c: params_period_size %d\n", params_period_size(params));
	s3cdbg("s3c: params_period_bytes %d\n", params_period_bytes(params));
	s3cdbg("s3c: params_periods %d\n", params_periods(params));
	s3cdbg("s3c: params_buffer_size %d\n", params_buffer_size(params));
	s3cdbg("s3c: params_buffer_bytes %d\n", params_buffer_bytes(params));
//	s3cdbg("s3c: params_tick_time %d\n", params_tick_time(params));
	s3cdbg("hw_params: IISCON: %lx IISMOD: %lx\n", iiscon, iismod);

	return 0;

}

/* trb: extra param dai added in function prototype as per v 2.6.29 */
static int s3c_i2s_v50_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	s3cdbg("Entered %s: cmd = %d\n", __FUNCTION__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (!s3c24xx_snd_is_clkmaster()) {
			ret = s3c24xx_snd_lrsync();
			if (ret)
				goto exit_err;
		}

		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s5pc1xx_snd_rxctrl(1);
		else
			s5pc1xx_snd_txctrl(1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s5pc1xx_snd_rxctrl(0);
		else
			s5pc1xx_snd_txctrl(0);
		break;
	default:
		ret = -EINVAL;
		break;
	}

exit_err:
	return ret;
}

/* trb: extra param dai added in function prototype as per v 2.6.29 */
static void s5pc1xx_i2s_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	unsigned long iismod, iiscon;

	s3cdbg("Entered %s\n", __FUNCTION__);
	
	iismod=readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		iismod &= ~S3C64XX_IIS0MOD_TXMODE;
	} else {
		iismod &= ~S3C64XX_IIS0MOD_RXMODE;
	}

	writel(iismod, s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

	iiscon=readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
	iiscon &= !S3C64XX_IIS0CON_I2SACTIVE;
	writel(iiscon, s5pc1xx_i2s.regs + S3C64XX_IIS0CON);

	/* Clock disable */
	/* EPLL disable */
	writel(readl(S5P_EPLL_CON)&~(0x1<<31),S5P_EPLL_CON);
}


/*
 * Set S3C24xx Clock source
 */
static int s3c_i2s_v50_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{
	u32 iismod = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);

	s3cdbg("Entered %s : clk_id = %d\n", __FUNCTION__, clk_id);

	iismod &= ~S3C64XX_IISMOD_MPLL;

	switch (clk_id) {
	case S5PC1XX_CLKSRC_PCLK:
		break;
	case S5PC1XX_CLKSRC_MPLL:
		iismod |= S3C64XX_IISMOD_MPLL;
		break;
	default:
		return -EINVAL;
	}

	writel(iismod, s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);
	return 0;
}

/*
 * Set S3C24xx Clock dividers
 */
static int s3c_i2s_v50_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	u32 reg;

	s3cdbg("Entered %s : div_id = %d, div = %x\n", __FUNCTION__, div_id, div);

	switch (div_id) {
	case S5PC1XX_DIV_MCLK:
		break;
	case S5PC1XX_DIV_BCLK:
		reg = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0MOD) & ~(S3C64XX_IIS0MOD_FS_MASK);
		writel(reg | div, s5pc1xx_i2s.regs + S3C64XX_IIS0MOD);
		break;
	case S5PC1XX_DIV_PRESCALER:
		if (div)
			div |= 1 << 15;
		writel(div, s5pc1xx_i2s.regs + S3C64XX_IIS0PSR);
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}

/*
 * To avoid duplicating clock code, allow machine driver to
 * get the clockrate from here.
 */
u32 s3c_i2s_v50_get_clockrate(void)
{
	return clk_get_rate(s5pc1xx_i2s.iis_clk);
}
EXPORT_SYMBOL_GPL(s3c_i2s_v50_get_clockrate);

static irqreturn_t s3c_iis_irq(int irqno, void *dev_id)
{
	u32 iiscon;
	
	iiscon  = readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
	if((1<<17) & iiscon) {
		iiscon &= ~(1<<16);
		iiscon |= (1<<17);
		writel(iiscon, s5pc1xx_i2s.regs + S3C64XX_IIS0CON);
		printk("underrun interrupt IISCON = 0x%08x\n", readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON));
	}

	return IRQ_HANDLED;
}

static int s3c_i2s_v50_probe(struct platform_device *pdev,
	struct snd_soc_dai *dai)
{
	int ret;

	s3cdbg("Entered %s\n", __FUNCTION__);

	s5pc1xx_i2s.regs = ioremap(S3C_PA_IIS, 0x100);
	if (s5pc1xx_i2s.regs == NULL)
		return -ENXIO;

	s5pc1xx_i2s.iis_clk=clk_get(&pdev->dev, "iis");
	if (s5pc1xx_i2s.iis_clk == NULL) {
		printk("failed to get iis_clock\n");
		iounmap(s5pc1xx_i2s.regs);
		return -ENODEV;
	}
	clk_enable(s5pc1xx_i2s.iis_clk);

	ret = request_irq(IRQ_I2S0, s3c_iis_irq, 0,
			 "s3c-i2s-v50", pdev);
	if (ret < 0) {
		printk("fail to claim i2s irq , ret = %d\n", ret);
		return -ENODEV;
	}

	s3cdbg("IIS Reset!\n");
//	writel(readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON)&~(0x1<<31),(s5pc1xx_i2s.regs + S3C64XX_IIS0CON));
//	msleep(100);
	writel(readl(s5pc1xx_i2s.regs + S3C64XX_IIS0CON)|(0x1<<31),(s5pc1xx_i2s.regs + S3C64XX_IIS0CON));

	return 0;
}

#ifdef CONFIG_PM
static int s3c_i2s_v50_suspend(struct snd_soc_dai *dai)
{
	s3cdbg("Entered %s\n", __FUNCTION__);
	return 0;
}

static int s3c_i2s_v50_resume(struct snd_soc_dai *dai)
{
	s3cdbg("Entered %s\n", __FUNCTION__);
	return 0;
}

#else
#define s3c_i2s_v50_suspend	NULL
#define s3c_i2s_v50_resume	NULL
#endif


#define S5PC1XX_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

struct snd_soc_dai s3c_i2s_v50_dai = {
	.name = "s3c-i2s-v50",
	.id = 0,
	.probe = s3c_i2s_v50_probe,
	.suspend = s3c_i2s_v50_suspend,
	.resume = s3c_i2s_v50_resume,
	.playback = {
		.channels_min = 2,
		.channels_max = 6,
		.rates = S5PC1XX_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = S5PC1XX_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,},
	.ops = {
		.shutdown = s5pc1xx_i2s_shutdown,
		.trigger = s3c_i2s_v50_trigger,
		.hw_params = s3c_i2s_v50_hw_params,
		.set_fmt = s3c_i2s_v50_set_fmt,
		.set_clkdiv = s3c_i2s_v50_set_clkdiv,
		.set_sysclk = s3c_i2s_v50_set_sysclk,
	},
};
EXPORT_SYMBOL_GPL(s3c_i2s_v50_dai);


/* trb: dai registration - included in 2.6.29 */
//[
static int __init s5pc1xx_i2s_init(void)
{
	return snd_soc_register_dai(&s3c_i2s_v50_dai);
}
module_init(s5pc1xx_i2s_init);


static void __exit s5pc1xx_i2s_exit(void)
{
	snd_soc_unregister_dai(&s3c_i2s_v50_dai);
}
module_exit(s5pc1xx_i2s_exit);
//]

/* Module information */
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_DESCRIPTION("s3c24xx I2S SoC Interface");
MODULE_LICENSE("GPL");
