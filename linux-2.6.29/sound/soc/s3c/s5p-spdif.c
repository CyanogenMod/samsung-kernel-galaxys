/*
 * s5pc100-spdif.c  --  
 *
 * (c) 2006 Wolfson Microelectronics PLC.
 * Graeme Gregory graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * (c) 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Ryu Euiyoul <ryu.real@gmail.com>
 *
 * Copyright (C) 2009, Kwak Hyun-Min<hyunmin.kwak@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *
 *  Revision history
 *    1th April 2009   Initial version.
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
#include <plat/regs-spdif.h>	//add for spdifs
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-g3.h>
#include <plat/regs-clock.h>
 
#include <mach/audio.h>
#include <mach/dma.h>
#include <mach/s3c-dma.h>
 
#include <plat/regs-clock.h>
#include "s5p-spdif.h"
#include "s3c-pcm.h"
 
#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif
 
 
 /* used to disable sysclk if external crystal is used */
 static int extclk = 0;
 module_param(extclk, int, 0);
 MODULE_PARM_DESC(extclk, "set to 1 to disable s3c24XX i2s sysclk");
 
 static struct s3c2410_dma_client s5p_spdif_dma_client_out = {
	 .name = "SPDIF out"
 };
 
 static struct s3c24xx_pcm_dma_params s5p_spdif_pcm_stereo_out = {
	 .client	 = &s5p_spdif_dma_client_out,
	 .channel	 = DMACH_SPDIF_OUT,					 // to do: need to modify for spdif
	 .dma_addr	 = S5P_PA_SPDIF + S5PC1XX_SPDIF_SPDDAT, 			 // to do: need to modify for spdif
	 .dma_size	 = 2,							 // to do: need to modify for spdif 
 };
  
 
 // to do: need to modify for spdif
 static void s5p_spdif_snd_txctrl(int on)
 {

	u32 value;
	 s3cdbg("Entered %s : on = %d \n", __FUNCTION__, on);
	 if (on) {
		 //writel(S5PC1XX_SPDIF_SPDCLKCON_SPDIFOUT_POWER_OFF, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCLKCON); 		 // sichoi
		 //writel(S5PC1XX_SPDIF_SPDCLKCON_MAIN_AUDIO_CLK_INT |
		 //	S5PC1XX_SPDIF_SPDCLKCON_SPDIFOUT_POWER_ON, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCLKCON);	 // SPDIFOUT clock power on
	
		value = readl(s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);
		value &= ~ ((0x7<<19) | (0x3<<17));
		value |= (0x7<<19) | (0x0<<17);
		writel(value, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);	
 
		// spdcon = readl(s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON) & ~(0xf<<19|0x3<<17);
		// writel(spdcon | S5PC1XX_SPDIF_SPDCON_FIFO_LEVEL_THRESHOLD_7,  s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);		 // DMA
	 } else {
		 writel(S5PC1XX_SPDIF_SPDCON_SOFTWARE_RESET_EN, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);	 // Sw Reset
		 mdelay(100);
		 writel(S5PC1XX_SPDIF_SPDCLKCON_SPDIFOUT_POWER_OFF, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCLKCON);	 // SPDIFOUT clock power off
	 }

 }
 
 //Modified as per 2.6.29
 static int s5p_spdif_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,struct snd_soc_dai *dai)
 {
	 struct snd_soc_pcm_runtime *rtd = substream->private_data;
	 int sampling_freq;
	
	u32 value;
	//u32 epll_vsel, epll_m, epll_p, epll_s, epll_div;
 
	 s3cdbg("Entered %s, rate = %d\n", __FUNCTION__, params_rate(params));
	 if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		 rtd->dai->cpu_dai->dma_data = &s5p_spdif_pcm_stereo_out;
	 } else {
	 }

	/* The values of epll_vsel, epll_m, epll_p, epll_s and epll_div are taken from firmware code. */
 	 switch (params_rate(params)) {
		case 32000:
			sampling_freq = S5PC1XX_SPDIF_SPDCSTAS_SAMPLING_FREQ_32;
			break;
		case 44100:
			sampling_freq = S5PC1XX_SPDIF_SPDCSTAS_SAMPLING_FREQ_44_1;
			break;
		case 48000:
			sampling_freq = S5PC1XX_SPDIF_SPDCSTAS_SAMPLING_FREQ_48;
			break;
		case 96000:
			sampling_freq = S5PC1XX_SPDIF_SPDCSTAS_SAMPLING_FREQ_96;
			break;
		default:
			sampling_freq = S5PC1XX_SPDIF_SPDCSTAS_SAMPLING_FREQ_44_1;
			break;
	}

	/* SPDIF S/W Reset */
	writel(S5PC1XX_SPDIF_SPDCON_SOFTWARE_RESET_EN, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);	 // Sw Reset

	/* SPDIF Power on and set internal clock as main clock  */
	value = readl(s5p_spdif.regs + S5PC1XX_SPDIF_SPDCLKCON);
	value &= ~ ( (0x1<<0) | (0x1<<2) );
	value |= S5PC1XX_SPDIF_SPDCLKCON_SPDIFOUT_POWER_ON | S5PC1XX_SPDIF_SPDCLKCON_MAIN_AUDIO_CLK_INT;
	writel(value, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCLKCON);
	
	 mdelay(100);
	// Clear Interrupt
	 writel(S5PC1XX_SPDIF_SPDCON_FIFO_LEVEL_INT_STATUS_PENDING|
	 	S5PC1XX_SPDIF_SPDCON_USER_DATA_INT_STATUS_PENDING |
	 	S5PC1XX_SPDIF_SPDCON_BUFFER_EMPTY_INT_STATUS_PENDING |
	 	S5PC1XX_SPDIF_SPDCON_STREAM_END_INT_STATUS_PENDING, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON); 
	 	
	 //set SPDIF Control Register
	 writel(S5PC1XX_SPDIF_SPDCON_FIFO_LEVEL_THRESHOLD_4 |
		S5PC1XX_SPDIF_SPDCON_FIFO_LEVEL_INT_ENABLE |	 
		S5PC1XX_SPDIF_SPDCON_ENDIAN_FORMAT_BIG |
		S5PC1XX_SPDIF_SPDCON_USER_DATA_ATTACH_AUDIO_DATA |	 // user data attach on  
		S5PC1XX_SPDIF_SPDCON_USER_DATA_INT_ENABLE|	  
		S5PC1XX_SPDIF_SPDCON_BUFFER_EMPTY_INT_ENABLE|	 
		S5PC1XX_SPDIF_SPDCON_STREAM_END_INT_ENABLE |	 
		S5PC1XX_SPDIF_SPDCON_SOFTWARE_RESET_NO |
		S5PC1XX_SPDIF_SPDCON_MAIN_AUDIO_CLK_FREQ_512 | 
		//S5PC1XX_SPDIF_SPDCON_MAIN_AUDIO_CLK_FREQ_384 |
		S5PC1XX_SPDIF_SPDCON_PCM_DATA_SIZE_16 |
		S5PC1XX_SPDIF_SPDCON_PCM, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);	
		 
	 // Set SPDIFOUT Burst Status Register
	 writel(S5PC1XX_SPDIF_SPDBSTAS_BURST_DATA_LENGTH(768)|		 	// Elementary stream size in number of bits
		 S5PC1XX_SPDIF_SPDBSTAS_BITSTREAM_NUMBER(0)| 	 		// Bitstream number set to 0
		 S5PC1XX_SPDIF_SPDBSTAS_DATA_TYPE_DEP_INFO |			// Data type dependent information
		 S5PC1XX_SPDIF_SPDBSTAS_ERROR_FLAG_VALID |		 	// Error flag indicating a valid burst_payload
		 //S5PC1XX_SPDIF_SPDBSTAS_COMPRESSED_DATA_TYPE_NULL,		// Compressed data type is Null Data ?
		 S5PC1XX_SPDIF_SPDBSTAS_COMPRESSED_DATA_TYPE_AC3,		// As per firmware code 
		 s5p_spdif.regs + S5PC1XX_SPDIF_SPDBSTAS);
	 
	 // Set SPDIFOUT Channel Status Register
	 writel(//S5PC1XX_SPDIF_SPDCSTAS_CLOCK_ACCURACY_50 |
		S5PC1XX_SPDIF_SPDCSTAS_CLOCK_ACCURACY_VARIABLE | 		// As per firmware code
	 	sampling_freq |
	 	S5PC1XX_SPDIF_SPDCSTAS_CHANNEL_NUMBER(0) |
	 	S5PC1XX_SPDIF_SPDCSTAS_SOURCE_NUMBER(0) |
	 	S5PC1XX_SPDIF_SPDCSTAS_CATEGORY_CODE_CD |
	 	S5PC1XX_SPDIF_SPDCSTAS_CHANNEL_SATAUS_MODE |
	 	S5PC1XX_SPDIF_SPDCSTAS_EMPHASIS_WITHOUT_PRE_EMP |
	 	S5PC1XX_SPDIF_SPDCSTAS_COPYRIGHT_ASSERTION_NO |
	 	S5PC1XX_SPDIF_SPDCSTAS_AUDIO_SAMPLE_WORD_LINEAR_PCM |
	 	S5PC1XX_SPDIF_SPDCSTAS_CHANNEL_STATUS_BLOCK_CON ,s5p_spdif.regs + S5PC1XX_SPDIF_SPDCSTAS);

	 // SPDIFOUT Repitition Count register
	 writel(S5PC1XX_SPDIF_SPDCNT_STREAM_REPETITION_COUNT(0), s5p_spdif.regs + S5PC1XX_SPDIF_SPDCNT);
	 return 0;

 }
 //Modified as per 2.6.29
 static int s5p_spdif_trigger(struct snd_pcm_substream *substream, int cmd,struct snd_soc_dai *dai)
 {
	 int ret = 0;
 
	 s3cdbg("Entered %s: cmd = %d\n", __FUNCTION__, cmd);
 
	 switch (cmd) {
	 case SNDRV_PCM_TRIGGER_START:
	 case SNDRV_PCM_TRIGGER_RESUME:
	 case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* if (!s3c24xx_snd_is_clkmaster()) {
			 ret = s3c24xx_snd_lrsync();
			 if (ret)
				 goto exit_err;
		 }*/
 
			 s5p_spdif_snd_txctrl(1);
		 break;
	 case SNDRV_PCM_TRIGGER_STOP:
	 case SNDRV_PCM_TRIGGER_SUSPEND:
	 case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			 s5p_spdif_snd_txctrl(0);
		 break;
	 default:
		 ret = -EINVAL;
		 break;
	 }
 
	 return ret;
 }
//Modified as per 2.6.29
 static void s5p_spdif_shutdown(struct snd_pcm_substream *substream,struct snd_soc_dai *dai)
 {
	 s3cdbg("Entered %s\n", __FUNCTION__);
 
	 writel((S5PC1XX_SPDIF_SPDCON_SOFTWARE_RESET_EN), s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);	 // Sw Reset
	 mdelay(100);
	 writel(S5PC1XX_SPDIF_SPDCLKCON_SPDIFOUT_POWER_OFF, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCLKCON);	 // SPDIFOUT clock power off
 }
 
 /*
  * Set S3C24xx Clock source
  */
 static int s5p_spdif_set_sysclk(struct snd_soc_dai *cpu_dai,
	 int clk_id, unsigned int freq, int dir)
 {
	 return 0;
 }
 
 /*
  * Set S3C24xx Clock dividers
  */
 static int s5p_spdif_set_clkdiv(struct snd_soc_dai *cpu_dai,
	 int div_id, int div)
 {
	 s3cdbg("Entered %s : div_id = %d, div = %x\n", __FUNCTION__, div_id, div);

	 return 0;
 }

 /*
  * To avoid duplicating clock code, allow machine driver to
  * get the clockrate from here.
  */
 u32 s5p_spdif_get_clockrate(void)
 {
	 return clk_get_rate(s5p_spdif.spdif_clk);
 }
 EXPORT_SYMBOL_GPL(s5p_spdif_get_clockrate);

 #define SPDIF_SPDCON_INT_MASK	(S5PC1XX_SPDIF_SPDCON_FIFO_LEVEL_INT_STATUS_PENDING | \
 				 S5PC1XX_SPDIF_SPDCON_BUFFER_EMPTY_INT_STATUS_PENDING | \
 				 S5PC1XX_SPDIF_SPDCON_STREAM_END_INT_STATUS_PENDING | \
 				 S5PC1XX_SPDIF_SPDCON_USER_DATA_INT_STATUS_PENDING )
 static irqreturn_t s5p_spdif_irq(int irqno, void *dev_id)
 {

	u32 spdifcon;

	spdifcon = readl(s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);

	if (spdifcon&(S5PC1XX_SPDIF_SPDCON_FIFO_LEVEL_INT_ENABLE | S5PC1XX_SPDIF_SPDCON_FIFO_LEVEL_INT_STATUS_PENDING)){
		s3cdbg("Entered %s interrupt src : SPDIF_INT_FIFOLVL \n", __FUNCTION__);
	}
	if (spdifcon&(S5PC1XX_SPDIF_SPDCON_BUFFER_EMPTY_INT_ENABLE | S5PC1XX_SPDIF_SPDCON_BUFFER_EMPTY_INT_STATUS_PENDING)){
		s3cdbg("Entered %s interrupt src : SPDIF_INT_BUFEMPTY \n", __FUNCTION__);
	}
	if (spdifcon&(S5PC1XX_SPDIF_SPDCON_STREAM_END_INT_ENABLE | S5PC1XX_SPDIF_SPDCON_STREAM_END_INT_STATUS_PENDING)){
		s3cdbg("Entered %s interrupt src : SPDIF_INT_STREAMEND \n", __FUNCTION__);
	}
	if (spdifcon&(S5PC1XX_SPDIF_SPDCON_USER_DATA_INT_ENABLE | S5PC1XX_SPDIF_SPDCON_USER_DATA_INT_STATUS_PENDING)){
		s3cdbg("Entered %s interrupt src : SPDIF_INT_USERDATA \n", __FUNCTION__);
	}
	
	writel(spdifcon, s5p_spdif.regs + S5PC1XX_SPDIF_SPDCON);	 // Sw Reset
	
	return IRQ_HANDLED;
 }
 
 static int s5p_spdif_probe(struct platform_device *pdev,
	 struct snd_soc_dai *dai)
 {
	 int ret;
	//u32 value;
	//u32 uMdiv, uPdiv, uSdiv;
 
	 s3cdbg("Entered %s\n", __FUNCTION__);
 
	 s5p_spdif.regs = ioremap(S5P_PA_SPDIF, 0x40);
	 if (s5p_spdif.regs == NULL)
		 return -ENXIO;

	s5p_spdif.spdif_clk=clk_get(&pdev->dev, "spdif");
	if (s5p_spdif.spdif_clk == NULL) {
		s3cdbg("failed to get spdif_clock\n");
		iounmap(s5p_spdif.regs);
		return -ENODEV;
	}
	clk_enable(s5p_spdif.spdif_clk);

	// set spcial clock for SPDIF
	s5p_spdif.spdif_sclk=clk_get(&pdev->dev, "sclk_spdif");
	if (s5p_spdif.spdif_clk == NULL) {
		s3cdbg("failed to get spdif_clock\n");
		iounmap(s5p_spdif.regs);
		return -ENODEV;
	}
	clk_enable(s5p_spdif.spdif_sclk);

	ret = request_irq(IRQ_SPDIF, s5p_spdif_irq, 0,
		  "s5p_spdif", pdev);
	if (ret < 0) {
		s3cdbg("fail to claim i2s irq , ret = %d\n", ret);
		return -ENODEV;
	}

	 return 0;
 }
 
#ifdef CONFIG_PM
//Modified as per 2.6.29
 static int s5p_spdif_suspend(struct snd_soc_dai *dai)
 {
	 s3cdbg("Entered %s\n", __FUNCTION__);
	 return 0;
 }
 //Modified as per 2.6.29
 static int s5p_spdif_resume(struct snd_soc_dai *dai)
 {
	 s3cdbg("Entered %s\n", __FUNCTION__);
	 return 0;
 }
 
#else
#define s5p_spdif_suspend	NULL
#define s5p_spdif_resume	NULL
#endif
 
 // to do: need to modify for spdif
#define S5PC1XX_SPDIF_RATES \
	 (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	 SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)
 
 //Modified as per 2.6.29
 struct snd_soc_dai s5p_spdif_dai = {
	 .name = "s5p-spdif",
	 .id = 0,
	 .probe = s5p_spdif_probe,					    
	 .suspend = s5p_spdif_suspend,
	 .resume = s5p_spdif_resume,
	 .playback = {
		 .channels_min = 1,
		 .channels_max = 2,
		 .rates = S5PC1XX_SPDIF_RATES, 
		 .formats = SNDRV_PCM_FMTBIT_S16_LE}, 
	 .ops = {
		 .shutdown = s5p_spdif_shutdown,	 
		 .trigger = s5p_spdif_trigger,	
		 .hw_params = s5p_spdif_hw_params,
                 .set_clkdiv = s5p_spdif_set_clkdiv,
                 .set_sysclk = s5p_spdif_set_sysclk,
		},	
 };

 EXPORT_SYMBOL_GPL(s5p_spdif_dai);

/*dai registration - Modified as per 2.6.29*/
//[
static int __init s5p_spdif_init(void)
{
	return snd_soc_register_dai(&s5p_spdif_dai);
}
module_init(s5p_spdif_init);

static void __exit s5pcxx_spdif_exit(void)
{
	 snd_soc_unregister_dai(&s5p_spdif_dai);
}
module_exit(s5pcxx_spdif_exit);
//]
 /* Module information */
 MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
 MODULE_DESCRIPTION("S5P SPDIF SoC Interface");
 MODULE_LICENSE("GPL");
 
