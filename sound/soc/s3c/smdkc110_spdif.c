/*
 * smdkc110_spdif.c
 *
 * Copyright 2007, 2008 Wolfson Microelectronics PLC.
 *
 * 
 * Copyright (C) 2009, Kwka Hyun Min <hyunmin.kwak@samsung.com>
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
#include <linux/delay.h>
	 
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
	 
#include <asm/mach-types.h>
	 
#include <plat/regs-iis.h>
#include <plat/map-base.h>
//#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-g3.h>
#include <plat/regs-spdif.h>	//add kwak for spdifs	 
#include <mach/hardware.h>
#include <mach/audio.h>
#include <mach/map.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <plat/regs-clock.h>
	 
#include "s5p-spdif.h"
#include "s3c-pcm.h"

#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

struct s5p_spdif_info s5p_spdif;

#define SPDIF_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)


struct snd_soc_dai spdif_dai[] = {
	{
		.name = "SPDIF PAIFRX",
		.id = 1,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		
		.capture = {
			.stream_name = "Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	},
};

static const struct snd_soc_dapm_widget spdif_dapm_widgets[] = {
	SND_SOC_DAPM_MIXER("VOUT1L",SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("VOUT1R",SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("VOUT2L",SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("VOUT2R",SND_SOC_NOPM, 0, 0, NULL, 0),
	
	SND_SOC_DAPM_LINE("SPDIF Front Jack", NULL),
	SND_SOC_DAPM_LINE("SPDIF Rear Jack", NULL),
	
};

/* example machine audio_mapnections */
static const struct snd_soc_dapm_route audio_map[] = {

	{ "SPDIF Front Jack", NULL, "VOUT1L" },
	{ "SPDIF Front Jack", NULL, "VOUT1R" },

	{ "SPDIF Rear Jack", NULL, "VOUT2L" },
	{ "SPDIF Rear Jack", NULL, "VOUT2R" },
		
};

static int smdkc110_spdif_init(struct snd_soc_codec *codec)
{
	int i;

	//printk("\n Enter the smdkc110_spdif_init \n");
	/* Add smdkc110 specific widgets */
	snd_soc_dapm_new_controls(codec, spdif_dapm_widgets,ARRAY_SIZE(spdif_dapm_widgets));

	/* set up smdkc110 specific audio paths */
	snd_soc_dapm_add_routes(codec, audio_map,ARRAY_SIZE(audio_map));

	snd_soc_dapm_new_widgets(codec);
	/* No jack detect - mark all jacks as enabled */
	for (i = 0; i < ARRAY_SIZE(spdif_dapm_widgets); i++){
		//Modified as per 2.6.29
		snd_soc_dapm_enable_pin(codec, spdif_dapm_widgets[i].name);
	}

	//Modified as per 2.6.29
	snd_soc_dapm_sync(codec);

	return 0;
}



static int smdkc110_spdif_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	u32 epll_vsel, epll_m, epll_p, epll_s, epll_div;
	u32 value;
 
	//configure gpio for spdif
	s3cdbg("Entered %s, rate = %d\n", __FUNCTION__, params_rate(params));
	/* The values of epll_vsel, epll_m, epll_p, epll_s and epll_div are taken from firmware code. */
 	 switch (params_rate(params)) {
		case 32000:
			epll_vsel = 1;
			epll_m = 131;
			epll_p = 6;
			epll_s = 4;
			epll_div = 1; 
			break;
		case 44100:
			epll_vsel = 0;
			epll_m = 271;
			epll_p = 18;
			epll_s = 3;
			epll_div = 1; 
			break;
		case 48000:
			epll_vsel = 0;
			epll_m = 131;
			epll_p = 8;
			epll_s = 3;
			epll_div = 1; 
			break;
		case 96000:
			epll_vsel = 0;
			epll_m = 131;
			epll_p = 8;
			epll_s = 3;
			epll_div = 0; 
			break;
		default:
			epll_vsel = 0;
			epll_m = 271;
			epll_p = 18;
			epll_s = 3;
			epll_div = 1; 
			break;
	}

	 /* Clock Settings */
	 // Mask output clock of MUXSPDIF
	 writel(readl(S5P_CLK_SRC_MASK0) | S5P_CLKSRC_MASK0_SPDIF | S5P_CLKSRC_MASK0_AUDIO0, S5P_CLK_SRC_MASK0);

	 /* Set IP3 Registers - Enable PCM0/1/2, I2S0/1/2, SPDIF */
	 value = readl(S5P_CLKGATE_IP3);
	 value |= S5P_CLKGATE_IP3_PCM0 | S5P_CLKGATE_IP3_PCM1 | S5P_CLKGATE_IP3_PCM2;
	 value |= S5P_CLKGATE_IP3_I2S0 | S5P_CLKGATE_IP3_I2S1 | S5P_CLKGATE_IP3_I2S2;
	 value |= S5P_CLKGATE_IP3_SPDIF; 
	 writel(value, S5P_CLKGATE_IP3);

         /* MUX EPLL */
	 writel(((readl(S5P_CLK_SRC0) & ~S5P_CLKSRC0_EPLL_MASK) | (0x1 << S5P_CLKSRC0_EPLL_SHIFT)), S5P_CLK_SRC0);

	/* Enable clock gating of AUDIO0 and SPDIF */
	value = readl(S5P_CLKGATE_SCLK0);
 	value |= S5P_CLKGATE_SCLK0_AUDIO0 | S5P_CLKGATE_SCLK0_SPDIF1;
	writel(value, S5P_CLKGATE_SCLK0);

	/* SCLK_AUDIO0, SCLK_EPLL */
	value = readl(S5P_CLK_SRC6);
	value &= ~ (S5P_CLKSRC6_SPDIF_MASK | S5P_CLKSRC6_AUDIO0_MASK);
	value |= (0x0<<12) | (0x7<<0);
	writel(value, S5P_CLK_SRC6);

	/* EPLL Lock Time, 300us */
	writel(((readl(S5P_EPLL_LOCK) &  ~(0xFFFF<<0)) | (0xE10<<0)), S5P_EPLL_LOCK);

	/* EPLL control setting */	
	value = readl(S5P_EPLL_CON);
	value &= !(S5P_EPLL_MASK_VSEL | S5P_EPLL_MASK_M | S5P_EPLL_MASK_P | S5P_EPLL_MASK_S);
	value |= S5P_EPLL_EN | epll_vsel << 27 | epll_m << 16 | epll_p << 8 | epll_s << 0;
	writel(value, S5P_EPLL_CON);

	/* Wait till EPLL is locked */
	/* Not sure if such while loop is good - system may hang */
        //while(!(readl(S5P_EPLL_CON)>>29)&0x1);
        mdelay(100);

	/* Set clock divider */
	writel(((readl(S5P_CLK_DIV6) & ~(0xF<<0)) | (epll_div<<0)), S5P_CLK_DIV6);


	return 0;
}

static struct snd_soc_ops smdkc110_spdif_ops = {
	.hw_params = smdkc110_spdif_hw_params,
};

static int spdif_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	
	int ret = 0;
	//printk("SPDIF Virtual Audio Codec \n");

	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;
		
	socdev->codec = codec;	

	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	codec->name = "SPDIF";
	codec->owner = THIS_MODULE;
	codec->dai = spdif_dai;
	codec->num_dai = ARRAY_SIZE(spdif_dai);
	ret = snd_soc_new_pcms(socdev, -1, NULL);

	if (ret < 0) {
		printk(KERN_ERR "spdif: failed to create pcms\n");
		return -1;
	}

    //Modified as per 2.6.29
	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "spdif: failed to register card\n");
		return -1;
	}
	return 0;
}

/* power down chip */
static int spdif_remove(struct platform_device *pdev)
{
	
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	s3cdbg("Entered %s\n", __FUNCTION__);
	
	snd_soc_free_pcms(socdev);
	//snd_soc_dapm_free(socdev);
	kfree(codec->private_data);
	kfree(codec);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_spdif = {
	.probe = 	spdif_probe,
	.remove = 	spdif_remove,
};

/*--- smdkc110 digital audio interface glue - connects codec <--> CPU ---*/
static struct snd_soc_dai_link smdkc110_dai[] = {
	{
		.name = "SPDIF",
		.stream_name = "SPDIF Playback",
		.cpu_dai = &s5p_spdif_dai,
		.codec_dai = &spdif_dai[0],
		.init = smdkc110_spdif_init,
		.ops = &smdkc110_spdif_ops,
	},
};

/*--- smdkc110 audio machine driver ---*/
//Modified as per 2.6.29
static struct snd_soc_card smdkc110 = {
	.name = "smdkc110_spdif",
	.dai_link = smdkc110_dai,
	.num_links = ARRAY_SIZE(smdkc110_dai),
	.platform = &s3c24xx_soc_platform
};

/*--- smdkc110 audio subsystem ---*/
//Modified as per 2.6.29
static struct snd_soc_device smdkc110_snd_devdata = {
	.card = &smdkc110,
	.codec_dev = &soc_codec_dev_spdif,
};

static struct platform_device *smdkc110_snd_device;

static int __init smdkc110_init(void)
{
	int ret;
	s3cdbg("Entered %s\n", __FUNCTION__);

	s3c_gpio_cfgpin (S5PC11X_GPC1(0), S3C_GPIO_SFN(3));		 //GPC1CON[0] spdif_0_out
	s3c_gpio_setpull(S5PC11X_GPC1(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin (S5PC11X_GPC1(0), S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin (S5PC11X_GPC1(1), S3C_GPIO_SFN(3));		 //GPC1CON[1] spdif_extcal
	s3c_gpio_setpull(S5PC11X_GPC1(1), S3C_GPIO_PULL_NONE);	
	s3c_gpio_setpin (S5PC11X_GPC1(1), S3C_GPIO_PULL_NONE);


	//Modified as per 2.6.29
	ret=snd_soc_register_dais(spdif_dai,ARRAY_SIZE(spdif_dai));
	if(ret){
		s3cdbg("spdif_dai registration failed");
		return ret;
	}
	smdkc110_snd_device = platform_device_alloc("soc-audio", 0);
	if (!smdkc110_snd_device){
		s3cdbg("soc-audio allocation failed");
		return -ENOMEM;
	}
	platform_set_drvdata(smdkc110_snd_device, &smdkc110_snd_devdata);
	smdkc110_snd_devdata.dev = &smdkc110_snd_device->dev;
	ret = platform_device_add(smdkc110_snd_device);
	if (ret)
		platform_device_put(smdkc110_snd_device);
	
	return ret;
}

static void __exit smdkc110_exit(void)
{
	snd_soc_unregister_dais(spdif_dai,ARRAY_SIZE(spdif_dai));  //Modified as per 2.6.29
	platform_device_unregister(smdkc110_snd_device);
}

module_init(smdkc110_init);
module_exit(smdkc110_exit);

/* Module information */
MODULE_AUTHOR("Mark Brown");
MODULE_DESCRIPTION("ALSA SoC SMDKC110 SPDIF");
MODULE_LICENSE("GPL");



