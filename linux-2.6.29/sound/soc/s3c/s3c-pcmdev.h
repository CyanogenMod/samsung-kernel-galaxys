/*
 * s3c-pcmdev.h  --  ALSA Soc Audio Layer
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef S3C_PCMDEV_H_
#define S3C_PCMDEV_H_

//#define USE_CLKAUDIO	1 /* Don't use it for LPMP3 mode */

/* Clock dividers */
//#define S3C_DIV_MCLK	0
//#define S3C_DIV_BCLK	1
//#define S3C_DIV_PRESCALER	2

#define S3C_PCM_CTL		(0x00)
#define S3C_PCM_CLKCTL		(0x04)
#define S3C_PCM_TXFIFO		(0x08)
#define S3C_PCM_RXFIFO		(0x0C)
#define S3C_PCM_IRQ_CTL		(0x10)
#define S3C_PCM_IRQ_STAT	(0x14)

#define S3C_PCM_FIFO_STAT	(0x18)
#define S3C_PCM_CLRINT		(0x1c)

#define S3C_PCMCTL_TXFIFO_DIPSTICK_MASK		(0x3F<<13)
#define S3C_PCMCTL_TXFIFO_DIPSTICK_SHIFT	(13)
#define S3C_PCMCTL_RXFIFO_DIPSTICK_MASK		(0x3F<<7)
#define S3C_PCMCTL_RXFIFO_DIPSTICK_SHIFT	(7)
#define S3C_PCMCTL_TX_DMA_EN		(0x1<<6)
#define S3C_PCMCTL_RX_DMA_EN		(0x1<<5)
#define S3C_PCMCTL_TX_MSB_POS_MASK	(0x1<<4)
#define S3C_PCMCTL_TX_MSB_POS0		(0x0<<4)
#define S3C_PCMCTL_TX_MSB_POS1		(0x1<<4)
#define S3C_PCMCTL_RX_MSB_POS_MASK	(0x1<<3)
#define S3C_PCMCTL_RX_MSB_POS0		(0x0<<3)
#define S3C_PCMCTL_RX_MSB_POS1		(0x1<<3)
#define S3C_PCMCTL_TXFIFO_EN		(0x1<<2)	
#define S3C_PCMCTL_RXFIFO_EN		(0x1<<1)
#define S3C_PCMCTL_ENABLE		(0x1<<0)

#define S3C_PCMCLKCTL_SERCLK_EN			(0x1<<19)
#define S3C_PCMCLKCTL_SERCLK_SEL		(0x1<<18)
#define S3C_PCMCLKCTL_SCLK_DIV			(0x1FF<<9)
#define S3C_PCMCLKCTL_SYNC_DIV			(0x1FF<<0)	

#define S3C_PCMTXFIFO_DVALID			(0x1<<16)
#define S3C_PCMTXFIFO_DATA			(0xFFFF<<0)

#define S3C_PCMRXFIFO_DVALID			(0x1<<16)
#define S3C_PCMRXFIFO_DATA			(0xFFFF<<0)

#define S3C_PCMIRQ_EN_IRQ_TO_ARM		(0x1<<14)
#define S3C_PCMIRQ_TRANSFER_DONE		(0x1<<12)
#define S3C_PCMIRQ_TXFIFO_EMPTY			(0x1<<11)
#define S3C_PCMIRQ_TXFIFO_ALMOST_EMPTY		(0x1<<10)
#define S3C_PCMIRQ_TXFIFO_FULL			(0x1<<9)
#define S3C_PCMIRQ_TXFIFO_ALMOST_FULL		(0x1<<8)
#define S3C_PCMIRQ_TXFIFO_ERROR_STARVE		(0x1<<7)
#define S3C_PCMIRQ_TXFIFO_ERROR_OVERFLOW	(0x1<<6)
#define S3C_PCMIRQ_RXFIFO_EMPTY			(0x1<<5)
#define S3C_PCMIRQ_RXFIFO_ALMOST_EMPTY		(0x1<<4)
#define S3C_PCMIRQ_RX_FIFO_FULL			(0x1<<3)
#define S3C_PCMIRQ_RX_FIFO_ALMOST_FULL		(0x1<<2)	
#define S3C_PCMIRQ_RXFIFO_ERROR_STARVE		(0x1<<1)	
#define S3C_PCMIRQ_RXFIFO_ERROR_OVERFLOW	(0x1<<0)	

#define S3C_PCMIRQSTAT_IRQ_PENDING		(0x1<<13)	
#define S3C_PCMIRQSTAT_TRANSFER_DONE		(0x1<<12)	
#define S3C_PCMIRQSTAT_TXFIFO_EMPTY		(0x1<<11)	
#define S3C_PCMIRQSTAT_TXFIFO_ALMOST_EMPTY	(0x1<<10)	
#define S3C_PCMIRQSTAT_TXFIFO_FULL		(0x1<<9)	
#define S3C_PCMIRQSTAT_TXFIFO_ALMOST_FULL	(0x1<<8)	
#define S3C_PCMIRQSTAT_TXFIFO_ERROR_STARVE	(0x1<<7)	
#define S3C_PCMIRQSTAT_TXFIFO_ERROR_OVERFLOW	(0x1<<6)	
#define S3C_PCMIRQSTAT_RXFIFO_EMPTY		(0x1<<5)	
#define S3C_PCMIRQSTAT_RXFIFO_ALMOST_EMPTY	(0x1<<4)	
#define S3C_PCMIRQSTAT_RX_FIFO_FULL		(0x1<<3)	
#define S3C_PCMIRQSTAT_RX_FIFO_ALMOST_FULL	(0x1<<2)	
#define S3C_PCMIRQSTAT_RXFIFO_ERROR_STARVE	(0x1<<1)	
#define S3C_PCMIRQSTAT_RXFIFO_ERROR_OVERFLOW	(0x1<<0)	

#define S3C_PCMFIFOSTAT_TXFIFO_COUNT		(0x3F<<14)	
#define S3C_PCMFIFOSTAT_TXFIFO_EMPTY		(0x1<<13)	
#define S3C_PCMFIFOSTAT_TXFIFO_ALMOST_EMPTY	(0x1<<12)	
#define S3C_PCMFIFOSTAT_TXFIFO_FULL		(0x1<<11)	
#define S3C_PCMFIFOSTAT_TXFIFO_ALMOST_FULL	(0x1<<10)	
#define S3C_PCMFIFOSTAT_RXFIFO_COUNT		(0x3F<<4)	
#define S3C_PCMFIFOSTAT_RXFIFO_EMPTY		(0x1<<3)	
#define S3C_PCMFIFOSTAT_RXFIFO_ALMOST_EMPTY	(0x1<<2)	
#define S3C_PCMFIFOSTAT_RX_FIFO_FULL		(0x1<<1)	
#define S3C_PCMFIFOSTAT_RX_FIFO_ALMOST_FULL	(0x1<<0)	


#define PCM_ID			1

#if (PCM_ID == 0)
#define IRQ_S3C_PCM		IRQ_PCM0
#define S3C_PA_PCM		S5PC11X_PA_PCM0	
#define DMACH_PCMDEV_OUT	DMACH_PCM0_OUT
#define DMACH_PCMDEV_IN		DMACH_PCM0_IN
#define PCLKCLK			"pcm"
#define EXTCLK			"sclk_audio0"
#elif (PCM_ID == 1)
#define IRQ_S3C_PCM		IRQ_PCM1
#define S3C_PA_PCM		S5PC11X_PA_PCM1	
#define DMACH_PCMDEV_OUT	DMACH_PCM1_OUT
#define DMACH_PCMDEV_IN		DMACH_PCM1_IN
#define PCLKCLK			"pcm"
#define EXTCLK			"sclk_audio1"
#elif (PCM_ID == 2)
#define IRQ_S3C_PCM		IRQ_PCM0
#define S3C_PA_PCM		S5PC11X_PA_PCM2	
#define DMACH_PCMDEV_OUT	DMACH_PCM2_OUT
#define DMACH_PCMDEV_IN		DMACH_PCM2_IN
#define PCLKCLK			"pcm"
#define EXTCLK			"sclk_audio2"
#else
#error "PCM Device ID is not set."
#endif

#define S3C_PA_VIC		S3C_VA_VIC2	
#define RATESRCCLK              "fout_epll"
#define EXTPRNT			"mout_epll"
//#define S3C_PCMCDCLK_FREQ	16934400	/* 16.9344MHz - defined in the data sheet */
//#define S3C_PCMCDCLK_FREQ       11289600 //for 256

#define PLBK_CHAN		6
#define S3C_DESC		"S3C AP PCM Interface"

/* dma_state */
#define S3C_I2SDMA_START   1
#define S3C_I2SDMA_STOP    2
#define S3C_I2SDMA_FLUSH   3

u32 s3c_i2s_get_clockrate(void);
//below functions added to use in interrupt mode
int s3c_pcmdev_fillfifo(struct snd_pcm_substream *substream,unsigned char* pos,unsigned long len);
void s3c_pcmdev_stopfifo(void);
void s3c_pcmdev_getposition(char *src,char *dst);

extern struct snd_soc_dai s3c_pcmdev_dai;

#endif /*S3C_PCMDEV_H_*/
