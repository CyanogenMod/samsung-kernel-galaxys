/*
 * smdkc110_wm8580.c
 *
 * Copyright (C) 2009, Samsung Elect. Ltd. - Jaswinder Singh <jassisinghbrar@gmail.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/io.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 
#include <plat/map-base.h>
#include <plat/regs-clock.h>

#include "../codecs/wm8580.h"
#include "s3c-pcm.h"
#include "s3c-i2s.h"

#define PLAY_51       0
#define PLAY_STEREO   1
#define PLAY_OFF      2

#define REC_MIC    0
#define REC_LINE   1
#define REC_OFF    2

#define SRC_CLK	s3c_i2s_get_clockrate()


//#define CONFIG_SND_DEBUG
#ifdef CONFIG_SND_DEBUG
#define debug_msg(x...) printk(x)
#else
#define debug_msg(x...)
#endif

static int smdkc110_play_opt;
static int smdkc110_rec_opt;

/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS(must be a multiple of BFS)                                 XXX */
/* XXX RFS & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
static int smdkc110_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	int bfs, rfs, psr, ret;

	debug_msg("%s\n", __FUNCTION__);

	/* Choose BFS and RFS values combination that is supported by
	 * both the WM8580 codec as well as the S3C AP
	 *
	 * WM8580 codec supports only S16_LE, S20_3LE, S24_LE & S32_LE.
	 * S3C AP supports only S8, S16_LE & S24_LE.
	 * We implement all for completeness but only S16_LE & S24_LE bit-lengths 
	 * are possible for this AP-Codec combination.
	 */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		bfs = 16;
		rfs = 256;		/* Can take any RFS value for AP */
 		break;
 	case SNDRV_PCM_FORMAT_S16_LE:
		bfs = 32;
		rfs = 256;		/* Can take any RFS value for AP */
 		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
 	case SNDRV_PCM_FORMAT_S24_LE:
		bfs = 48;
		rfs = 512;		/* B'coz 48-BFS needs atleast 512-RFS acc to *S5P6440* UserManual */
 		break;
 	case SNDRV_PCM_FORMAT_S32_LE:	/* Impossible, as the AP doesn't support 64fs or more BFS */
	default:
		return -EINVAL;
 	}
 
	/* Select the AP Sysclk */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CDCLKSRC_INT, params_rate(params), SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

#ifdef USE_CLKAUDIO
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_CLKAUDIO, params_rate(params), SND_SOC_CLOCK_OUT);
#else
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_PCLK, 0, SND_SOC_CLOCK_OUT);
#endif
	if (ret < 0)
		return ret;

	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set the AP RFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_MCLK, rfs);
	if (ret < 0)
		return ret;

	/* Set the AP BFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_BCLK, bfs);
	if (ret < 0)
		return ret;

	switch (params_rate(params)) {
	case 8000:
	case 11025:
	case 16000:
	case 22050:
	case 32000:
	case 44100: 
	case 48000:
	case 64000:
	case 88200:
	case 96000:
		psr = SRC_CLK / rfs / params_rate(params);
		ret = SRC_CLK / rfs - psr * params_rate(params);
		if(ret >= params_rate(params)/2)	// round off
		   psr += 1;
		psr -= 1;
		break;
	default:
		return -EINVAL;
	}

	//printk("SRC_CLK=%d PSR=%d RFS=%d BFS=%d\n", SRC_CLK, psr, rfs, bfs);

	/* Set the AP Prescalar */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_PRESCALER, psr);
	if (ret < 0)
		return ret;

	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set the Codec BCLK(no option to set the MCLK) */
	/* See page 2 and 53 of Wm8580 Manual */
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_MCLK, WM8580_CLKSRC_MCLK); /* Use MCLK provided by CPU i/f */
	if (ret < 0)
		return ret;
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_DAC_CLKSEL, WM8580_CLKSRC_MCLK); /* Fig-26 Pg-43 */
	if (ret < 0)
		return ret;
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_CLKOUTSRC, WM8580_CLKSRC_NONE); /* Pg-58 */
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * WM8580 DAI operations.
 */
static struct snd_soc_ops smdkc110_ops = {
	.hw_params = smdkc110_hw_params,
};

static void smdkc110_ext_control(struct snd_soc_codec *codec)
{
	debug_msg("%s\n", __FUNCTION__);

	/* set up jack connection */
	if(smdkc110_play_opt == PLAY_51){
		snd_soc_dapm_enable_pin(codec, "Front-L/R");
		snd_soc_dapm_enable_pin(codec, "Center/Sub");
		snd_soc_dapm_enable_pin(codec, "Rear-L/R");
	}else if(smdkc110_play_opt == PLAY_STEREO){
		snd_soc_dapm_enable_pin(codec, "Front-L/R");
		snd_soc_dapm_disable_pin(codec, "Center/Sub");
		snd_soc_dapm_disable_pin(codec, "Rear-L/R");
	}else{
		snd_soc_dapm_disable_pin(codec, "Front-L/R");
		snd_soc_dapm_disable_pin(codec, "Center/Sub");
		snd_soc_dapm_disable_pin(codec, "Rear-L/R");
	}

	if(smdkc110_rec_opt == REC_MIC){
		snd_soc_dapm_enable_pin(codec, "MicIn");
		snd_soc_dapm_disable_pin(codec, "LineIn");
	}else if(smdkc110_rec_opt == REC_LINE){
		snd_soc_dapm_disable_pin(codec, "MicIn");
		snd_soc_dapm_enable_pin(codec, "LineIn");
	}else{
		snd_soc_dapm_disable_pin(codec, "MicIn");
		snd_soc_dapm_disable_pin(codec, "LineIn");
	}

	/* signal a DAPM event */
	snd_soc_dapm_sync(codec);
}

static int smdkc110_get_pt(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	debug_msg("%s\n", __FUNCTION__);

	ucontrol->value.integer.value[0] = smdkc110_play_opt;
	return 0;
}

static int smdkc110_set_pt(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	debug_msg("%s\n", __FUNCTION__);

	if(smdkc110_play_opt == ucontrol->value.integer.value[0])
		return 0;

	smdkc110_play_opt = ucontrol->value.integer.value[0];
	smdkc110_ext_control(codec);
	return 1;
}

static int smdkc110_get_cs(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	debug_msg("%s\n", __FUNCTION__);
	ucontrol->value.integer.value[0] = smdkc110_rec_opt;
	return 0;
}

static int smdkc110_set_cs(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec =  snd_kcontrol_chip(kcontrol);
	debug_msg("%s\n", __FUNCTION__);

	if(smdkc110_rec_opt == ucontrol->value.integer.value[0])
		return 0;

	smdkc110_rec_opt = ucontrol->value.integer.value[0];
	smdkc110_ext_control(codec);
	return 1;
}

/* smdkc110 card dapm widgets */
static const struct snd_soc_dapm_widget wm8580_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Front-L/R", NULL),
	SND_SOC_DAPM_HP("Center/Sub", NULL),
	SND_SOC_DAPM_HP("Rear-L/R", NULL),
	SND_SOC_DAPM_MIC("MicIn", NULL),
	SND_SOC_DAPM_LINE("LineIn", NULL),
};

/* smdk machine audio map (connections to the codec pins) */
static const struct snd_soc_dapm_route audio_map[] = {
	/* Front Left/Right are fed VOUT1L/R */
	{"Front-L/R", NULL, "VOUT1L"},
	{"Front-L/R", NULL, "VOUT1R"},

	/* Center/Sub are fed VOUT2L/R */
	{"Center/Sub", NULL, "VOUT2L"},
	{"Center/Sub", NULL, "VOUT2R"},

	/* Rear Left/Right are fed VOUT3L/R */
	{"Rear-L/R", NULL, "VOUT3L"},
	{"Rear-L/R", NULL, "VOUT3R"},

	/* MicIn feeds AINL */
	{"AINL", NULL, "MicIn"},

	/* LineIn feeds AINL/R */
	{"AINL", NULL, "LineIn"},
	{"AINR", NULL, "LineIn"},
};

static const char *play_function[] = {
	[PLAY_51]     = "5.1 Chan",
	[PLAY_STEREO] = "Stereo",
	[PLAY_OFF]    = "Off",
};

static const char *rec_function[] = {
	[REC_MIC] = "Mic",
	[REC_LINE] = "Line",
	[REC_OFF] = "Off",
};

static const struct soc_enum smdkc110_enum[] = {
	SOC_ENUM_SINGLE_EXT(3, play_function),
	SOC_ENUM_SINGLE_EXT(3, rec_function),
};

static const struct snd_kcontrol_new wm8580_smdkc110_controls[] = {
	SOC_ENUM_EXT("Playback Target", smdkc110_enum[0], smdkc110_get_pt,
		smdkc110_set_pt),
	SOC_ENUM_EXT("Capture Source", smdkc110_enum[1], smdkc110_get_cs,
		smdkc110_set_cs),
};

static int smdkc110_wm8580_init(struct snd_soc_codec *codec)
{
	int i, err;

	debug_msg("%s\n", __FUNCTION__);

	/* Add smdkc110 specific controls */
	for (i = 0; i < ARRAY_SIZE(wm8580_smdkc110_controls); i++) {
		err = snd_ctl_add(codec->card,
			snd_soc_cnew(&wm8580_smdkc110_controls[i], codec, NULL));
		if (err < 0)
			return err;
	}

	/* Add smdkc110 specific widgets */
	snd_soc_dapm_new_controls(codec, wm8580_dapm_widgets,
				  ARRAY_SIZE(wm8580_dapm_widgets));

	/* Set up smdkc110 specific audio path audio_map */
	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));

	/* No jack detect - mark all jacks as enabled */
	for (i = 0; i < ARRAY_SIZE(wm8580_dapm_widgets); i++)
		snd_soc_dapm_enable_pin(codec, wm8580_dapm_widgets[i].name);

	/* Setup Default Route */
	smdkc110_play_opt = PLAY_STEREO;
	smdkc110_rec_opt = REC_LINE;
	smdkc110_ext_control(codec);

	return 0;
}

static struct snd_soc_dai_link smdkc110_dai[] = {
{
	.name = "WM8580",
	.stream_name = "WM8580 HiFi Playback",
	.cpu_dai = &s3c_i2s_dai,
	.codec_dai = &wm8580_dai[WM8580_DAI_PAIFRX],
	.init = smdkc110_wm8580_init,
	.ops = &smdkc110_ops,
},
};

static struct snd_soc_card smdkc110 = {
	.name = "smdkc110",
	.platform = &s3c24xx_soc_platform,
	.dai_link = smdkc110_dai,
	.num_links = ARRAY_SIZE(smdkc110_dai),
};

static struct wm8580_setup_data smdkc110_wm8580_setup = {
	.i2c_address = 0x1b,
};

static struct snd_soc_device smdkc110_snd_devdata = {
	.card = &smdkc110,
	.codec_dev = &soc_codec_dev_wm8580,
	.codec_data = &smdkc110_wm8580_setup,
};

static struct platform_device *smdkc110_snd_device;

static int __init smdkc110_audio_init(void)
{
	int ret;

	debug_msg("%s\n", __FUNCTION__);

	smdkc110_snd_device = platform_device_alloc("soc-audio", 0);
	if (!smdkc110_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdkc110_snd_device, &smdkc110_snd_devdata);
	smdkc110_snd_devdata.dev = &smdkc110_snd_device->dev;
	ret = platform_device_add(smdkc110_snd_device);

	if (ret)
		platform_device_put(smdkc110_snd_device);
	
	return ret;
}

static void __exit smdkc110_audio_exit(void)
{
	debug_msg("%s\n", __FUNCTION__);

	platform_device_unregister(smdkc110_snd_device);
}

module_init(smdkc110_audio_init);
module_exit(smdkc110_audio_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC SMDKC110 WM8580");
MODULE_LICENSE("GPL");
