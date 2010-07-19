/*
 * s3c-pcmdev.c  --  ALSA Soc Audio Layer
 *
 * (c) 2009 Samsung Electronics   - 
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

#include "s3c-pcm.h"
#include "s3c-pcmdev.h"
//#define CONFIG_SND_DEBUG

#define USE_INFINIEON_EC_FOR_VT

#ifdef CONFIG_SND_DEBUG
#define debug_msg(x...) printk(x)
#else
#define debug_msg(x...)
#endif

static u32 remaining_len; //..added to use interrupt mode
//static dma_addr_t curr_pos;//.. added to use interrupt mode
static unsigned char* curr_pos;//.. added to use interrupt mode
static struct snd_pcm_substream *req_stream;//.. added to use interrupt mode
static u32 req_len;//..added to use interrupt mode

#if defined USE_INFINIEON_EC_FOR_VT
static unsigned char vtcallActive = 0;
#endif


/*
struct s3c_pcmdev_buff{
	static dma_addr_t curr_pos;
	};//added to use interrupt mode
*/


static struct s3c2410_dma_client s3c_dma_client_out = {
	.name = "PCM Stereo out"
};

static struct s3c2410_dma_client s3c_dma_client_in = {
	.name = "PCM Stereo in"
};

static struct s3c24xx_pcm_dma_params s3c_pcmdev_pcm_stereo_out = {
	.client		= &s3c_dma_client_out,
	.channel	= DMACH_PCMDEV_OUT,
	.dma_addr	= S3C_PA_PCM + S3C_PCM_TXFIFO ,
	.dma_size	= 4,
};

static struct s3c24xx_pcm_dma_params s3c_pcmdev_pcm_stereo_in = {
	.client		= &s3c_dma_client_in,
	.channel	= DMACH_PCMDEV_IN,
	.dma_addr	= S3C_PA_PCM + S3C_PCM_RXFIFO ,
	.dma_size	= 4,
};

struct s3c_pcmdev_info {
	void __iomem	*regs;
	void __iomem 	*vic_regs;
	struct clk	*pcm_clk;
	struct clk	*clk_src;
	u32		clk_rate;
	u32		pcmctl;
	u32		pcmclkctl;
	u32		pcmirqctl;
} ;

static struct s3c_pcmdev_info s3c_pcmdev;
static u32 clk_en_dis_play  = 0;
static u32 clk_en_dis_rec  = 0;

static void s3c_snd_txctrl(int on)
{
	u32 value;

	debug_msg("%s\n", __FUNCTION__);

	value = readl(s3c_pcmdev.regs + S3C_PCM_CTL);

	if(on){
		value &= ~(S3C_PCMCTL_TXFIFO_DIPSTICK_MASK | S3C_PCMCTL_TX_DMA_EN | S3C_PCMCTL_TXFIFO_EN);
		value |= (0x8<<S3C_PCMCTL_TXFIFO_DIPSTICK_SHIFT) 
			| S3C_PCMCTL_TX_DMA_EN 
			| S3C_PCMCTL_TXFIFO_EN; 
		
		writel(value, s3c_pcmdev.regs + S3C_PCM_CTL); 
		//writel(readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL) | S3C_PCMCLKCTL_SERCLK_EN, s3c_pcmdev.regs + S3C_PCM_CLKCTL); 
		//writel(readl(s3c_pcmdev.regs +S3C_PCM_CTL) | S3C_PCMCTL_ENABLE, s3c_pcmdev.regs + S3C_PCM_CTL);
		debug_msg("In..%s..PCM_CTL=0X%x..PCM_CLKCTL=0X%x..@%d..\n", __FUNCTION__,readl(s3c_pcmdev.regs +S3C_PCM_CTL),readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL),__LINE__);
	} else {
		value &= ~(S3C_PCMCTL_TX_DMA_EN | S3C_PCMCTL_TXFIFO_EN );
		writel(value, s3c_pcmdev.regs + S3C_PCM_CTL);
		//writel(readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL) & ~S3C_PCMCLKCTL_SERCLK_EN, s3c_pcmdev.regs + S3C_PCM_CLKCTL);
		value = readl(s3c_pcmdev.regs +S3C_PCM_IRQ_CTL); 
		value &= ~(0x7FFF);
		debug_msg("In..%s..PCM_CTL=0X%x..PCM_CLKCTL=0X%x..@%d..\n", __FUNCTION__,readl(s3c_pcmdev.regs +S3C_PCM_CTL),readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL),__LINE__);
	}
debug_msg("In..%s..PCM_IRQ_STAT=0X%x..PCM_FIFO_STAT=0X%x..IRQ_CTL=0x%x@%d..\n", __FUNCTION__,readl(s3c_pcmdev.regs +S3C_PCM_IRQ_STAT),readl(s3c_pcmdev.regs + S3C_PCM_FIFO_STAT),readl(s3c_pcmdev.regs +S3C_PCM_IRQ_CTL),__LINE__);
}

static void s3c_snd_rxctrl(int on)
{
	u32 value;

	debug_msg("%s\n", __FUNCTION__);
	if(on){
		value = readl(s3c_pcmdev.regs + S3C_PCM_CTL);
		value &= ~(S3C_PCMCTL_RXFIFO_DIPSTICK_MASK | S3C_PCMCTL_RX_DMA_EN | S3C_PCMCTL_RXFIFO_EN);
		value |= (0x8<<S3C_PCMCTL_RXFIFO_DIPSTICK_SHIFT) 
			| S3C_PCMCTL_RX_DMA_EN 
			| S3C_PCMCTL_RXFIFO_EN ;
		//	| S3C_PCMCTL_ENABLE;
		writel(value, s3c_pcmdev.regs + S3C_PCM_CTL); 
		//writel(readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL) | S3C_PCMCLKCTL_SERCLK_EN, s3c_pcmdev.regs + S3C_PCM_CLKCTL); 
		debug_msg("...check point..%s..PCM_CTL=0X%x..PCM_CLKCTL=0X%x..@%d..\n", __FUNCTION__,readl(s3c_pcmdev.regs +S3C_PCM_CTL),readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL),__LINE__);
	} else {
		value = readl(s3c_pcmdev.regs + S3C_PCM_CTL);
		value &= ~(S3C_PCMCTL_RX_DMA_EN | S3C_PCMCTL_RXFIFO_EN );
		writel(value, s3c_pcmdev.regs + S3C_PCM_CTL);
		//writel(readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL) & ~S3C_PCMCLKCTL_SERCLK_EN, s3c_pcmdev.regs + S3C_PCM_CLKCTL);
		debug_msg("...check point..%s..PCM_CTL=0X%x..PCM_CLKCTL=0X%x..@%d..\n", __FUNCTION__,readl(s3c_pcmdev.regs +S3C_PCM_CTL),readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL),__LINE__); 
	}
	debug_msg("...check point..%s..PCM_IRQ_STAT=0X%x..PCM_FIFO_STAT=0X%x..IRQ_CTL=0x%x@%d..\n", __FUNCTION__,readl(s3c_pcmdev.regs +S3C_PCM_IRQ_STAT),readl(s3c_pcmdev.regs + S3C_PCM_FIFO_STAT),readl(s3c_pcmdev.regs +S3C_PCM_IRQ_CTL),__LINE__);
        debug_msg("..check point in %s...S5P_CLK_SRC6=0x%x..S5P_CLKGATE_IP3=0x%x\n",__func__,readl(S5P_CLK_SRC6),readl(S5P_CLKGATE_IP3));
}

static int s3c_pcmdev_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	u32 value;
	debug_msg("Entered %s\n", __FUNCTION__);

	value = readl(s3c_pcmdev.regs + S3C_PCM_CTL);
	value &= ~(S3C_PCMCTL_TX_MSB_POS_MASK | S3C_PCMCTL_RX_MSB_POS_MASK);
        
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_I2S: /* Data starts (MSB) 1 clock after PCMSYNC signal */
		value |= S3C_PCMCTL_TX_MSB_POS1 | S3C_PCMCTL_RX_MSB_POS1;
                break;
	case SND_SOC_DAIFMT_DSP_B:
		value |= S3C_PCMCTL_TX_MSB_POS0 | S3C_PCMCTL_RX_MSB_POS0;
                break;
	case SND_SOC_DAIFMT_DSP_A:
                value |= S3C_PCMCTL_TX_MSB_POS1 | S3C_PCMCTL_RX_MSB_POS1;
                break;
        default:
		debug_msg("Invalid DAI format specified - 0x%x\n", fmt & SND_SOC_DAIFMT_FORMAT_MASK);
                return -EINVAL;
        }

	writel(value, s3c_pcmdev.regs + S3C_PCM_CTL);
	debug_msg("...check point..%s..PCM_CTL=0X%x..PCM_CLKCTL=0X%x\n", __FUNCTION__,readl(s3c_pcmdev.regs +S3C_PCM_CTL),readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL));
	return 0;
}

static int s3c_pcmdev_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	u32 value,clk_rate;
	u32 bfs, rfs,sclk_div, sync_div,clk_div;
	u32 audioclk,tmp;	
	struct clk *clk;

	debug_msg("%s\n", __FUNCTION__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		rtd->dai->cpu_dai->dma_data = &s3c_pcmdev_pcm_stereo_out;
	else
		rtd->dai->cpu_dai->dma_data = &s3c_pcmdev_pcm_stereo_in;


	switch (params_format(params)) {
        case SNDRV_PCM_FORMAT_S8:
                bfs = 16;
                rfs = 256;              /* Can take any RFS value for AP */
                break;
        case SNDRV_PCM_FORMAT_S16_LE:
                bfs = 32;
                rfs = 256;              /* Can take any RFS value for AP */
                break;
        case SNDRV_PCM_FORMAT_S20_3LE:
        case SNDRV_PCM_FORMAT_S24_LE:
                bfs = 48;
                rfs = 512;              /* B'coz 48-BFS needs atleast 512-RFS acc to *S5P6440* UserManual */
                break;
        case SNDRV_PCM_FORMAT_S32_LE:   /* Impossible, as the AP doesn't support 64fs or more BFS */
        default:
		printk("###in function..%s..Default format not supported..\n",__func__);
                return -EINVAL;
        }


	/* Enable the interrupts */
	value = readl(s3c_pcmdev.regs + S3C_PCM_IRQ_CTL);
	value |= //0x1FF;//S3C_PCMIRQ_TRANSFER_DONE
		 //S3C_PCMIRQSTAT_TXFIFO_EMPTY;
		 S3C_PCMIRQSTAT_TXFIFO_ALMOST_EMPTY;
		//| S3C_PCMIRQSTAT_TXFIFO_FULL;
	value |= S3C_PCMIRQ_EN_IRQ_TO_ARM;
	//writel(value, s3c_pcmdev.regs + S3C_PCM_IRQ_CTL);//..for using interrupt mode	
	
	clk = clk_get(NULL, RATESRCCLK);
                if (IS_ERR(clk)) {
                        printk("failed to get %s\n", RATESRCCLK);
                        return -EBUSY;
                }
        clk_disable(clk);     
	clk_div = readl(S5P_CLK_DIV6);
	clk_div &= ~(0xF<<4); 
         
	switch (params_rate(params)) {
	case 8000:
		clk_set_rate(clk, 49152000);
		clk_div |= (3<<4);
		writel(clk_div,S5P_CLK_DIV6);
                audioclk = (49152000/4);
                break;
        case 16000:
		clk_set_rate(clk, 49152000);
		clk_div |= (1<<4);
		writel(clk_div,S5P_CLK_DIV6);
                audioclk = (49152000/2);
                break;
        case 32000:
		clk_set_rate(clk, 49152000);
                audioclk = 49152000;
		clk_div |= (0<<4);
                writel(clk_div,S5P_CLK_DIV6);
                break;
                	
	case 64000:	
		clk_set_rate(clk, 49152000);
                audioclk = 49152000;
		clk_div |= (0<<4);
                writel(clk_div,S5P_CLK_DIV6);
                break;


       	case 48000:
		clk_set_rate(clk, 73728000);
                audioclk = 73728000;
		clk_div |= (0<<4);
                writel(clk_div,S5P_CLK_DIV6);
                break;

       	case 96000:
                clk_set_rate(clk, 73728000);
		audioclk = 73728000;
		clk_div |= (0<<4);
                writel(clk_div,S5P_CLK_DIV6);
                break;
       	case 11025:
		clk_set_rate(clk, 67738000);
                audioclk = 67738000/4;
		clk_div |= (3<<4);
                writel(clk_div,S5P_CLK_DIV6);
		break;

       	case 22050:
		clk_set_rate(clk, 67738000);
                audioclk = 67738000/2;
		clk_div |= (1<<4);
                writel(clk_div,S5P_CLK_DIV6);
		break;

       	case 44100:	
		clk_set_rate(clk, 67738000);
                audioclk = 67738000;
		clk_div |= (0<<4);
                writel(clk_div,S5P_CLK_DIV6);
		break;
					
      	case 88200:
                clk_set_rate(clk, 67738000);
		audioclk = 67738000;
		clk_div |= (0<<4);
                writel(clk_div,S5P_CLK_DIV6);
               	break;
       	default:
       		printk("Required Rate =%d..not supported\n",params_rate(params));
		return -EINVAL;
        }
	
	clk_rate = (params_rate(params)*bfs);
	tmp = ((audioclk/(clk_rate))/2)-1;
	debug_msg(".....clk_rate=%d...sclk_div=%d..clk_div6=0x%x..smaple_rate=%d..\n",clk_rate,tmp,readl(S5P_CLK_DIV6),params_rate(params));
	sclk_div = tmp;

	sync_div = bfs-1;

	value = readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL);
	value &= ~(S3C_PCMCLKCTL_SCLK_DIV | S3C_PCMCLKCTL_SYNC_DIV);
        value |= (sclk_div << 9);
        value |= (sync_div << 0);

        writel(value,s3c_pcmdev.regs + S3C_PCM_CLKCTL);

	clk_enable(clk);
        s3c_pcmdev.clk_rate = clk_get_rate(s3c_pcmdev.clk_src);
        debug_msg("Setting FOUTepll to %dHz\n",s3c_pcmdev.clk_rate);

        s3c_pcmdev.clk_rate = clk_get_rate(s3c_pcmdev.pcm_clk);
        clk_put(clk);
        
        debug_msg("....checkpoint...%s...S3C_PCM_CLKCTL=0x%x\n", __FUNCTION__,readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL));
  	return 0;
}
static bool value_saved = false;
static int s3c_pcmdev_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	debug_msg("%s\n", __FUNCTION__);

#if defined USE_INFINIEON_EC_FOR_VT
	if(vtcallActive == 1)
	{
		debug_msg("%s : clock is already setup!!",__FUNCTION__);
		return 0;
	}
#endif
		
	if(!clk_en_dis_rec && !clk_en_dis_play)
                {
			clk_enable(s3c_pcmdev.clk_src);
			clk_enable(s3c_pcmdev.pcm_clk);
			if(value_saved == true){
				writel(s3c_pcmdev.pcmctl, s3c_pcmdev.regs + S3C_PCM_CTL);
			        writel(s3c_pcmdev.pcmclkctl, s3c_pcmdev.regs + S3C_PCM_CLKCTL);
			        writel(s3c_pcmdev.pcmirqctl, s3c_pcmdev.regs + S3C_PCM_IRQ_CTL);
				value_saved = false;
			}
		        printk("..enabled pcm clocks finally..\n");
		}
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		clk_en_dis_play =1 ;
	else
		clk_en_dis_rec = 1 ;
	
	writel(readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL) | S3C_PCMCLKCTL_SERCLK_EN, s3c_pcmdev.regs + S3C_PCM_CLKCTL); 
	writel(readl(s3c_pcmdev.regs +S3C_PCM_CTL) | S3C_PCMCTL_ENABLE, s3c_pcmdev.regs + S3C_PCM_CTL);
	debug_msg("In..%s..PCM_CTL=0X%x..PCM_CLKCTL=0X%x..@%d..\n", __FUNCTION__,readl(s3c_pcmdev.regs +S3C_PCM_CTL),readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL),__LINE__);
	return 0;
}

static void s3c_pcmdev_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	u32 value;

#if defined USE_INFINIEON_EC_FOR_VT
	if(vtcallActive == 1)
	{
		debug_msg("%s : vtcall is using!!", __FUNCTION__);
		return;		
	}
#endif
	
	value = readl(s3c_pcmdev.regs + S3C_PCM_CTL);
	value &= ~(S3C_PCMCTL_ENABLE);
        writel(value, s3c_pcmdev.regs + S3C_PCM_CTL);
        writel(readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL) & ~S3C_PCMCLKCTL_SERCLK_EN, s3c_pcmdev.regs + S3C_PCM_CLKCTL);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
                clk_en_dis_play = 0 ;
        else
                clk_en_dis_rec =  0 ;
	
	if((!clk_en_dis_play)&&(!clk_en_dis_rec))
	{
		if(value_saved == true)
			return;
		s3c_pcmdev.pcmctl= readl(s3c_pcmdev.regs + S3C_PCM_CTL);
	        s3c_pcmdev.pcmclkctl = readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL);
        	s3c_pcmdev.pcmirqctl = readl(s3c_pcmdev.regs + S3C_PCM_IRQ_CTL);
		value_saved = true;
		clk_disable(s3c_pcmdev.pcm_clk);
		clk_disable(s3c_pcmdev.clk_src);
		//printk("..disabled pcm clocks finally..\n");
	}
}
static int s3c_pcmdev_prepare(struct snd_pcm_substream *substream,struct snd_soc_dai *dai)
{

	debug_msg("%s\n", __FUNCTION__);
	return 0;
}

static int s3c_pcmdev_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	debug_msg("%s\n", __FUNCTION__);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		debug_msg("Pcm trigger start - %d\n", cmd);
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s3c_snd_rxctrl(1);
		else
			s3c_snd_txctrl(1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		debug_msg("Pcm trigger stop - %d\n", cmd);
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s3c_snd_rxctrl(0);
		else
			s3c_snd_txctrl(0);
		break;
	default:
		printk("Invalid pcm trigger cmd - %d\n", cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int s3c_pcmdev_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{
	u32 value;	
	debug_msg("%s\n", __FUNCTION__);
	s3c_pcmdev.clk_rate = clk_get_rate(s3c_pcmdev.pcm_clk);
	value = readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL);
	value |= ~(S3C_PCMCLKCTL_SERCLK_SEL); // ..using  SCLK_* clock source	
	//value |= (S3C_PCMCLKCTL_SERCLK_SEL); // ..for using PCLK_* clock source 
	writel(value,s3c_pcmdev.regs + S3C_PCM_CLKCTL);
	
	return 0;
}

static int s3c_pcmdev_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	return 0;
}

/*
 * To avoid duplicating clock code, allow machine driver to
 * get the clockrate from here.
 */
u32 s3c_pcmdev_get_clockrate(void)
{
	debug_msg("%s\n", __FUNCTION__);
	return s3c_pcmdev.clk_rate;
}
EXPORT_SYMBOL_GPL(s3c_pcmdev_get_clockrate);

//TODO
// ..added below to use in interrupt mode instead of DMA mode... Interrupt mode has to be implemented properly.
int s3c_pcmdev_fillfifo(struct snd_pcm_substream *substream,unsigned char* pos,unsigned long len)
{
#if 0
	int count=0,wordloop;
	req_stream = substream;
	req_len	   = len;
	static int flip =0;
	u16 temp=0;
	u32 cur_count = 0;
	curr_pos = pos;
	printk("...requested length=%ld...@..%d..\n",len,__LINE__);
	while((count<32) && len && (cur_count<32))
	{
//	if(flip==0)
//	{	
//		printk("..check point.@=%d..*pos=0x%x..\n",__LINE__,*(u32 *)curr_pos);
		for(wordloop=0;wordloop<2;wordloop++)
			{
			  temp |= (*curr_pos)<<(wordloop*8);
			  curr_pos ++;
			} 
		//temp = *(unsigned short*)curr_pos;
		//(unsigned short*)curr_pos ++ ;
		writel(temp,s3c_pcmdev.regs + S3C_PCM_TXFIFO);
//		flip = 1;
		temp = 0;
//	}
//	else
//	{
//		printk("..check point.@=%d..curr_pos=0x%x..\n",__LINE__,curr_pos);
//		writel((*(u32 *)curr_pos) & (~(0xFFFF)),s3c_pcmdev.regs + S3C_PCM_TXFIFO);
//		flip = 0;
//		curr_pos = curr_pos +1 ;	
//	}
	count ++;
	len = len -2;
	cur_count = readl(s3c_pcmdev.regs + S3C_PCM_FIFO_STAT);
	cur_count = (cur_count >> 14) & 0x3F;
//	printk("..check point.@=%d..loop executed count=%d..\n",__LINE__,count);
	}	
	remaining_len = len;
	printk("..check point.@=%d..curr_pos=0x%x..cur_count=%d..\n",__LINE__,(u32)curr_pos,cur_count);
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(s3c_pcmdev_fillfifo);
//TODO
void s3c_pcmdev_getposition(char * src,char *dst)
{
	printk("checkpoint ..in %s..\n",__func__);
	src = curr_pos;
}
EXPORT_SYMBOL_GPL(s3c_pcmdev_getposition);
//TODO
void s3c_pcmdev_stopfifo(void)
{
	int value = 0;
	s3c_snd_txctrl(0);
	printk("checkpoint ..in %s..value=0x%x..\n",__func__,value);
	//value = readl(s3c_pcmdev.vic_regs + 0x14);
	//value |= (1<<21);
//	printk("checkpoint ..in %s..value=0x%x..status=0x%x\n",__func__,value,readl(s3c_pcmdev.vic_regs + 0x00));
	//writel(value,s3c_pcmdev.vic_regs + 0x14);
	//writel(0,s3c_pcmdev.vic_regs + 0xF00);
	//disable_irq(IRQ_S3C_PCM);

}
EXPORT_SYMBOL_GPL(s3c_pcmdev_stopfifo);

// added to use in interrupt mode..interrupt mode implementation has to be done
/* TODO */
static irqreturn_t s3c_pcmdev_irq(int irqno, void *dev_id)
{
	u32 irq_status;
	static u32 count;

	irq_status = readl(s3c_pcmdev.regs + S3C_PCM_IRQ_STAT);
printk("%s] irq count %d, status = 0x%x..fifo_stat =0x%x\n", __func__, count++, irq_status,readl(s3c_pcmdev.regs + S3C_PCM_FIFO_STAT));
	debug_msg("%s\n", __FUNCTION__);
	//printk("..irq_ctl=0x%x..VIC=0x%x..\n",readl(s3c_pcmdev.regs +S3C_PCM_IRQ_CTL),readl(S5PC11X_PA_VIC2+0x04));

	printk("..IRQ..check point..curr_pos=0x%x..\n",(u32)curr_pos);
	if(remaining_len)
	{
	 	s3c_pcmdev_fillfifo(req_stream,curr_pos,remaining_len);//..added to use interrupt mode 
		writel(0x1,s3c_pcmdev.regs + S3C_PCM_CLRINT);// ..added to clear interrupt
	}
	else
	{
	 //s3c24xx_pcm_buffdone(req_stream,req_len);//..implementation will be in s3c-pcm.c 
	writel(0x1,s3c_pcmdev.regs + S3C_PCM_CLRINT);
	}

	return IRQ_HANDLED;
}

#if defined USE_INFINIEON_EC_FOR_VT
int s3c_pcmdev_clock_control(int enable)
{
	vtcallActive = enable;
}
#endif

static int s3c_pcmdev_probe(struct platform_device *pdev,
			     struct snd_soc_dai *dai)
{
	int ret = 0;
	struct clk *cm, *cf;

	debug_msg("+++++++%s\n", __FUNCTION__);

	s3c_pcmdev.regs = ioremap(S3C_PA_PCM, 0x100);
	if (s3c_pcmdev.regs == NULL)
		return -ENXIO;
	//s3c_pcmdev.vic_regs = ioremap(S3C_PA_VIC,0xFFC);//..used for checking in interrupt mode
	//if (s3c_pcmdev.vic_regs == NULL)
        //        return -ENXIO;

	ret = request_irq(IRQ_S3C_PCM, s3c_pcmdev_irq, 0, "s3c-pcmdev", pdev);
	if (ret < 0) {
		printk("fail to claim pcmdev irq , ret = %d\n", ret);
		iounmap(s3c_pcmdev.regs);
		return -ENODEV;
	}


	s3c_pcmdev.pcm_clk = clk_get(&pdev->dev, "pcm");
        if (IS_ERR(s3c_pcmdev.pcm_clk)) {
                printk("failed to get clk(pcm1)\n");
                goto err_exit4;
        }
        //clk_enable(s3c_pcmdev.pcm_clk);
        s3c_pcmdev.clk_rate = clk_get_rate(s3c_pcmdev.pcm_clk);

	s3c_pcmdev.clk_src = clk_get(NULL, EXTCLK);
        if (IS_ERR(s3c_pcmdev.clk_src)) {
                printk("failed to get clk(%s)\n", EXTCLK);
		goto err_exit3;
        }

	cm = clk_get(NULL, EXTPRNT);
        if (IS_ERR(cm)) {
                printk("failed to get %s\n", EXTPRNT);
                goto err_exit2;
        }
        s3cdbg("Got Audio Bus Source Clock -> %s\n", EXTPRNT);

	if(clk_set_parent(s3c_pcmdev.clk_src, cm)){
                printk("failed to set mOUTepll as parent of scklkaudio1\n");
                goto err_exit1;
                }
                
	 cf = clk_get(NULL, "fout_epll");
        if (IS_ERR(cf)) {
                printk("failed to get fout_epll\n");
                goto err_exit1;
        }

        if(clk_set_parent(cm, cf)){
                printk("failed to set FOUTepll as parent of MOUTepll\n");
                goto err_exit0;
        }

        s3cdbg("Set fout_epll as parent of %s\n", EXTPRNT);


       // clk_enable(s3c_pcmdev.clk_src);
        s3c_pcmdev.clk_rate = clk_get_rate(s3c_pcmdev.clk_src);

	debug_msg("....checkpoint..set..EXTclock rate..%d..@..%d..\n",s3c_pcmdev.clk_rate,__LINE__);
	s3c_pcmdev.clk_rate = clk_get_rate(s3c_pcmdev.pcm_clk);
        debug_msg("....checkpoint..set..PCMclock rate..%d..@..%d..\n",s3c_pcmdev.clk_rate,__LINE__);
	clk_put(cf);
        clk_put(cm);
	s3c_snd_txctrl(0);
	s3c_snd_rxctrl(0);

	debug_msg("----------%s\n", __func__);
	return 0;
err_exit0:
	clk_put(cf);
err_exit1:
	clk_put(cm);
err_exit2:
	clk_put(s3c_pcmdev.clk_src);
err_exit3:
	clk_disable(s3c_pcmdev.pcm_clk);
        clk_put(s3c_pcmdev.pcm_clk);
err_exit4:
	free_irq(IRQ_S3C_PCM, pdev);
	iounmap(s3c_pcmdev.regs);
	
	return -ENODEV;
}

static void s3c_pcmdev_remove(struct platform_device *pdev,
		       struct snd_soc_dai *dai)
{
	clk_disable(s3c_pcmdev.clk_src);
	clk_put(s3c_pcmdev.clk_src);
	clk_disable(s3c_pcmdev.pcm_clk);
	clk_put(s3c_pcmdev.pcm_clk);
	free_irq(IRQ_S3C_PCM, pdev);
	iounmap(s3c_pcmdev.regs);
}

#ifdef CONFIG_PM
static int s3c_pcmdev_suspend(struct snd_soc_dai *dai)
{
	debug_msg("%s\n", __FUNCTION__);
#if 1
	s3c_pcmdev.pcmctl= readl(s3c_pcmdev.regs + S3C_PCM_CTL);
	s3c_pcmdev.pcmclkctl = readl(s3c_pcmdev.regs + S3C_PCM_CLKCTL);
	s3c_pcmdev.pcmirqctl = readl(s3c_pcmdev.regs + S3C_PCM_IRQ_CTL);

	if((clk_en_dis_play)||(clk_en_dis_rec))
	{
		clk_disable(s3c_pcmdev.pcm_clk);
        	clk_disable(s3c_pcmdev.clk_src);
	}
	debug_msg("%s-done\n", __FUNCTION__);
#endif
	return 0;
}

static int s3c_pcmdev_resume(struct snd_soc_dai *dai)
{
	debug_msg("%s\n", __FUNCTION__);
#if 1 
	if((clk_en_dis_play)||(clk_en_dis_rec))
	{
		clk_enable(s3c_pcmdev.pcm_clk);
        	clk_enable(s3c_pcmdev.clk_src);
	}

	writel(s3c_pcmdev.pcmctl, s3c_pcmdev.regs + S3C_PCM_CTL);
	writel(s3c_pcmdev.pcmclkctl, s3c_pcmdev.regs + S3C_PCM_CLKCTL);
	writel(s3c_pcmdev.pcmirqctl, s3c_pcmdev.regs + S3C_PCM_IRQ_CTL);

	debug_msg("%s-done\n", __FUNCTION__);
#endif
	return 0;
}
#else
#define s3c_pcmdev_suspend NULL
#define s3c_pcmdev_resume NULL
#endif

struct snd_soc_dai s3c_pcmdev_dai = {
	.name = "s3c-pcmdev",
	.id = PCM_ID,
	.probe = s3c_pcmdev_probe,
	.remove = s3c_pcmdev_remove,
	.suspend = s3c_pcmdev_suspend,
	.resume = s3c_pcmdev_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_96000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U16_LE ,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_96000,
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = {
		.hw_params = s3c_pcmdev_hw_params,
		.prepare   = s3c_pcmdev_prepare,
		.startup   = s3c_pcmdev_startup,
		.shutdown  = s3c_pcmdev_shutdown,
		.trigger   = s3c_pcmdev_trigger,
		.set_fmt = s3c_pcmdev_set_fmt,
		.set_clkdiv = s3c_pcmdev_set_clkdiv,
		.set_sysclk = s3c_pcmdev_set_sysclk,
	},
};
EXPORT_SYMBOL_GPL(s3c_pcmdev_dai);

static int __init s3c_pcmdev_init(void)
{
	debug_msg("%s\n", __FUNCTION__);
	return snd_soc_register_dai(&s3c_pcmdev_dai);
}
module_init(s3c_pcmdev_init);

static void __exit s3c_pcmdev_exit(void)
{
	debug_msg("%s\n", __FUNCTION__);
	snd_soc_unregister_dai(&s3c_pcmdev_dai);
}
module_exit(s3c_pcmdev_exit);
//End


/* Module information */
MODULE_AUTHOR("Sayanta");
MODULE_DESCRIPTION(S3C_DESC);
MODULE_LICENSE("GPL");
