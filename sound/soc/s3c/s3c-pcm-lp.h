/*
 *  s5p-pcm.h --
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  ALSA PCM interface for the Samsung CPU
 */

#ifndef _S5P_PCM_H
#define _S5P_PCM_H

#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

#define ST_RUNNING		(1<<0)
#define ST_OPENED		(1<<1)

#ifdef CONFIG_ARCH_S5PC1XX /* S5PC100 */
#define MAX_LP_BUFF	(128 * 1024) /* 128KB in S5PC100 */
#else
#define MAX_LP_BUFF	(160 * 1024) /* 160KB in S5PC110 (Unused RP) */
#endif

#define LP_TXBUFF_ADDR    (0xC0000000)

struct s5p_pcm_dma_params {
	struct s3c2410_dma_client *client;	/* stream identifier */
	int channel;				/* Channel ID */
	dma_addr_t dma_addr;
	int dma_size;			/* Size of the DMA transfer */
};

struct pcm_buffs {
	unsigned char    *cpu_addr[2]; /* Vir addr of Playback and Capture stream */
	dma_addr_t        dma_addr[2];  /* DMA addr of Playback and Capture stream */
};

struct s5p_pcm_pdata {
	int               lp_mode;
	struct pcm_buffs  nm_buffs;
	struct pcm_buffs  lp_buffs;
	struct snd_soc_platform pcm_pltfm;
	struct snd_pcm_hardware pcm_hw_tx;
	struct snd_pcm_hardware pcm_hw_rx;
	void (*set_mode)(int lp_mode, void *ptr);
};

#endif
