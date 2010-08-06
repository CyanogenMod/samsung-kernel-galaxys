/*
 * smdkc100_spdif.c
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
	 
#include "s5pc1xx-spdif.h"
#include "s5pc1xx-pcm.h"

#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

struct s5pc1xx_spdif_info s5pc1xx_spdif;

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

static int smdkc100_spdif_init(struct snd_soc_codec *codec)
{
	int i;

	//printk("\n Enter the smdkc100_spdif_init \n");
	/* Add smdkc100 specific widgets */
	snd_soc_dapm_new_controls(codec, spdif_dapm_widgets,ARRAY_SIZE(spdif_dapm_widgets));

	/* set up smdkc100 specific audio paths */
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



static int smdkc100_spdif_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	//configure gpio for spdif
	s3cdbg("Entered %s, rate = %d\n", __FUNCTION__, params_rate(params));
	s3c_gpio_cfgpin(S5PC1XX_GPG3(5), S3C_GPIO_SFN(5));		 //GPG3CON[5] spdif_0_out
	s3c_gpio_setpull(S5PC1XX_GPG3(5), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(S5PC1XX_GPG3(6), S3C_GPIO_SFN(5));		 //GPG3CON[5] spdif_extcal
	s3c_gpio_setpull(S5PC1XX_GPG3(6), S3C_GPIO_PULL_UP);	

	return 0;
}

static struct snd_soc_ops smdkc100_spdif_ops = {
	.hw_params = smdkc100_spdif_hw_params,
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

/*--- smdkc100 digital audio interface glue - connects codec <--> CPU ---*/
static struct snd_soc_dai_link smdkc100_dai[] = {
	{
		.name = "SPDIF",
		.stream_name = "SPDIF Playback",
		.cpu_dai = &s5pc1xx_spdif_dai,
		.codec_dai = &spdif_dai[0],
		.init = smdkc100_spdif_init,
		.ops = &smdkc100_spdif_ops,
	},
};

/*--- smdkc100 audio machine driver ---*/
//Modified as per 2.6.29
static struct snd_soc_card smdkc100 = {
	.name = "smdkc100_spdif",
	.dai_link = smdkc100_dai,
	.num_links = ARRAY_SIZE(smdkc100_dai),
	.platform = &s3c24xx_soc_platform
};

/*--- smdkc100 audio subsystem ---*/
//Modified as per 2.6.29
static struct snd_soc_device smdkc100_snd_devdata = {
	.card = &smdkc100,
	.codec_dev = &soc_codec_dev_spdif,
};

static struct platform_device *smdkc100_snd_device;

static int __init smdkc100_init(void)
{
	int ret;
	s3cdbg("Entered %s\n", __FUNCTION__);
	//Modified as per 2.6.29
	ret=snd_soc_register_dais(spdif_dai,ARRAY_SIZE(spdif_dai));
	if(ret){
		s3cdbg("spdif_dai registration failed");
		return ret;
	}
	smdkc100_snd_device = platform_device_alloc("soc-audio", 0);
	if (!smdkc100_snd_device){
		s3cdbg("soc-audio allocation failed");
		return -ENOMEM;
	}
	platform_set_drvdata(smdkc100_snd_device, &smdkc100_snd_devdata);
	smdkc100_snd_devdata.dev = &smdkc100_snd_device->dev;
	ret = platform_device_add(smdkc100_snd_device);
	if (ret)
		platform_device_put(smdkc100_snd_device);
	
	return ret;
}

static void __exit smdkc100_exit(void)
{
	snd_soc_unregister_dais(spdif_dai,ARRAY_SIZE(spdif_dai));  //Modified as per 2.6.29
	platform_device_unregister(smdkc100_snd_device);
}

module_init(smdkc100_init);
module_exit(smdkc100_exit);

/* Module information */
MODULE_AUTHOR("Mark Brown");
MODULE_DESCRIPTION("ALSA SoC SMDKC100 SPDIF");
MODULE_LICENSE("GPL");



