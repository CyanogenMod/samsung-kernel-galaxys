/*
 * s3c-i2s.c  --  ALSA Soc Audio Layer
 *
 * (c) 2009 Samsung Electronics   - Jaswinder Singh Brar <jassi.brar@samsung.com>
 *  Derived from Ben Dooks' driver for s3c24xx
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/map.h>
#include <mach/s3c-dma.h>
#include <plat/regs-clock.h>
#include <plat/regs-audss.h>
#include "s3c-pcm-lp.h"
#include "s3c-i2s.h"
#ifdef CONFIG_CPU_FREQ
#include <plat/s5pc11x-dvfs.h>
#endif
static struct s3c2410_dma_client s3c_dma_client_out = {
	.name = "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s3c_dma_client_in = {
	.name = "I2S PCM Stereo in"
};

static struct s5p_pcm_dma_params s3c_i2s_pcm_stereo_out = {
	.client		= &s3c_dma_client_out,
	.channel	= S3C_DMACH_I2S_OUT,
	.dma_addr	= S3C_IIS_PABASE + S3C_IISTXD,
	.dma_size	= 4,
};

static struct s5p_pcm_dma_params s3c_i2s_pcm_stereo_in = {
	.client		= &s3c_dma_client_in,
	.channel	= S3C_DMACH_I2S_IN,
	.dma_addr	= S3C_IIS_PABASE + S3C_IISRXD,
	.dma_size	= 4,
};

struct s3c_i2s_info {
	void __iomem  *regs;
	struct clk    *iis0_ip_clk;
	struct clk    *i2sclk;
	struct clk    *i2sbusclk;
	u32           iiscon;
	u32           iismod;
	u32           iisfic;
	u32           iispsr;
	u32	      audss_clksrc;
	u32	      audss_clkdiv;
	u32	      audss_clkgate;
	u32           slave;
	u32           clk_rate;
};

static struct s3c_i2s_info s3c_i2s;
struct s5p_i2s_pdata s3c_i2s_pdat;
static u32 clk_en_dis_play  = 0;
static u32 clk_en_dis_rec  = 0;

#define S3C_IISFIC_LP 			(s3c_i2s_pdat.lp_mode ? S3C_IISFICS : S3C_IISFIC)
#define S3C_IISCON_TXDMACTIVE_LP 	(s3c_i2s_pdat.lp_mode ? S3C_IISCON_TXSDMACTIVE : S3C_IISCON_TXDMACTIVE)
#define S3C_IISCON_TXDMAPAUSE_LP 	(s3c_i2s_pdat.lp_mode ? S3C_IISCON_TXSDMAPAUSE : S3C_IISCON_TXDMAPAUSE)
#define S3C_IISMOD_BLCMASK_LP	(s3c_i2s_pdat.lp_mode ? S3C_IISMOD_BLCSMASK : S3C_IISMOD_BLCPMASK)
#define S3C_IISMOD_8BIT_LP	(s3c_i2s_pdat.lp_mode ? S3C_IISMOD_S8BIT : S3C_IISMOD_P8BIT)
#define S3C_IISMOD_16BIT_LP	(s3c_i2s_pdat.lp_mode ? S3C_IISMOD_S16BIT : S3C_IISMOD_P16BIT)
#define S3C_IISMOD_24BIT_LP	(s3c_i2s_pdat.lp_mode ? S3C_IISMOD_S24BIT : S3C_IISMOD_P24BIT)

#define dump_i2s()	do{	\
				printk("%s:%s:%d\t", __FILE__, __func__, __LINE__);	\
				printk("\tS3C_IISCON : %x", readl(s3c_i2s.regs + S3C_IISCON));		\
				printk("\tS3C_IISMOD : %x\n", readl(s3c_i2s.regs + S3C_IISMOD));	\
				printk("\tS3C_IISFIC : %x", readl(s3c_i2s.regs + S3C_IISFIC_LP));	\
				printk("\tS3C_IISPSR : %x\n", readl(s3c_i2s.regs + S3C_IISPSR));	\
				printk("\tS3C_IISAHB : %x\n", readl(s3c_i2s.regs + S3C_IISAHB));	\
				printk("\tS3C_IISSTR : %x\n", readl(s3c_i2s.regs + S3C_IISSTR));	\
				printk("\tS3C_IISSIZE : %x\n", readl(s3c_i2s.regs + S3C_IISSIZE));	\
				printk("\tS3C_IISADDR0 : %x\n", readl(s3c_i2s.regs + S3C_IISADDR0));	\
			}while(0)

#define dump_i2s_sub() do{      \
				printk("%s:%s:%d\t",__FILE__,__func__,__LINE__);	\
				printk("\tAudio_sub_clk_Src: %x",__raw_readl(S5P_CLKSRC_AUDSS));	\
				printk("\tAudio_sub_clk_divider: %x\n",__raw_readl(S5P_CLKDIV_AUDSS));	\
				printk("\tAudio_sub_clk_gate: %x\n",__raw_readl(S5P_CLKGATE_AUDSS));	\
				}while(0)

static void s3c_snd_txctrl(int on)
{
	u32 iiscon;
	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	s3cdbg("\n Txcontrol on:%d \n",on);

	if(on){
		iiscon |= S3C_IISCON_I2SACTIVE;
		iiscon  &= ~S3C_IISCON_TXCHPAUSE;
		iiscon  &= ~S3C_IISCON_TXDMAPAUSE_LP;
		iiscon  |= S3C_IISCON_TXDMACTIVE_LP;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
#ifdef CONFIG_CPU_FREQ
        	// if conservative governor
        	// printk("s5pc110_lock_dvfs_high_level\n");
        	//s5pc110_lock_dvfs_high_level(DVFS_LOCK_TOKEN_3, 1);
#endif

		//dump_i2s();
	}else{
		if(!(iiscon & S3C_IISCON_RXDMACTIVE)) /* Stop only if RX not active */
			iiscon &= ~S3C_IISCON_I2SACTIVE;
		iiscon  |= S3C_IISCON_TXCHPAUSE;
		iiscon  |= S3C_IISCON_TXDMAPAUSE_LP;
		iiscon  &= ~S3C_IISCON_TXDMACTIVE_LP;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
#ifdef CONFIG_CPU_FREQ
        	// change cpufreq to original one
		//printk("s5pc110_unlock_dvfs_high_level\n");
    		//s5pc110_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_3);

#endif
		//dump_i2s();
	}
}

static void s3c_snd_rxctrl(int on)
{
	u32 iiscon;
	s3cdbg("\n Rxcontrol on:%d \n",on);

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	
	if(on){
		iiscon |= S3C_IISCON_I2SACTIVE;
		iiscon  &= ~S3C_IISCON_RXCHPAUSE;
		iiscon  &= ~S3C_IISCON_RXDMAPAUSE;
		iiscon  |= S3C_IISCON_RXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
#ifdef CONFIG_CPU_FREQ
                // if conservative governor
                //s5pc110_lock_dvfs_high_level(DVFS_LOCK_TOKEN_3, 1);
#endif

		//dump_i2s();
	}else{
		if(!(iiscon & S3C_IISCON_TXDMACTIVE_LP)) /* Stop only if TX not active */
			iiscon &= ~S3C_IISCON_I2SACTIVE;
		//iiscon	&= ~
		iiscon  |= S3C_IISCON_RXCHPAUSE;
		iiscon  |= S3C_IISCON_RXDMAPAUSE;
		iiscon  &= ~S3C_IISCON_RXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
#ifdef CONFIG_CPU_FREQ
                // change cpufreq to original one
		//printk("s5pc110_unlock_dvfs_high_level\n");
                //s5pc110_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_3);

#endif
		//dump_i2s();
	
		#if 0	
		if(readl(s3c_i2s.regs + S3C_IISCON) & (1<<26))
			printk("\noverflow bit already set!!!\n");	
		
		writel((1<<7), s3c_i2s.regs + S3C_IISFIC); //mkh flush rx fifo
		//while(readl(s3c_i2s.regs + S3C_IISFIC) & 0x3f)
		//{}
		#endif
	}
}

/*
 * Wait for the LR signal to allow synchronisation to the L/R clock
 * from the codec. May only be needed for slave mode.
 */
static int s3c_snd_lrsync(void)
{
	u32 iiscon;
	int timeout = 50; /* 5ms */

	while (1) {
		iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		if (iiscon & S3C_IISCON_LRI)
			break;

		if (!timeout--)
			return -ETIMEDOUT;
		udelay(100);
	}

	return 0;
}

/*
 * Set s3c_ I2S DAI format
 */
static int s3c_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	u32 iismod;

	iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	iismod &= ~S3C_IISMOD_SDFMASK;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		s3c_i2s.slave = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		s3c_i2s.slave = 0;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iismod &= ~S3C_IISMOD_MSB;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iismod |= S3C_IISMOD_MSB;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iismod |= S3C_IISMOD_LSB;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iismod &= ~S3C_IISMOD_LRP;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iismod |= S3C_IISMOD_LRP;
		break;
	case SND_SOC_DAIFMT_IB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	default:
		printk("Inv-combo(%d) not supported!\n", fmt & SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	return 0;
}

static int s3c_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	u32 iismod;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		rtd->dai->cpu_dai->dma_data = &s3c_i2s_pcm_stereo_out;
	else
		rtd->dai->cpu_dai->dma_data = &s3c_i2s_pcm_stereo_in;

	/* Working copies of register */
	iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	iismod &= ~(S3C_IISMOD_BLCMASK | S3C_IISMOD_BLCMASK_LP);

	/* TODO */
	switch(params_channels(params)) {
	case 1:
		s3c_i2s_pcm_stereo_in.dma_size = 2;
		break;
	case 2:
		s3c_i2s_pcm_stereo_in.dma_size =4;
		break;
	case 4:
		break;
	case 6:
		break;
	default:
		break;
	}

	/* RFS & BFS are set by dai_link(machine specific) code via set_clkdiv */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		iismod |= S3C_IISMOD_8BIT | S3C_IISMOD_8BIT_LP;
 		break;
 	case SNDRV_PCM_FORMAT_S16_LE:
 		iismod |= S3C_IISMOD_16BIT | S3C_IISMOD_16BIT_LP;
 		break;
 	case SNDRV_PCM_FORMAT_S24_LE:
 		iismod |= S3C_IISMOD_24BIT | S3C_IISMOD_24BIT_LP;
 		break;
	default:
		return -EINVAL;
 	}
 
	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	return 0;
}
static bool value_saved = false;
//static u32 clk_en_dis  = 0;
static int s3c_i2s_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	u32 iiscon, iisfic;
	u32 retval;

//	iiscon = readl(s3c_i2s.regs + S3C_IISCON);

	/* FIFOs must be flushed before enabling PSR and other MOD bits, so we do it here. */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		s3cdbg("Entering..%s..for PLAYBACK\n",__func__);	
		if(!clk_en_dis_play && !clk_en_dis_rec)
	        {
//Clk enabling should be in following sequence..with iis0_ip clk enabled IIS power will be ON
        	retval = clk_enable(s3c_i2s.iis0_ip_clk);
        	clk_enable(s3c_i2s.i2sbusclk);
        	clk_enable(s3c_i2s.i2sclk);
		if(value_saved == true)
		{
		__raw_writel(s3c_i2s.audss_clksrc,S5P_CLKSRC_AUDSS);
		__raw_writel(s3c_i2s.audss_clkdiv,S5P_CLKDIV_AUDSS);
		__raw_writel(s3c_i2s.audss_clkgate,S5P_CLKGATE_AUDSS);
		writel(s3c_i2s.iiscon, s3c_i2s.regs + S3C_IISCON);
	        writel(s3c_i2s.iismod, s3c_i2s.regs + S3C_IISMOD);
       		writel(s3c_i2s.iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
        	writel(s3c_i2s.iispsr, s3c_i2s.regs + S3C_IISPSR);
		value_saved = false;
		}
		s3cdbg("..Enabled all clocks finally..\n");
       		//clk_en_dis_play ++ ;
        	}
       		clk_en_dis_play =1 ;

		iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		if(iiscon & S3C_IISCON_TXDMACTIVE_LP)
			return 0;

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
		iisfic |= S3C_IISFIC_TFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);

		do{
	   	   cpu_relax();
		   //iiscon = __raw_readl(s3c_i2s.regs + S3C_IISCON);
		}while((__raw_readl(s3c_i2s.regs + S3C_IISFIC) >> 8) & 0x7f);

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
		iisfic &= ~S3C_IISFIC_TFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
	
		//dump_i2s_sub();
		//dump_i2s();
	}else{
		s3cdbg("Entering..%s..for RECORD\n",__func__);	
		if(!clk_en_dis_rec && !clk_en_dis_play)
	        {
		s3cdbg("..Enabling clock finally..\n");
        	retval = clk_enable(s3c_i2s.iis0_ip_clk);
        	clk_enable(s3c_i2s.i2sbusclk);
        	clk_enable(s3c_i2s.i2sclk);
		if(value_saved == true)
		{
		__raw_writel(s3c_i2s.audss_clksrc,S5P_CLKSRC_AUDSS);
                __raw_writel(s3c_i2s.audss_clkdiv,S5P_CLKDIV_AUDSS);
                __raw_writel(s3c_i2s.audss_clkgate,S5P_CLKGATE_AUDSS);
		writel(s3c_i2s.iiscon, s3c_i2s.regs + S3C_IISCON);
        	writel(s3c_i2s.iismod, s3c_i2s.regs + S3C_IISMOD);
        	writel(s3c_i2s.iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
        	writel(s3c_i2s.iispsr, s3c_i2s.regs + S3C_IISPSR);
		value_saved = false;
		}
       		//clk_en_dis_rec ++ ;
        	}
		
        	clk_en_dis_rec = 1 ;
		iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		if(iiscon & S3C_IISCON_RXDMACTIVE)
			return 0;

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
		iisfic |= S3C_IISFIC_RFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);

		do{
	   	   cpu_relax();
		   //iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		}while((__raw_readl(s3c_i2s.regs + S3C_IISFIC) >> 0) & 0x7f);

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
		iisfic &= ~S3C_IISFIC_RFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
	}

	return 0;
}

static void s3c_i2s_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	s3cdbg("Entering..%s..\n",__func__);	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
                s3cdbg("Entering..%s..for PLAYBACK\n",__func__);
		clk_en_dis_play = 0 ;
	}
	else{
		s3cdbg("Entering..%s..for RECORD\n",__func__);
#if 1
                if(readl(s3c_i2s.regs + S3C_IISCON) & (1<<26))
                {
                s3cdbg("\n rx overflow int in %s.. \n",__func__);
                writel(readl(s3c_i2s.regs + S3C_IISCON) | (1<<26),s3c_i2s.regs + S3C_IISCON); //clear rxfifo overflow interrupt
                writel(readl(s3c_i2s.regs + S3C_IISFIC) | (1<<7) , s3c_i2s.regs + S3C_IISFIC); //flush rx
                }
#endif

		//clk_en_dis_rec --;
		clk_en_dis_rec = 0;
	}
	
       if((!clk_en_dis_play)&&(!clk_en_dis_rec))
	{
	//dump_i2s_sub();
	//dump_i2s();
	s3c_i2s.iiscon = readl(s3c_i2s.regs + S3C_IISCON);
        s3c_i2s.iismod = readl(s3c_i2s.regs + S3C_IISMOD);
        s3c_i2s.iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
        s3c_i2s.iispsr = readl(s3c_i2s.regs + S3C_IISPSR);
	s3c_i2s.audss_clksrc = __raw_readl(S5P_CLKSRC_AUDSS);
	s3c_i2s.audss_clkdiv = __raw_readl(S5P_CLKDIV_AUDSS);
	s3c_i2s.audss_clkgate = __raw_readl(S5P_CLKGATE_AUDSS);
	value_saved = true;
//Don't change the clock disabling sequence..With iis0_ip clk disabled power will be gated for IIS
        clk_disable(s3c_i2s.i2sbusclk);
        clk_disable(s3c_i2s.i2sclk);    
	clk_disable(s3c_i2s.iis0_ip_clk);
	s3cdbg("..Disbaled all clock finally..\n");
	}
}
static int s3c_i2s_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	u32 iismod;

	iismod = readl(s3c_i2s.regs + S3C_IISMOD);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_RX){
			iismod &= ~S3C_IISMOD_TXRMASK;
			iismod |= S3C_IISMOD_TXRX;
		}
	}else{
		if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_TX){
			iismod &= ~S3C_IISMOD_TXRMASK;
			iismod |= S3C_IISMOD_TXRX;
		}
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);

	return 0;
}

static int s3c_i2s_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
#if 0
		if(s3c_i2s_pdat.lp_mode)
			break;
#endif
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (s3c_i2s.slave) {
			ret = s3c_snd_lrsync();
			if (ret)
				goto exit_err;
		}

		 
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s3c_snd_rxctrl(1);
		else
			s3c_snd_txctrl(1);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
#if 0
		if(s3c_i2s_pdat.lp_mode)
			break;
#endif
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s3c_snd_rxctrl(0);
		else
			s3c_snd_txctrl(0);
		break;
	default:
		ret = -EINVAL;
		break;
	}
exit_err:
	return ret;
}

/*
 * Set s3c_ Clock source
 * Since, we set frequencies using PreScaler and BFS, RFS, we select input clock source to the IIS here.
 */
static int s3c_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{
	struct clk *clk;
	//u32 clksrc;
	u32 iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	switch (clk_id) {
	case S3C_CLKSRC_PCLK: /* IIS-IP is Master and derives its clocks from PCLK */
		if(s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.iis0_ip_clk);
		break;

#ifdef USE_CLKAUDIO
	case S3C_CLKSRC_CLKAUDIO: /* IIS-IP is Master and derives its clocks from I2SCLKD2 */
		if(s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
/*
8000 x 256 = 2048000
         49152000 mod 2048000 = 0
         32768000 mod 2048000 = 0 
         73728000 mod 2048000 = 0

11025 x 256 = 2822400
         67738000 mod 2822400 = 400

16000 x 256 = 4096000
         49152000 mod 4096000 = 0
         32768000 mod 4096000 = 0 
         73728000 mod 4096000 = 0

22050 x 256 = 5644800
         67738000 mod 5644800 = 400

32000 x 256 = 8192000
         49152000 mod 8192000 = 0
         32768000 mod 8192000 = 0
         73728000 mod 8192000 = 0

44100 x 256 = 11289600
         67738000 mod 11289600 = 400

48000 x 256 = 12288000
         49152000 mod 12288000 = 0
         73728000 mod 12288000 = 0

64000 x 256 = 16384000
         49152000 mod 16384000 = 0
         32768000 mod 16384000 = 0

88200 x 256 = 22579200
         67738000 mod 22579200 = 400

96000 x 256 = 24576000
         49152000 mod 24576000 = 0
         73728000 mod 24576000 = 0

	From the table above, we find that 49152000 gives least(0) residue 
	for most sample rates, followed by 67738000.
*/
		clk = clk_get(NULL, EPLLCLK);
		if (IS_ERR(clk)) {
			printk("failed to get %s\n", EPLLCLK);
			return -EBUSY;
		}
		clk_disable(clk);
		switch (freq) {
		case 8000:
		case 16000:
		case 32000:
		case 48000:
		case 64000:
		case 96000:
			clk_set_rate(clk, 49152000);
			break;
		case 11025:
		case 22050:
		case 44100:
		case 88200:
		default:
			clk_set_rate(clk, 67738000);
			break;
		}
		clk_enable(clk);
		s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.i2sclk);
		s3cdbg("Setting FOUTepll to %dHz\n", s3c_i2s.clk_rate);
		clk_put(clk);
		break;
#endif

	case S3C_CLKSRC_SLVPCLK: /* IIS-IP is Slave, but derives its clocks from PCLK */
	case S3C_CLKSRC_I2SEXT:  /* IIS-IP is Slave and derives its clocks from the Codec Chip */
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		//Operation clock for I2S logic selected as Audio Bus Clock
		iismod |= S3C_IISMOD_OPPCLK;

		clk = clk_get(NULL, EPLLCLK);
                if (IS_ERR(clk)) {
                        printk("failed to get %s\n", EPLLCLK);
                        return -EBUSY;
                }
                clk_disable(clk);
		clk_set_rate(clk, 67738000);
		clk_enable(clk);
		clk_put(clk);

		break;

	case S3C_CDCLKSRC_INT:
		iismod &= ~S3C_IISMOD_CDCLKCON;
		break;

	case S3C_CDCLKSRC_EXT:
		iismod |= S3C_IISMOD_CDCLKCON;
		break;

	default:
		return -EINVAL;
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	s3cdbg("...IISMOD=0x%x..\n",readl(s3c_i2s.regs + S3C_IISMOD));
	return 0;
}

/*
 * Set s3c_ Clock dividers
 * NOTE: NOT all combinations of RFS, BFS and BCL are supported! XXX
 * Machine specific(dai-link) code must consider that while setting MCLK and BCLK in this function. XXX
 */
/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS_VAL(must be a multiple of BFS)                                 XXX */
/* XXX RFS_VAL & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
static int s3c_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	u32 reg;

	switch (div_id) {
	case S3C_DIV_MCLK:
		reg = readl(s3c_i2s.regs + S3C_IISMOD) & ~S3C_IISMOD_RFSMASK;
		switch(div) {
		case 256: div = S3C_IISMOD_256FS; break;
		case 512: div = S3C_IISMOD_512FS; break;
		case 384: div = S3C_IISMOD_384FS; break;
		case 768: div = S3C_IISMOD_768FS; break;
		default: return -EINVAL;
		}
		writel(reg | div, s3c_i2s.regs + S3C_IISMOD);
		break;
	case S3C_DIV_BCLK:
		reg = readl(s3c_i2s.regs + S3C_IISMOD) & ~S3C_IISMOD_BFSMASK;
		switch(div) {
		case 16: div = S3C_IISMOD_16FS; break;
		case 24: div = S3C_IISMOD_24FS; break;
		case 32: div = S3C_IISMOD_32FS; break;
		case 48: div = S3C_IISMOD_48FS; break;
		default: return -EINVAL;
		}
		writel(reg | div, s3c_i2s.regs + S3C_IISMOD);
		break;
	case S3C_DIV_PRESCALER:
		reg = readl(s3c_i2s.regs + S3C_IISPSR) & ~S3C_IISPSR_PSRAEN;
		writel(reg, s3c_i2s.regs + S3C_IISPSR);
		reg = readl(s3c_i2s.regs + S3C_IISPSR) & ~S3C_IISPSR_PSVALA;
		div &= 0x3f;
		writel(reg | (div<<8) | S3C_IISPSR_PSRAEN, s3c_i2s.regs + S3C_IISPSR);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static irqreturn_t s3c_iis_irq(int irqno, void *dev_id)
{
	u32 iiscon, iisahb, val;

	//dump_i2s();

#if 1	
	if(readl(s3c_i2s.regs + S3C_IISCON) & (1<<26))
	{
	printk("\n rx overflow int ..@=%d\n",__LINE__);
	writel(readl(s3c_i2s.regs + S3C_IISCON) | (1<<26),s3c_i2s.regs + S3C_IISCON); //clear rxfifo overflow interrupt
//	writel(readl(s3c_i2s.regs + S3C_IISFIC) | (1<<7) , s3c_i2s.regs + S3C_IISFIC); //flush rx
	}
	//printk("++++IISCON = 0x%08x\n, IISFIC 0x%x", readl(s3c_i2s.regs + S3C_IISCON),readl(s3c_i2s.regs + S3C_IISFIC));
#endif
	iisahb  = readl(s3c_i2s.regs + S3C_IISAHB);
	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	if(iiscon & S3C_IISCON_FTXSURSTAT) {
		iiscon |= S3C_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_i2s.regs + S3C_IISCON);
		s3cdbg("TX_S underrun interrupt IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	}
	if(iiscon & S3C_IISCON_FTXURSTATUS) {
		iiscon &= ~S3C_IISCON_FTXURINTEN;
		iiscon |= S3C_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_i2s.regs + S3C_IISCON);
		s3cdbg("TX_P underrun interrupt IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	}

	val = 0;
	val |= ((iisahb & S3C_IISAHB_LVL0INT) ? S3C_IISAHB_CLRLVL0 : 0);
	//val |= ((iisahb & S3C_IISAHB_DMAEND) ? S3C_IISAHB_DMACLR : 0);
	if(val){
		iisahb |= val;
		iisahb |= S3C_IISAHB_INTENLVL0;
		writel(iisahb, s3c_i2s.regs + S3C_IISAHB);

		if(iisahb & S3C_IISAHB_LVL0INT){
			val = readl(s3c_i2s.regs + S3C_IISADDR0) - LP_TXBUFF_ADDR; /* current offset */
			val += s3c_i2s_pdat.dma_prd; /* Length before next Lvl0 Intr */
			val %= MAX_LP_BUFF; /* Round off at boundary */
			writel(LP_TXBUFF_ADDR + val, s3c_i2s.regs + S3C_IISADDR0); /* Update start address */
		}

		if(iisahb & S3C_IISAHB_DMAEND)
			printk("Didnt expect S3C_IISAHB_DMAEND\n");

		iisahb  = readl(s3c_i2s.regs + S3C_IISAHB);
		iisahb |= S3C_IISAHB_DMAEN;
		writel(iisahb, s3c_i2s.regs + S3C_IISAHB);

		/* Keep callback in the end */
		if(s3c_i2s_pdat.dma_cb){
		   val = (iisahb & S3C_IISAHB_LVL0INT) ? 
				s3c_i2s_pdat.dma_prd:
				MAX_LP_BUFF;
		   s3c_i2s_pdat.dma_cb(s3c_i2s_pdat.dma_token, val);
		}
	}

//	if(readl(s3c_i2s.regs + S3C_IISFIC) & (1<<7))
//	writel(readl(s3c_i2s.regs + S3C_IISFIC) & (~(1<<7)), s3c_i2s.regs + S3C_IISFIC);

	//printk("----IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	return IRQ_HANDLED;
}

static void init_i2s(void)
{
	u32 iiscon, iismod, iisahb;

	writel(S3C_IISCON_I2SACTIVE | S3C_IISCON_SWRESET, s3c_i2s.regs + S3C_IISCON);

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	iismod  = readl(s3c_i2s.regs + S3C_IISMOD);
	iisahb  = S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT;

	/* Enable all interrupts to find bugs */
	iiscon |= S3C_IISCON_FRXOFINTEN;
	iismod &= ~S3C_IISMOD_OPMSK;
	iismod |= S3C_IISMOD_OPCCO;

	if(s3c_i2s_pdat.lp_mode){
		iiscon &= ~S3C_IISCON_FTXURINTEN;
		iiscon |= S3C_IISCON_FTXSURINTEN;
		iismod |= S3C_IISMOD_TXSLP;
		//iisahb |= S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT;
	}else{
		iiscon &= ~S3C_IISCON_FTXSURINTEN;
		iiscon |= S3C_IISCON_FTXURINTEN;
		iismod &= ~S3C_IISMOD_TXSLP;
		iisahb = 0;
	}

	writel(iisahb, s3c_i2s.regs + S3C_IISAHB);
	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	writel(iiscon, s3c_i2s.regs + S3C_IISCON);
}

static int s3c_i2s_probe(struct platform_device *pdev,
			     struct snd_soc_dai *dai)
{
	int ret = 0;
	struct clk *cm , *cn;

	s3c_i2s.regs = ioremap(S3C_IIS_PABASE, 0x100);
	if (s3c_i2s.regs == NULL)
		return -ENXIO;

	ret = request_irq(S3C_IISIRQ, s3c_iis_irq, 0, "s3c-i2s", pdev);
	if (ret < 0) {
		printk("fail to claim i2s irq , ret = %d\n", ret);
		iounmap(s3c_i2s.regs);
		return -ENODEV;
	}

	s3c_i2s.iis0_ip_clk = clk_get(&pdev->dev, CLK_I2S0);
	if (IS_ERR(s3c_i2s.iis0_ip_clk)) {
		printk("failed to get clk(%s)\n", CLK_I2S0);
		goto lb4;
	}
	s3cdbg("Got Clock -> %s\n", CLK_I2S0);

#ifdef USE_CLKAUDIO
	/* To avoid switching between sources(LP vs NM mode),
	 * we use EXTPRNT as parent clock of i2sclkd2.
	 */
	s3c_i2s.i2sclk = clk_get(NULL, EXTCLK);
	if (IS_ERR(s3c_i2s.i2sclk)) {
		printk("failed to get clk(%s)\n", EXTCLK);
		goto lb3;
	}
	s3cdbg("Got Audio I2s-Clock -> %s\n", EXTCLK);

	s3c_i2s.i2sbusclk = clk_get(NULL, "audss_hclk");//getting AUDIO BUS CLK
         if (IS_ERR(s3c_i2s.i2sbusclk)) {
                        printk("failed to get audss_hclk\n");
                        goto lb2;
         }
         s3cdbg("Got audss_hclk..Audio-Bus-clk\n");
#ifndef CONFIG_SND_UNIVERSAL_WM8994_MASTER

	cm = clk_get(NULL, EXTPRNT);
	if (IS_ERR(cm)) {
		printk("failed to get %s\n", EXTPRNT);
		goto lb2_busclk;
	}
	s3cdbg("Got Audio Bus Source Clock -> %s\n", EXTPRNT);

	
	if(clk_set_parent(s3c_i2s.i2sclk, cm)){
		printk("failed to set %s as parent of %s\n", EXTPRNT, EXTCLK);
		goto lb1;
	}

	s3cdbg("Set %s as parent of %s\n", EXTPRNT, EXTCLK);

	cn = clk_get(NULL, EPLLCLK);
        if (IS_ERR(cn)) {
                printk("failed to get fout_epll\n");
                goto lb1;
        }
        s3cdbg("Got Clock -> fout_epll\n");

        if(clk_set_parent(cm, cn)){
                printk("failed to set fout_epll as parent of %s\n", EXTPRNT);
                clk_put(cn);
                goto lb1;
        }
        s3cdbg("Set fout_epll as parent of %s\n", EXTPRNT);
	

#else
	
	cm = clk_get(NULL, EPLLCLK);
        if (IS_ERR(cm)) {
                printk("failed to get fout_epll\n");
                goto lb2_busclk;
        }
	
	cn = clk_get(NULL,"i2smain_clk");
	if (IS_ERR(cn)) {
                printk("failed to get i2smain_clk\n");
                goto lb1;
        }

	 if(clk_set_parent(s3c_i2s.i2sbusclk, cn)){
                printk("failed to set i2s-main clock as parent of audss_hclk\n");
                clk_put(s3c_i2s.i2sbusclk);
		clk_put(cn);
                goto lb1;
        }

	if(clk_set_parent(cn, cm)){
                printk("failed to set fout-epll as parent of i2s-main clock \n");
            	clk_put(cm);
		clk_put(cn);
                clk_put(s3c_i2s.i2sbusclk);
		goto lb2;
	}
	
	clk_put(cn);
#endif
	clk_put(cm);

        clk_put(s3c_i2s.i2sbusclk);
#else
	if(s3c_i2s_pdat.lp_mode){
		printk("Enable USE_CLKAUDIO for LP_Audio mode!\n");
		goto lb3;
	}
#endif

	init_i2s();

	s3c_snd_txctrl(0);
	s3c_snd_rxctrl(0);

	return 0;

#ifdef USE_CLKAUDIO
lb1:
	clk_put(cm);

lb2_busclk:
	clk_put(s3c_i2s.i2sbusclk);
lb2:
	clk_put(s3c_i2s.i2sclk);
#endif
lb3:
	clk_disable(s3c_i2s.iis0_ip_clk);
	clk_put(s3c_i2s.iis0_ip_clk);
lb4:
	free_irq(S3C_IISIRQ, pdev);
	iounmap(s3c_i2s.regs);

	return -ENODEV;
}

static void s3c_i2s_remove(struct platform_device *pdev,
		       struct snd_soc_dai *dai)
{
	writel(0, s3c_i2s.regs + S3C_IISCON);

#ifdef USE_CLKAUDIO
	clk_disable(s3c_i2s.i2sclk);
	clk_put(s3c_i2s.i2sclk);
#endif
	clk_disable(s3c_i2s.iis0_ip_clk);
	clk_put(s3c_i2s.iis0_ip_clk);
	free_irq(S3C_IISIRQ, pdev);
	iounmap(s3c_i2s.regs);
}

#ifdef CONFIG_PM
static int s3c_i2s_suspend(struct snd_soc_dai *cpu_dai)
{
#if 0	
	u32 retval;
	s3cdbg("Inside..%s..routine\n",__func__);
	 if((!clk_en_dis_play) && (!clk_en_dis_rec))
        {
	s3cdbg("..Enabling clock finally..\n");
        retval = clk_enable(s3c_i2s.iis0_ip_clk);
        clk_enable(s3c_i2s.i2sbusclk);
        clk_enable(s3c_i2s.i2sclk);
        clk_en_dis_play = 1 ;
        }

	s3c_i2s.iiscon = readl(s3c_i2s.regs + S3C_IISCON);
	s3c_i2s.iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	s3c_i2s.iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
	s3c_i2s.iispsr = readl(s3c_i2s.regs + S3C_IISPSR);
#endif
	if(value_saved != true)
	{
	s3c_i2s.iiscon = readl(s3c_i2s.regs + S3C_IISCON);
        s3c_i2s.iismod = readl(s3c_i2s.regs + S3C_IISMOD);
        s3c_i2s.iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
        s3c_i2s.iispsr = readl(s3c_i2s.regs + S3C_IISPSR);
	}
	return 0;

}

static int s3c_i2s_resume(struct snd_soc_dai *cpu_dai)
{
	s3cdbg("Inside..%s..routine\n",__func__);
	if(value_saved != true)
	{
	writel(s3c_i2s.iiscon, s3c_i2s.regs + S3C_IISCON);
	writel(s3c_i2s.iismod, s3c_i2s.regs + S3C_IISMOD);
	writel(s3c_i2s.iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
	writel(s3c_i2s.iispsr, s3c_i2s.regs + S3C_IISPSR);
	}
	return 0;
}
#else
#define s3c_i2s_suspend NULL
#define s3c_i2s_resume NULL
#endif

static void s3c_i2sdma_getpos(dma_addr_t *src, dma_addr_t *dst)
{
	*dst = s3c_i2s_pcm_stereo_out.dma_addr;
	*src = LP_TXBUFF_ADDR + (readl(s3c_i2s.regs + S3C_IISTRNCNT) & 0xffffff) * 4;
}

static int s3c_i2sdma_enqueue(void *id)
{
	u32 val;

	spin_lock(&s3c_i2s_pdat.lock);
	s3c_i2s_pdat.dma_token = id;
	spin_unlock(&s3c_i2s_pdat.lock);

	s3cdbg("%s: %d@%x\n", __func__, MAX_LP_BUFF, LP_TXBUFF_ADDR);
	s3cdbg("CB @%x", LP_TXBUFF_ADDR + MAX_LP_BUFF);
	if(s3c_i2s_pdat.dma_prd != MAX_LP_BUFF)
		s3cdbg(" and @%x\n", LP_TXBUFF_ADDR + s3c_i2s_pdat.dma_prd);
	else
		s3cdbg("\n");

	val = LP_TXBUFF_ADDR + s3c_i2s_pdat.dma_prd;
	//val |= S3C_IISADDR_ENSTOP;
	writel(val, s3c_i2s.regs + S3C_IISADDR0);

	val = readl(s3c_i2s.regs + S3C_IISSTR);
	val = LP_TXBUFF_ADDR;
	writel(val, s3c_i2s.regs + S3C_IISSTR);

	val = readl(s3c_i2s.regs + S3C_IISSIZE);
	val &= ~(S3C_IISSIZE_TRNMSK << S3C_IISSIZE_SHIFT);
	val |= (((MAX_LP_BUFF >> 2) & S3C_IISSIZE_TRNMSK) << S3C_IISSIZE_SHIFT);
	writel(val, s3c_i2s.regs + S3C_IISSIZE);

	val = readl(s3c_i2s.regs + S3C_IISAHB);
	val |= S3C_IISAHB_INTENLVL0;
	writel(val, s3c_i2s.regs + S3C_IISAHB);

	return 0;
}

static void s3c_i2sdma_setcallbk(void (*cb)(void *id, int result), unsigned prd)
{
	if(!prd || prd > MAX_LP_BUFF)
	   prd = MAX_LP_BUFF;

	spin_lock(&s3c_i2s_pdat.lock);
	s3c_i2s_pdat.dma_cb = cb;
	s3c_i2s_pdat.dma_prd = prd;
	spin_unlock(&s3c_i2s_pdat.lock);

	s3cdbg("%s:%d dma_period=%d\n", __func__, __LINE__, s3c_i2s_pdat.dma_prd);
}

static inline void __lpinit(int lpmd)
{
#ifdef CONFIG_ARCH_S5PC1XX /* S5PC100 */
	u32 mdsel;

	mdsel = readl(S5P_LPMP_MODE_SEL) & ~0x3;

	if(lpmd)
	   mdsel |= (1<<0);
	else
	   mdsel |= (1<<1);

	writel(mdsel, S5P_LPMP_MODE_SEL);
	writel(readl(S5P_CLKGATE_D20) | S5P_CLKGATE_D20_HCLKD2, S5P_CLKGATE_D20);
#else
	//#error PUT SOME CODE HERE !!!
#endif
}

static void s3c_i2s_setmode(int lpmd)
{
	s3c_i2s_pdat.lp_mode = lpmd;

	spin_lock_init(&s3c_i2s_pdat.lock);

	if(s3c_i2s_pdat.lp_mode){
	   s3c_i2s_pdat.i2s_dai.capture.channels_min = 0;
	   s3c_i2s_pdat.i2s_dai.capture.channels_max = 0;
	   s3c_i2s_pcm_stereo_out.dma_addr = S3C_IIS_PABASE + S3C_IISTXDS;
	}else{
	   s3c_i2s_pdat.i2s_dai.capture.channels_min = 1;
	   s3c_i2s_pdat.i2s_dai.capture.channels_max = 2;
	   s3c_i2s_pcm_stereo_out.dma_addr = S3C_IIS_PABASE + S3C_IISTXD;
	}

	__lpinit(s3c_i2s_pdat.lp_mode);
}

static void s3c_i2sdma_ctrl(int state)
{
	u32 val;

	spin_lock(&s3c_i2s_pdat.lock);

	val = readl(s3c_i2s.regs + S3C_IISAHB);

	switch(state){
	   case S3C_I2SDMA_START:
				  val |= S3C_IISAHB_INTENLVL0 | S3C_IISAHB_DMAEN;
		                  break;

	   case S3C_I2SDMA_SUSPEND:
	   case S3C_I2SDMA_RESUME:
		                  break;
	   case S3C_I2SDMA_FLUSH:
	   case S3C_I2SDMA_STOP: 
				  val &= ~(S3C_IISAHB_INTENLVL0 | S3C_IISAHB_DMAEN);
		                  break;

	   default:
				  spin_unlock(&s3c_i2s_pdat.lock);
				  return;
	}

	writel(val, s3c_i2s.regs + S3C_IISAHB);

	s3c_i2s_pdat.dma_state = state;

	spin_unlock(&s3c_i2s_pdat.lock);
}

struct s5p_i2s_pdata s3c_i2s_pdat = {
	.lp_mode = 0,
	.set_mode = s3c_i2s_setmode,
	.p_rate = &s3c_i2s.clk_rate,
	.dma_getpos = s3c_i2sdma_getpos,
	.dma_enqueue = s3c_i2sdma_enqueue,
	.dma_setcallbk = s3c_i2sdma_setcallbk,
	.dma_token = NULL,
	.dma_cb = NULL,
	.dma_ctrl = s3c_i2sdma_ctrl,
	.i2s_dai = {
		.name = "s3c-i2s",
		.id = 0,
		.probe = s3c_i2s_probe,
		.remove = s3c_i2s_remove,
		.suspend = s3c_i2s_suspend,
		.resume = s3c_i2s_resume,
		.playback = {
			.channels_min = 2,
			.channels_max = PLBK_CHAN,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
			},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = {
			.hw_params = s3c_i2s_hw_params,
			.prepare   = s3c_i2s_prepare,
	       		.startup   = s3c_i2s_startup,
			.shutdown  = s3c_i2s_shutdown,
			.trigger   = s3c_i2s_trigger,
			.set_fmt = s3c_i2s_set_fmt,
			.set_clkdiv = s3c_i2s_set_clkdiv,
			.set_sysclk = s3c_i2s_set_sysclk,
		},
	},
};

EXPORT_SYMBOL_GPL(s3c_i2s_pdat);

static int __init s5p_i2s_init(void)
{
	return snd_soc_register_dai(&s3c_i2s_pdat.i2s_dai);
}
module_init(s5p_i2s_init);

static void __exit s5p_i2s_exit(void)
{
	snd_soc_unregister_dai(&s3c_i2s_pdat.i2s_dai);
}
module_exit(s5p_i2s_exit);

/* Module information */
MODULE_AUTHOR("Jaswinder Singh <jassi.brar@samsung.com>");
MODULE_DESCRIPTION(S3C_DESC);
MODULE_LICENSE("GPL");
