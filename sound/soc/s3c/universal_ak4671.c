/*
 * universal_ak4671.c
 *
 * Copyright (C) 2009 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "../codecs/ak4671.h"
#include "../codecs/max9877.h"
#include "s3c-pcm-lp.h"
#include "s3c-i2s.h"

extern struct s5pc1xx_pcm_pdata s3c_pcm_pdat;
extern struct s5pc1xx_i2s_pdata s3c_i2s_pdat;

static int lowpower = 0;

static const struct snd_soc_dapm_widget universal_dapm_widgets[] = {
	/* TODO */
};

static const struct snd_soc_dapm_route dapm_routes[] = {
	/* TODO */
};

static int universal_ak4671_init(struct snd_soc_codec *codec)
{
	/* set up codec pins */
	/* TODO */

	/* add universal specific widgets */
	snd_soc_dapm_new_controls(codec, universal_dapm_widgets,
			ARRAY_SIZE(universal_dapm_widgets));

	/* set up universal specific audio routes */
	snd_soc_dapm_add_routes(codec, dapm_routes, ARRAY_SIZE(dapm_routes));

	/* add max9877 amp specific controls */
	max9877_add_controls(codec);

	snd_soc_dapm_sync(codec);

	return 0;
}

static int universal_hifi_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	/* TODO */

	return 0;
}

static struct snd_soc_ops universal_hifi_ops = {
	.hw_params = universal_hifi_hw_params,

	/* TODO */
};

static struct snd_soc_dai_link universal_dai[] = {
{
	.name = "AK4671",
	.stream_name = "AK4671 HiFi",
	.cpu_dai = &s3c_i2s_pdat.i2s_dai,
	.codec_dai = &ak4671_dai,
	.init = universal_ak4671_init,
	.ops = &universal_hifi_ops,
},
};

static struct snd_soc_card universal = {
	.name = "universal",
	.platform = &s3c_pcm_pdat.pcm_pltfm,
	.dai_link = universal_dai,
	.num_links = ARRAY_SIZE(universal_dai),
};

static struct snd_soc_device universal_snd_devdata = {
	.card = &universal,
	.codec_dev = &soc_codec_dev_ak4671,
};

static struct platform_device *universal_snd_device;

static int __init universal_init(void)
{
	int ret;

	s3c_pcm_pdat.set_mode(lowpower, &s3c_i2s_pdat);
	s3c_i2s_pdat.set_mode(lowpower);

	if(lowpower){ /* LPMP3 Mode doesn't support recording */
		ak4671_dai.capture.channels_min = 0;
		ak4671_dai.capture.channels_max = 0;
	}else{
		ak4671_dai.capture.channels_min = 2;
		ak4671_dai.capture.channels_max = 2;
	}

	universal_snd_device = platform_device_alloc("soc-audio", 0);
	if (!universal_snd_device)
		return -ENOMEM;

	platform_set_drvdata(universal_snd_device, &universal_snd_devdata);
	universal_snd_devdata.dev = &universal_snd_device->dev;
	ret = platform_device_add(universal_snd_device);

	if (ret)
		platform_device_put(universal_snd_device);

	return ret;
}

static void __exit universal_exit(void)
{
	platform_device_unregister(universal_snd_device);
}

module_init(universal_init);
module_exit(universal_exit);

module_param (lowpower, int, 0444);

/* Module information */
MODULE_DESCRIPTION("ASoC AK4671 codec driver");
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_LICENSE("GPL");
