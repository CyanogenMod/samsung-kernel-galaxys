/*
 * smdkc100_wm8580.c
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
#include "s3c-pcm-lp.h"
#include "s3c-i2s.h"

/* SMDKC100 has 12MHZ Osc attached to WM8580 */
#define SMDKC100_WM8580_OSC_FREQ (12000000)

#define PLAY_51       0
#define PLAY_STEREO   1
#define PLAY_OFF      2

#define REC_MIC    0
#define REC_LINE   1
#define REC_OFF    2

extern struct s5p_pcm_pdata s3c_pcm_pdat;
extern struct s5p_i2s_pdata s3c_i2s_pdat;

#define SRC_CLK	(*s3c_i2s_pdat.p_rate)

static int lowpower = 0;
static int smdkc100_play_opt;
static int smdkc100_rec_opt;

/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS(must be a multiple of BFS)                                 XXX */
/* XXX RFS & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
static int smdkc100_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	int bfs, rfs, psr, ret;
	unsigned int pll_out;

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
 
	switch (params_rate(params)) {
	case 8000:
		/* Can't have 2.048MHz from 12MHz by PLLA of WM8580 */
		pll_out = 8192000 / 4;
#ifdef CONFIG_SND_WM8580_MASTER
		rfs = 512;
#endif
		break;
	case 16000:
		pll_out = 8192000 / 2;
		break;
	case 32000:
		pll_out = 8192000;
		break;
	case 64000:
		/* Can't have 16.384MHz from 12MHz by PLLA of WM8580 */
		pll_out = 24576000;
#ifdef CONFIG_SND_WM8580_MASTER
		rfs = 384;
#endif
		break;
	case 11025:
		/* Can't have 2.822MHz from 12MHz by PLLA of WM8580 */
		pll_out = 11289600 / 4;
#ifdef CONFIG_SND_WM8580_MASTER
		rfs = 512;
#endif
		break;
	case 22050:
		pll_out = 11289600 / 2;
		break;
	case 44100: 
		pll_out = 11289600;
		break;
	case 88200:
		pll_out = 11289600 * 2;
		break;
	case 48000:
		pll_out = 12288000;
		break;
	case 96000:
		pll_out = 12288000 * 2;
		break;
	default:
		return -EINVAL;
	}
	ret = rfs / 256;
	pll_out *= ret;

	/* Set the Codec DAI configuration */
#ifdef CONFIG_SND_WM8580_MASTER
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
#else
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
#endif
	if (ret < 0)
		return ret;

	/* Set the AP DAI configuration */
#ifdef CONFIG_SND_WM8580_MASTER
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
#else
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
#endif
	if (ret < 0)
		return ret;

	/* Select the AP Sysclk */
#ifdef CONFIG_SND_WM8580_MASTER
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CDCLKSRC_EXT, params_rate(params), SND_SOC_CLOCK_IN);
#else
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CDCLKSRC_INT, params_rate(params), SND_SOC_CLOCK_OUT);
#endif
	if (ret < 0)
		return ret;

#ifdef USE_CLKAUDIO
#ifdef CONFIG_SND_WM8580_MASTER
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_I2SEXT, params_rate(params), SND_SOC_CLOCK_IN);
#else
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_CLKAUDIO, params_rate(params), SND_SOC_CLOCK_OUT);
#endif
#else
#ifdef CONFIG_SND_WM8580_MASTER
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_SLVPCLK, 0, SND_SOC_CLOCK_IN);
#else
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_PCLK, 0, SND_SOC_CLOCK_OUT);
#endif
#endif
	if (ret < 0)
		return ret;

	/* Set the Codec BCLK(no option to set the MCLK) */
	/* See page 2 and 53 of Wm8580 Manual */
#ifdef CONFIG_SND_WM8580_MASTER 
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_MCLK, WM8580_CLKSRC_PLLA); /* Use PLLACLK in AP-Slave Mode */
#else
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_MCLK, WM8580_CLKSRC_MCLK); /* Use MCLK provided by CPU i/f */
#endif
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_DAC_CLKSEL, WM8580_CLKSRC_MCLK); /* Fig-26 Pg-43 */
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_DAC_CLKSEL, WM8580_CLKSRC_MCLK); /* Fig-26 Pg-43 */
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_CLKOUTSRC, WM8580_CLKSRC_NONE); /* Pg-58 */
	if (ret < 0)
		return ret;

#ifdef CONFIG_SND_WM8580_MASTER
	/* Set the AP Prescalar */
	ret = snd_soc_dai_set_pll(codec_dai, WM8580_PLLA, SMDKC100_WM8580_OSC_FREQ, pll_out);
#else
	psr = SRC_CLK / rfs / params_rate(params);
	ret = SRC_CLK / rfs - psr * params_rate(params);
	if(ret >= params_rate(params)/2)	// round off
	   psr += 1;
	psr -= 1;
	//printk("SRC_CLK=%d PSR=%d RFS=%d BFS=%d\n", SRC_CLK, psr, rfs, bfs);

	/* Set the AP Prescalar */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_PRESCALER, psr);
#endif
	if (ret < 0)
		return ret;

#ifdef CONFIG_SND_WM8580_MASTER
	/* Set the WM8580 RFS */
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_MCLKRATIO, rfs);
#else
	/* Set the AP RFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_MCLK, rfs);
#endif
	if (ret < 0)
		return ret;

#ifdef CONFIG_SND_WM8580_MASTER
	/* Set the WM8580 BFS */
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_BCLKRATIO, bfs);
#else
	/* Set the AP BFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_BCLK, bfs);
#endif
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * WM8580 DAI operations.
 */
static struct snd_soc_ops smdkc100_ops = {
	.hw_params = smdkc100_hw_params,
};

static void smdkc100_ext_control(struct snd_soc_codec *codec)
{
	/* set up jack connection */
	if(smdkc100_play_opt == PLAY_51){
		snd_soc_dapm_enable_pin(codec, "Front-L/R");
		snd_soc_dapm_enable_pin(codec, "Center/Sub");
		snd_soc_dapm_enable_pin(codec, "Rear-L/R");
	}else if(smdkc100_play_opt == PLAY_STEREO){
		snd_soc_dapm_enable_pin(codec, "Front-L/R");
		snd_soc_dapm_disable_pin(codec, "Center/Sub");
		snd_soc_dapm_disable_pin(codec, "Rear-L/R");
	}else{
		snd_soc_dapm_disable_pin(codec, "Front-L/R");
		snd_soc_dapm_disable_pin(codec, "Center/Sub");
		snd_soc_dapm_disable_pin(codec, "Rear-L/R");
	}

	if(smdkc100_rec_opt == REC_MIC){
		snd_soc_dapm_enable_pin(codec, "MicIn");
		snd_soc_dapm_disable_pin(codec, "LineIn");
	}else if(smdkc100_rec_opt == REC_LINE){
		snd_soc_dapm_disable_pin(codec, "MicIn");
		snd_soc_dapm_enable_pin(codec, "LineIn");
	}else{
		snd_soc_dapm_disable_pin(codec, "MicIn");
		snd_soc_dapm_disable_pin(codec, "LineIn");
	}

	/* signal a DAPM event */
	snd_soc_dapm_sync(codec);
}

static int smdkc100_get_pt(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = smdkc100_play_opt;
	return 0;
}

static int smdkc100_set_pt(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if(smdkc100_play_opt == ucontrol->value.integer.value[0])
		return 0;

	smdkc100_play_opt = ucontrol->value.integer.value[0];
	smdkc100_ext_control(codec);
	return 1;
}

static int smdkc100_get_cs(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = smdkc100_rec_opt;
	return 0;
}

static int smdkc100_set_cs(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec =  snd_kcontrol_chip(kcontrol);

	if(smdkc100_rec_opt == ucontrol->value.integer.value[0])
		return 0;

	smdkc100_rec_opt = ucontrol->value.integer.value[0];
	smdkc100_ext_control(codec);
	return 1;
}

/* smdkc100 card dapm widgets */
static const struct snd_soc_dapm_widget wm8580_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Front-L/R", NULL),
	SND_SOC_DAPM_HP("Center/Sub", NULL),
	SND_SOC_DAPM_HP("Rear-L/R", NULL),
	SND_SOC_DAPM_MIC("MicIn", NULL),
	SND_SOC_DAPM_LINE("LineIn", NULL),
};

/* smdk card audio map (connections to the codec pins) */
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

static const struct soc_enum smdkc100_enum[] = {
	SOC_ENUM_SINGLE_EXT(3, play_function),
	SOC_ENUM_SINGLE_EXT(3, rec_function),
};

static const struct snd_kcontrol_new wm8580_smdkc100_controls[] = {
	SOC_ENUM_EXT("Playback Target", smdkc100_enum[0], smdkc100_get_pt,
		smdkc100_set_pt),
	SOC_ENUM_EXT("Capture Source", smdkc100_enum[1], smdkc100_get_cs,
		smdkc100_set_cs),
};

static int smdkc100_wm8580_init(struct snd_soc_codec *codec)
{
	int i, err;

	/* Add smdkc100 specific controls */
	for (i = 0; i < ARRAY_SIZE(wm8580_smdkc100_controls); i++) {
		err = snd_ctl_add(codec->card,
			snd_soc_cnew(&wm8580_smdkc100_controls[i], codec, NULL));
		if (err < 0)
			return err;
	}

	/* Add smdkc100 specific widgets */
	snd_soc_dapm_new_controls(codec, wm8580_dapm_widgets,
				  ARRAY_SIZE(wm8580_dapm_widgets));

	/* Set up smdkc100 specific audio path audio_map */
	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));

	/* No jack detect - mark all jacks as enabled */
	for (i = 0; i < ARRAY_SIZE(wm8580_dapm_widgets); i++)
		snd_soc_dapm_enable_pin(codec, wm8580_dapm_widgets[i].name);

	/* Setup Default Route */
	smdkc100_play_opt = PLAY_STEREO;
	smdkc100_rec_opt = REC_LINE;
	smdkc100_ext_control(codec);

	return 0;
}

static struct snd_soc_dai_link smdkc100_dai[] = {
{
	.name = "WM8580",
	.stream_name = "WM8580 HiFi Playback",
	.cpu_dai = &s3c_i2s_pdat.i2s_dai,
	.codec_dai = &wm8580_dai[WM8580_DAI_PAIFRX],
	.init = smdkc100_wm8580_init,
	.ops = &smdkc100_ops,
},
};

static struct snd_soc_card smdkc100 = {
	.name = "smdkc100",
	.lp_mode = 0,
	.platform = &s3c_pcm_pdat.pcm_pltfm,
	.dai_link = smdkc100_dai,
	.num_links = ARRAY_SIZE(smdkc100_dai),
};

static struct wm8580_setup_data smdkc100_wm8580_setup = {
	.i2c_address = 0x1b,
};

static struct snd_soc_device smdkc100_snd_devdata = {
	.card = &smdkc100,
	.codec_dev = &soc_codec_dev_wm8580,
	.codec_data = &smdkc100_wm8580_setup,
};

static struct platform_device *smdkc100_snd_device;

static int __init smdkc100_audio_init(void)
{
	int ret;

	if(lowpower){ /* LPMP3 Mode doesn't support recording */
		wm8580_dai[0].capture.channels_min = 0;
		wm8580_dai[0].capture.channels_max = 0;
		smdkc100.lp_mode = 1;
	}else{
		wm8580_dai[0].capture.channels_min = 2;
		wm8580_dai[0].capture.channels_max = 2;
		smdkc100.lp_mode = 0;
	}
	printk("In %s function\n",__FUNCTION__);
	s3c_pcm_pdat.set_mode(lowpower, &s3c_i2s_pdat);
	s3c_i2s_pdat.set_mode(lowpower);

	smdkc100_snd_device = platform_device_alloc("soc-audio", 0);
	if (!smdkc100_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdkc100_snd_device, &smdkc100_snd_devdata);
	smdkc100_snd_devdata.dev = &smdkc100_snd_device->dev;
	ret = platform_device_add(smdkc100_snd_device);
	printk("In %s function %d line\n",__FUNCTION__,__LINE__);

	if (ret)
		platform_device_put(smdkc100_snd_device);
	printk("In %s function %d l\n",__FUNCTION__,__LINE__);
	
#ifdef CONFIG_SND_WM8580_MASTER
	s3cdbg("WM8580 is I2S Master\n");
#else
	s3cdbg("S5P is I2S Master\n");
#endif

	return ret;
}

static void __exit smdkc100_audio_exit(void)
{
	platform_device_unregister(smdkc100_snd_device);
}

module_init(smdkc100_audio_init);
module_exit(smdkc100_audio_exit);

module_param (lowpower, int, 0444);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC SMDKC100 WM8580");
MODULE_LICENSE("GPL");
