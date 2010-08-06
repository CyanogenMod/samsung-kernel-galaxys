/*
 * smdk6400_wm8580.c
 *
 * Copyright 2007, 2008 Wolfson Microelectronics PLC.
 *
 * Copyright (C) 2007, Ryu Euiyoul <ryu.real@gmail.com>
 * 
 * Copyright (C) 2008, SungJun Bae <june.bae@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>

#include <plat/regs-iis.h>
#include <plat/map-base.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-g3.h>

#include <mach/hardware.h>
#include <mach/audio.h>
#include <mach/map.h>
#include <asm/io.h>
#include <plat/regs-clock.h>

#include "s5pc1xx-i2s.h"
#include "s5pc1xx-pcm.h"

#include "../codecs/wm8580.h"

#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

extern void msleep(unsigned int msecs);

static int smdkc100_hifi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	unsigned int pll_out = 0; /*bclk = 0; */
	int ret = 0;
	unsigned int prescaler;
	u32 *iiscon;

	s3cdbg("Entered %s, rate = %d\n", __FUNCTION__, params_rate(params));

	iiscon = ioremap(S3C_PA_IIS, 0x100) + S3C64XX_IIS0CON;	

	writel(readl(iiscon)&~(0x1<<31),iiscon);
	msleep(100);
	writel(readl(iiscon)|(0x1<<31),iiscon);
	
	//writel(readl(S5P_CLK_SRC0)|S5P_CLKSRC0_EPLL_MASK,S5P_CLK_SRC0);
	s3cdbg("CLK_SRC0 : %x\n",readl(S5P_CLK_SRC0));

	//writel(readl(S5P_CLK_SRC3)|(0x0<<12)|(0x0<<24)|(0x0<<4),S5P_CLK_SRC3);
	//writel(readl(S5P_CLK_SRC3)|(0x2<<8),S5P_CLK_SRC3);
	s3cdbg("MUX Audio 0: %x\n",readl(S5P_CLK_SRC3));

	writel(readl(S5P_CLK_OUT)|(0x2<<12),S5P_CLK_OUT);
	s3cdbg("CLK OUT : %x\n",readl(S5P_CLK_OUT));

	writel(readl(S5P_CLKGATE_D20)|S5P_CLKGATE_D20_HCLKD2|S5P_CLKGATE_D20_I2SD2,S5P_CLKGATE_D20);
	s3cdbg("HCLKD2 Gate : %x\n",readl(S5P_CLKGATE_D20));

	writel(readl(S5P_SCLKGATE1)|S5P_CLKGATE_SCLK1_AUDIO0,S5P_SCLKGATE1);
	s3cdbg("S5P_SCLKGATE1 : %x\n",readl(S5P_SCLKGATE1));

	writel(readl(S5P_CLKGATE_D15)|(1<<0),S5P_CLKGATE_D15);
	s3cdbg("GATE D1_5 : %x\n",readl(S5P_CLKGATE_D15));

	writel(readl(S5P_EPLL_CON)|(0x1<<31),S5P_EPLL_CON);
	s3cdbg("EPLL CON : %x\n",readl(S5P_EPLL_CON));

//	writel((readl(S5P_CLK_DIV4)&~(0x07<<12))|(0x0<<12),S5P_CLK_DIV4);
	////writel((readl(S5P_CLK_DIV4)&~(0x07<<8))|(0x1<<8),S5P_CLK_DIV4);
	s3cdbg("DIV4: %x\n",readl(S5P_CLK_DIV4));

	/*PCLK & SCLK gating enable*/

//	writel(readl(S3C_PCLK_GATE)|S3C6410_CLKCON_PCLK_IIS2, S3C_PCLK_GATE);
//	writel(readl(S3C_SCLK_GATE)|S3C_CLKCON_SCLK_AUDIO0, S3C_SCLK_GATE);

	/*Clear I2S prescaler value [13:8] and disable prescaler*/
	/* set prescaler division for sample rate */
	ret = cpu_dai->ops.set_clkdiv(cpu_dai, S5PC1XX_DIV_PRESCALER, 0); //trb-29

	if (ret < 0)
		return ret;
//
//
	s3cdbg("%s: %d , params = %d\n", __FUNCTION__, __LINE__, params_rate(params));

	switch (params_rate(params)) {
	case 8000:
	case 16000:
	case 32000:
	case 64100:
		/* M=99, P=3, S=3 -- Fout=49.152*/
		writel((1<<31)|(99<<16)|(3<<8)|(3<<0) ,S5P_EPLL_CON);
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		/* M=135, P=3, S=3 -- Fout=67.738 */
		writel((1<<31)|(135<<16)|(3<<8)|(3<<0) ,S5P_EPLL_CON);
		break;
	case 48000:
	case 96000:
		/* M=147, P=3, S=3 -- Fin=12, Fout=73.728; */
		writel((1<<31)|(147<<16)|(3<<8)|(3<<0) ,S5P_EPLL_CON);
		break;
	default:
		writel((1<<31)|(128<<16)|(25<<8)|(0<<0) ,S5P_EPLL_CON);
		break;
	}

	switch (params_rate(params)) {
	case 8000:
		pll_out = 2048000;
		prescaler = 8;
		break;
	case 11025:
		pll_out = 2822400;
		prescaler = 8; 
		break;
	case 16000:
		pll_out = 4096000;
		prescaler = 4; 
		break;
	case 22050:
		pll_out = 5644800;
		prescaler = 4; 
		break;
	case 32000:
		pll_out = 8192000;
		prescaler = 2; 
		break;
	case 44100:
		pll_out = 11289600;
		prescaler = 2;
		break;
	case 48000:
		pll_out = 12288000;
		prescaler = 2; 
		break;
	case 88200:
		pll_out = 22579200;
		prescaler = 1; 
		break;
	case 96000:
		pll_out = 24576000;
		prescaler = 1;
		break;
	default:
		/* somtimes 32000 rate comes to 96000 
		   default values are same as 32000 */
		prescaler = 4;
		pll_out = 12288000;
		break;
	}

	/* set MCLK division for sample rate */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
	case SNDRV_PCM_FORMAT_S16_LE:
		prescaler *= 3;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		prescaler *= 2;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		prescaler *= 2;
		break;
	default:
		return -EINVAL;
	}

	prescaler = prescaler - 1; 

	/* set cpu DAI configuration */
	/*
	ret = cpu_dai->dai_ops.set_fmt(cpu_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBS_CFS); 
	if (ret < 0)
		return ret;
		*/

	/* trb: dai_ops merged into ops */
	ret = cpu_dai->ops.set_clkdiv(cpu_dai, S5PC1XX_DIV_BCLK,
		S3C64XX_IIS0MOD_256FS);
	if (ret < 0)
		return ret;

	/* trb: dai_ops merged into ops */
	/* set prescaler division for sample rate */
	ret = cpu_dai->ops.set_clkdiv(cpu_dai, S5PC1XX_DIV_PRESCALER,
		(prescaler << 0x8));
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * WM8580 HiFi DAI opserations.
 */
static struct snd_soc_ops smdkc100_hifi_ops = {
	.hw_params = smdkc100_hifi_hw_params,
};

static const struct snd_soc_dapm_widget wm8580_dapm_widgets[] = {
	SND_SOC_DAPM_LINE("I2S Front Jack", NULL),
	SND_SOC_DAPM_LINE("I2S Center Jack", NULL),
	SND_SOC_DAPM_LINE("I2S Rear Jack", NULL),
	SND_SOC_DAPM_LINE("Line In Jack", NULL),
};

/* example machine audio_mapnections */
static const struct snd_soc_dapm_route audio_map[] = {

	{ "I2S Front Jack", NULL, "VOUT1L" },
	{ "I2S Front Jack", NULL, "VOUT1R" },

	{ "I2S Center Jack", NULL, "VOUT2L" },
	{ "I2S Center Jack", NULL, "VOUT2R" },

	{ "I2S Rear Jack", NULL, "VOUT3L" },
	{ "I2S Rear Jack", NULL, "VOUT3R" },

	{ "AINL", NULL, "Line In Jack" },
	{ "AINR", NULL, "Line In Jack" },
		
};

static int smdkc100_wm8580_init(struct snd_soc_codec *codec)
{
	int i;

	/* Add smdkc100 specific widgets */
		snd_soc_dapm_new_controls(codec, wm8580_dapm_widgets,ARRAY_SIZE(wm8580_dapm_widgets));

	/* set up smdkc100 specific audio paths */
		snd_soc_dapm_add_routes(codec, audio_map,ARRAY_SIZE(audio_map));

	/* No jack detect - mark all jacks as enabled */
	for (i = 0; i < ARRAY_SIZE(wm8580_dapm_widgets); i++){
		//snd_soc_dapm_set_endpoint(codec,  wm8580_dapm_widgets[i].name, 1);
		snd_soc_dapm_enable_pin(codec, wm8580_dapm_widgets[i].name);
	}

	//snd_soc_dapm_sync_endpoints(codec);
	snd_soc_dapm_sync(codec);

	return 0;
}

static struct snd_soc_dai_link smdkc100_i2s_dai[] = {
{
	.name = "WM8580",
	.stream_name = "WM8580 HiFi Playback",
	.cpu_dai = &s3c_i2s_v50_dai,
	.codec_dai = &wm8580_dai[WM8580_DAI_PAIFRX],
	.init = smdkc100_wm8580_init,
	.ops = &smdkc100_hifi_ops,
},
};

/* trb: Parameters modified as per v 2.6.29 */
static struct snd_soc_card smdkc100 = {
	.name = "smdkc100",
	.dai_link = smdkc100_i2s_dai,
	.num_links = ARRAY_SIZE(smdkc100_i2s_dai),
	.platform = &s3c24xx_soc_platform 
};

static struct wm8580_setup_data smdkc100_wm8580_setup = {
	.i2c_address = 0x1b,
};


/* trb: Parameters modified as per v 2.6.29 */
static struct snd_soc_device smdkc100_snd_devdata = {
	.card = &smdkc100,
	.codec_dev = &soc_codec_dev_wm8580,
	.codec_data = &smdkc100_wm8580_setup,
};

static struct platform_device *smdkc100_snd_device;

static int __init smdkc100_init(void)
{
	int ret;

	smdkc100_snd_device = platform_device_alloc("soc-audio", -1);
	if (!smdkc100_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdkc100_snd_device, &smdkc100_snd_devdata);
	smdkc100_snd_devdata.dev = &smdkc100_snd_device->dev;
	ret = platform_device_add(smdkc100_snd_device);

	if (ret)
		platform_device_put(smdkc100_snd_device);
	
	return ret;
}

static void __exit smdkc100_exit(void)
{
	platform_device_unregister(smdkc100_snd_device);
}

module_init(smdkc100_init);
module_exit(smdkc100_exit);

/* Module information */
MODULE_AUTHOR("Mark Brown");
MODULE_DESCRIPTION("ALSA SoC SMDK6410 WM8580");
MODULE_LICENSE("GPL");
