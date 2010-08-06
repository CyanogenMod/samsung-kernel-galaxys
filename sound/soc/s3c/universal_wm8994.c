/*
 * univsersal_wm8994.c
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
#include <mach/gpio-jupiter.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include "../codecs/wm8994.h"
#include "s3c-pcm-lp.h"
#include "s3c-i2s.h"

#define PLAY_51       0
#define PLAY_STEREO   1
#define PLAY_OFF      2

#define REC_MIC    0
#define REC_LINE   1
#define REC_OFF    2


extern struct s5p_pcm_pdata s3c_pcm_pdat;
extern struct s5p_i2s_pdata s3c_i2s_pdat;

#define SRC_CLK	(*s3c_i2s_pdat.p_rate)

//#define CONFIG_SND_DEBUG
#ifdef CONFIG_SND_DEBUG
#define debug_msg(x...) printk(x)
#else
#define debug_msg(x...)
#endif

static int lowpower = 0;

//static int smdkc110_play_opt;
//static int smdkc110_rec_opt;
/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS(must be a multiple of BFS)                                 XXX */
/* XXX RFS & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
int smdkc110_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	int bfs, rfs, ret;
	u32 ap_codec_clk;
	//struct clk    *clk_out,*clk_epll;

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

#ifdef CONFIG_SND_UNIVERSAL_WM8994_MASTER
	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	
	if (ret < 0)
	{
		printk(KERN_ERR "smdkc110_wm8994_hw_params : Codec DAI configuration error!\n");
		return ret;
	}

	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
        
	if (ret < 0)
	{
		printk(KERN_ERR "smdkc110_wm8994_hw_params : AP DAI configuration error!\n");
		return ret;
	}

	/* Select the AP Sysclk */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CDCLKSRC_EXT, params_rate(params), SND_SOC_CLOCK_IN);

	if (ret < 0)
	{
		printk("smdkc110_wm8994_hw_params : AP sys clock INT setting error!\n");
		return ret;
	}

        ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_I2SEXT, params_rate(params), SND_SOC_CLOCK_IN);
        if (ret < 0)
                return ret;

       switch (params_rate(params)) {
                
		case 8000:
                        ap_codec_clk = 4096000; //Shaju ..need to figure out this rate
		break;

                case 11025:
                        ap_codec_clk = 2822400; 
		break;
                case 12000:
                        ap_codec_clk = 6144000; 
		break;

                case 16000:
                        ap_codec_clk = 4096000;
		break;

                case 22050:
                        ap_codec_clk = 6144000;
		break;
                case 24000:
                        ap_codec_clk = 6144000;
		break;

                case 32000:
                        ap_codec_clk = 8192000;
		break;
                case 44100:
                        ap_codec_clk = 11289600;
		break;

                case 48000:
                        ap_codec_clk = 12288000;
		break;

                default:
                        ap_codec_clk = 11289600;
                        break;
                }

        ret = snd_soc_dai_set_sysclk(codec_dai,WM8994_SYSCLK_FLL, ap_codec_clk, 0);
        if (ret < 0)
                return ret;

#else 
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	
	if (ret < 0)
		return ret;
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);

	if (ret < 0)

		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CDCLKSRC_INT, params_rate(params), SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;
#ifdef USE_CLKAUDIO
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_CLKAUDIO, params_rate(params), SND_SOC_CLOCK_OUT);

	if (ret < 0)
	{
		printk("smdkc110_wm8994_hw_params : AP sys clock setting error!\n");
		return ret;
	}
#endif
	clk_out = clk_get(NULL,"clk_out");
        if (IS_ERR(clk_out)) {
                        printk("failed to get CLK_OUT\n");
                        return -EBUSY;
                }

	clk_epll = clk_get(NULL, "fout_epll");
        if (IS_ERR(clk_epll)) {
                printk("failed to get fout_epll\n");
		clk_put(clk_out);
                return -EBUSY;
        }

	if(clk_set_parent(clk_out, clk_epll)){
                printk("failed to set CLK_EPLL as parent of CLK_OUT\n");
		clk_put(clk_out);
		clk_put(clk_epll);
                return -EBUSY; 
        }

//	printk("..check point..CLK_OUT=0X%x\n",__raw_readl(S5P_CLK_OUT));	

	switch (params_rate(params)) {
                case 8000:
                case 16000:
                case 32000:
                case 48000:
                case 64000:
                case 96000:
                        clk_set_rate(clk_out,12288000);
                        ap_codec_clk = SRC_CLK/4;
                        break;
		case 11025:
                case 22050:
                case 44100:
                case 88200:
                default:
                        clk_set_rate(clk_out,11289600);
                        ap_codec_clk = SRC_CLK/6;
                        break;
                }


//	printk("..check point.. ap_codec_clk = %d..CLK_OUT=0X%x\n",ap_codec_clk,__raw_readl(S5P_CLK_OUT));
	//ret = snd_soc_dai_set_sysclk(codec_dai,WM8994_SYSCLK_MCLK, SRC_CLK/6, 0); /* Use MCLK = EPLL/6 for 44.1 */
	ret = snd_soc_dai_set_sysclk(codec_dai,WM8994_SYSCLK_MCLK, ap_codec_clk, 0); 
	if (ret < 0)
	{
		printk("smdkc110_wm8994_hw_params : Codec sys clock setting error!\n");
		return ret;
	}

	/* Calculate Prescalare/PLL values for supported Rates */	
	psr = SRC_CLK / rfs / params_rate(params);
	ret = SRC_CLK / rfs - psr * params_rate(params);
	
	if(ret >= params_rate(params)/2)	// round off
	   psr += 1;

	psr -= 1;
	//printk("SRC_CLK=%d PSR=%d RFS=%d BFS=%d\n", SRC_CLK, psr, rfs, bfs);

	/* Set the AP Prescalar/Pll */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_PRESCALER, psr);

	if (ret < 0)
	{
		printk("smdkc110_wm8994_hw_params : AP prescalar setting error!\n");
		return ret;
	}

	/* Set the AP RFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_MCLK, rfs);
	if (ret < 0)
	{
		printk("smdkc110_wm8994_hw_params : AP RFS setting error!\n");
		return ret;
	}

	/* Set the AP BFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_BCLK, bfs);

	if (ret < 0)
	{
		printk("smdkc110_wm8994_hw_params : AP BCLK setting error!\n");
		return ret;
	}

	clk_put(clk_epll);
	clk_put(clk_out);	
#endif
	return 0;

}


#if 0
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
#endif

static int smdkc110_wm8994_init(struct snd_soc_codec *codec)
{
#if 0
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
#endif
	return 0;
}


/* machine stream operations */
static struct snd_soc_ops smdkc110_ops = {
	.hw_params = smdkc110_hw_params,
};

/* digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link smdkc100_dai = {
	.name = "WM8994",
	.stream_name = "WM8994 HiFi Playback",
	.cpu_dai = &s3c_i2s_pdat.i2s_dai,
	.codec_dai = &wm8994_dai,
	.init = smdkc110_wm8994_init,
	.ops = &smdkc110_ops,
};

static struct snd_soc_card smdkc100 = {
	.name = "smdkc110",
	.lp_mode = 0,
	.platform = &s3c_pcm_pdat.pcm_pltfm,
	.dai_link = &smdkc100_dai,
	.num_links = 1,//ARRAY_SIZE(smdkc100_dai),
};

static struct wm8994_setup_data smdkc110_wm8994_setup = {
	/*
		The I2C address of the WM89940 is 0x34. To the I2C driver 
		the address is a 7-bit number hence the right shift .
	*/
	.i2c_address = 0x34>>1,
	.i2c_bus = 4,
};

/* audio subsystem */
static struct snd_soc_device smdkc100_snd_devdata = {
	.card = &smdkc100,
	.codec_dev = &soc_codec_dev_wm8994,
	.codec_data = &smdkc110_wm8994_setup,
};

static struct platform_device *smdkc100_snd_device;

static int __init smdkc110_audio_init(void)
{
	int ret;

	debug_msg("%s\n", __FUNCTION__);
	
	if(lowpower){ /* LPMP3 Mode doesn't support recording */
		wm8994_dai.capture.channels_min = 0;
		wm8994_dai.capture.channels_max = 0;
		smdkc100.lp_mode = 1;
	}else{
		wm8994_dai.capture.channels_min = 1;
		wm8994_dai.capture.channels_max = 2;
		smdkc100.lp_mode = 0;
	}
	s3c_pcm_pdat.set_mode(lowpower, &s3c_i2s_pdat);
	s3c_i2s_pdat.set_mode(lowpower);

	smdkc100_snd_device = platform_device_alloc("soc-audio", 0);
	if (!smdkc100_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdkc100_snd_device, &smdkc100_snd_devdata);
	smdkc100_snd_devdata.dev = &smdkc100_snd_device->dev;
	ret = platform_device_add(smdkc100_snd_device);

	if (ret)
		platform_device_put(smdkc100_snd_device);

	return ret;
}

static void __exit smdkc110_audio_exit(void)
{
	debug_msg("%s\n", __FUNCTION__);

	platform_device_unregister(smdkc100_snd_device);
}

module_init(smdkc110_audio_init);
module_exit(smdkc110_audio_exit);
module_param (lowpower, int, 0444);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC SMDKC110 WM8994");
MODULE_LICENSE("GPL");
