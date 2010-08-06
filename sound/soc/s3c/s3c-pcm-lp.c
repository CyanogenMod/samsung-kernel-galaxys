/*
 * s3c-pcm.c  --  ALSA Soc Audio Layer
 *
 * (c) 2006 Wolfson Microelectronics PLC.
 * Graeme Gregory graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * (c) 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/s3c-dma.h>
#include <mach/audio.h>

#include "s3c-pcm-lp.h"
#include "s3c-i2s.h"

/* Set LP_DMA_PERIOD to maximum possible size without latency issues with playback.
 * Keep LP_DMA_PERIOD > MAX_LP_BUFF/2
 * Also, this value must be aligned at KB bounday (multiple of 1024). */
#define LP_DMA_PERIOD (105 * 1024) //when LCD is enabled
//#define LP_DMA_PERIOD (125 * 1024)   //when LCD is disabled

struct s5p_pcm_pdata s3c_pcm_pdat;

struct s5p_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	unsigned int dma_size;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	struct s5p_pcm_dma_params *params;
};

static struct s5p_i2s_pdata *s3ci2s_func = NULL;

extern unsigned int ring_buf_index;
extern unsigned int period_index;

/* s5p_pcm_enqueue
 *
 * place a dma buffer onto the queue for the dma system
 * to handle.
 */
static void s5p_pcm_enqueue(struct snd_pcm_substream *substream)
{
	struct s5p_runtime_data *prtd = substream->runtime->private_data;
	dma_addr_t pos = prtd->dma_pos;
	int ret;

	s3cdbg("Entered %s\n", __FUNCTION__);

	while (prtd->dma_loaded < prtd->dma_limit) {
		unsigned long len = prtd->dma_period;

		s3cdbg("dma_loaded: %d\n",prtd->dma_loaded);

		if ((pos + len) > prtd->dma_end) {
			len  = prtd->dma_end - pos;
			s3cdbg(KERN_DEBUG "%s: corrected dma len %ld\n",
			       __FUNCTION__, len);
		}

		s3cdbg("enqing at %x, %lu bytes\n", pos, len);
		ret = s3c2410_dma_enqueue(prtd->params->channel, substream, pos, len);

		if (ret == 0) {
			prtd->dma_loaded++;
			pos += prtd->dma_period;
			if (pos >= prtd->dma_end)
				pos = prtd->dma_start;
		} else
			break;
	}

	prtd->dma_pos = pos;
}

static void s5p_audio_buffdone(struct s3c2410_dma_chan *channel,
				void *dev_id, int size,
				enum s3c2410_dma_buffresult result)
{
	struct snd_pcm_substream *substream = dev_id;
	struct s5p_runtime_data *prtd;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if (result == S3C2410_RES_ABORT || result == S3C2410_RES_ERR)
		return;
		
	if (!substream)
		return;

	prtd = substream->runtime->private_data;
	snd_pcm_period_elapsed(substream);

	spin_lock(&prtd->lock);
	if (prtd->state & ST_RUNNING) {
		prtd->dma_loaded--;
		s5p_pcm_enqueue(substream);
	}
	spin_unlock(&prtd->lock);
}

static void pcm_dmaupdate(void *id, int bytes_xfer)
{
	struct snd_pcm_substream *substream = id;
	struct s5p_runtime_data *prtd = substream->runtime->private_data;

	s3cdbg("%s:%d\n", __func__, __LINE__);

	if(prtd && (prtd->state & ST_RUNNING))
		snd_pcm_period_elapsed(substream); /* Only once for any num of periods while RUNNING */
}

static int s3c_pcm_hw_params_lp(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long totbytes = params_buffer_bytes(params);

	s3cdbg("Entered %s\n", __FUNCTION__);

	if(totbytes != MAX_LP_BUFF){
		s3cdbg("Use full buffer(128KB) in lowpower playback mode!");
		return -EINVAL;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = totbytes;

	/* We configure callback at partial playback complete acc to dutycyle selected */
	s3ci2s_func->dma_setcallbk(pcm_dmaupdate, LP_DMA_PERIOD);
	s3ci2s_func->dma_enqueue((void *)substream); /* Configure to loop the whole buffer */

	s3cdbg("DmaAddr=@%x Total=%lubytes PrdSz=%u #Prds=%u\n",
				runtime->dma_addr, totbytes, 
				params_period_bytes(params), runtime->hw.periods_min);

	return 0;
}

static int s5p_pcm_hw_params_nm(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s5p_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct s5p_pcm_dma_params *dma = rtd->dai->cpu_dai->dma_data;
	unsigned long totbytes = params_buffer_bytes(params);
	int ret=0;

	s3cdbg("Entered %s, params = %p \n", __FUNCTION__, prtd->params);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        totbytes = params_buffer_bytes(params) * ANDROID_BUF_NUM;

    else
        totbytes = params_buffer_bytes(params);

	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!dma)
		return 0;

	/* this may get called several times by oss emulation
	 * with different params */
	if (prtd->params == NULL) {
		prtd->params = dma;
		s3cdbg("params %p, client %p, channel %d\n", prtd->params,
			prtd->params->client, prtd->params->channel);

		/* prepare DMA */
		ret = s3c2410_dma_request(prtd->params->channel,
					  prtd->params->client, NULL);

		if (ret) {
			printk(KERN_ERR "failed to get dma channel\n");
			return ret;
		}
	} else if (prtd->params != dma) {

		s3c2410_dma_free(prtd->params->channel, prtd->params->client);

		prtd->params = dma;
		s3cdbg("params %p, client %p, channel %d\n", prtd->params,
			prtd->params->client, prtd->params->channel);

		/* prepare DMA */
		ret = s3c2410_dma_request(prtd->params->channel,
					  prtd->params->client, NULL);

		if (ret) {
			printk(KERN_ERR "failed to get dma channel\n");
			return ret;
		}
	}

	/* channel needs configuring for mem=>device, increment memory addr,
	 * sync to pclk, half-word transfers to the IIS-FIFO. */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		s3c2410_dma_devconfig(prtd->params->channel,
				S3C2410_DMASRC_MEM, 0,
				prtd->params->dma_addr);

		s3c2410_dma_config(prtd->params->channel,
				prtd->params->dma_size, 0);

	} else {
		s3c2410_dma_devconfig(prtd->params->channel,
				S3C2410_DMASRC_HW, 0,
				prtd->params->dma_addr);		

		s3c2410_dma_config(prtd->params->channel,
				prtd->params->dma_size, 0);
	}

	s3c2410_dma_set_buffdone_fn(prtd->params->channel,
				    s5p_audio_buffdone);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->dma_bytes = totbytes;

	spin_lock_irq(&prtd->lock);
	prtd->dma_loaded = 0;
	prtd->dma_limit = runtime->hw.periods_min;
	prtd->dma_period = params_period_bytes(params);
	prtd->dma_start = runtime->dma_addr;
	prtd->dma_pos = prtd->dma_start;
	prtd->dma_end = prtd->dma_start + totbytes;
	spin_unlock_irq(&prtd->lock);
	s3cdbg("DmaAddr=@%x Total=%lubytes PrdSz=%u #Prds=%u\n",
				runtime->dma_addr, totbytes, params_period_bytes(params), runtime->hw.periods_min);

	return 0;
}

static int s3c_pcm_hw_free_lp(struct snd_pcm_substream *substream)
{
	s3cdbg("Entered %s\n", __FUNCTION__);

	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int s5p_pcm_hw_free_nm(struct snd_pcm_substream *substream)
{
	struct s5p_runtime_data *prtd = substream->runtime->private_data;

	s3cdbg("Entered %s\n", __FUNCTION__);

	/* TODO - do we need to ensure DMA flushed */
	snd_pcm_set_runtime_buffer(substream, NULL);

	if (prtd->params) {
		s3c2410_dma_free(prtd->params->channel, prtd->params->client);
		prtd->params = NULL;
	}

	return 0;
}

static int s3c_pcm_prepare_lp(struct snd_pcm_substream *substream)
{
	s3cdbg("Entered %s\n", __FUNCTION__);

	/* flush the DMA channel */
	s3ci2s_func->dma_ctrl(S3C_I2SDMA_FLUSH);

	return 0;
}

static int s5p_pcm_prepare_nm(struct snd_pcm_substream *substream)
{
	struct s5p_runtime_data *prtd = substream->runtime->private_data;

	s3cdbg("Entered %s\n", __FUNCTION__);
#if !defined (CONFIG_CPU_S3C6400) && !defined (CONFIG_CPU_S3C6410) 
	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!prtd->params)
	 	return 0;
#endif

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ring_buf_index   = 0;
		period_index     = 0;
	}

	/* flush the DMA channel */
	s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_FLUSH);

	prtd->dma_loaded = 0;

	prtd->dma_pos = prtd->dma_start;

	/* enqueue dma buffers */
	s5p_pcm_enqueue(substream);

	return 0;
}

static int s5p_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct s5p_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	s3cdbg("Entered %s...trigger case=%d..\n", __FUNCTION__,cmd);

	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
#if 0
		if(s3c_pcm_pdat.lp_mode){
			prtd->state |= ST_RUNNING;
			s3ci2s_func->dma_ctrl(S3C_I2SDMA_RESUME);
			break;
		}
		s5p_pcm_enqueue(substream);
#endif
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->state |= ST_RUNNING;
		if(s3c_pcm_pdat.lp_mode)
		   s3ci2s_func->dma_ctrl(S3C_I2SDMA_START);
		else
		   s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_START);
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
#if 0
		if(s3c_pcm_pdat.lp_mode){
		   prtd->state &= ~ST_RUNNING;
		   s3ci2s_func->dma_ctrl(S3C_I2SDMA_SUSPEND);
		   break;
		}
		if(prtd->dma_loaded)
		   prtd->dma_loaded--; /* we may never get buffdone callback */
#endif
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->state &= ~ST_RUNNING;
		if(s3c_pcm_pdat.lp_mode)
		   s3ci2s_func->dma_ctrl(S3C_I2SDMA_STOP);
		else
		   s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_STOP);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);

	return ret;
}

static snd_pcm_uframes_t 
	s5p_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s5p_runtime_data *prtd = runtime->private_data;
	unsigned long res;
	dma_addr_t src, dst;

	spin_lock(&prtd->lock);

	if(s3c_pcm_pdat.lp_mode)
	   s3ci2s_func->dma_getpos(&src, &dst);
	else
	   s3c2410_dma_getposition(prtd->params->channel, &src, &dst);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		res = src - prtd->dma_start;
	else
		res = dst - prtd->dma_start;

	spin_unlock(&prtd->lock);

	s3cdbg("Pointer %x %x\n", src, dst);

	/* we seem to be getting the odd error from the pcm library due
	 * to out-of-bounds pointers. this is maybe due to the dma engine
	 * not having loaded the new values for the channel before being
	 * callled... (todo - fix )
	 */

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (res >= snd_pcm_lib_buffer_bytes(substream) * ANDROID_BUF_NUM) {
			if (res == snd_pcm_lib_buffer_bytes(substream) * ANDROID_BUF_NUM)
				res = 0;
		}
	} else {
		if (res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (res == snd_pcm_lib_buffer_bytes(substream))
				res = 0;
		}
	}

	return bytes_to_frames(substream->runtime, res);
}

static int s5p_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long size, offset;
	int ret;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if(s3c_pcm_pdat.lp_mode){
		/* From snd_pcm_lib_mmap_iomem */
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		vma->vm_flags |= VM_IO;
		size = vma->vm_end - vma->vm_start;
		offset = vma->vm_pgoff << PAGE_SHIFT;
		ret = io_remap_pfn_range(vma, vma->vm_start,
				(runtime->dma_addr + offset) >> PAGE_SHIFT,
				size, vma->vm_page_prot);
	}else{
		ret = dma_mmap_writecombine(substream->pcm->card->dev, vma,
						runtime->dma_area,
                                		runtime->dma_addr,
						runtime->dma_bytes);
	}

	return ret;
}

static int s5p_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s5p_runtime_data *prtd;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	   snd_soc_set_runtime_hwparams(substream, &s3c_pcm_pdat.pcm_hw_tx);
	else
	   snd_soc_set_runtime_hwparams(substream, &s3c_pcm_pdat.pcm_hw_rx);

	prtd = kzalloc(sizeof(struct s5p_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;

	return 0;
}

static int s5p_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s5p_runtime_data *prtd = runtime->private_data;

	s3cdbg("Entered %s, prtd = %p\n", __FUNCTION__, prtd);
	if (prtd)
		kfree(prtd);
	else
		printk("s5p_pcm_close called with prtd == NULL\n");

	return 0;
}

static struct snd_pcm_ops s5p_pcm_ops = {
	.open		= s5p_pcm_open,
	.close		= s5p_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	//.hw_params	= s5p_pcm_hw_params,
	//.hw_free	= s5p_pcm_hw_free,
	//.prepare	= s5p_pcm_prepare,
	.trigger	= s5p_pcm_trigger,
	.pointer	= s5p_pcm_pointer,
	.mmap		= s5p_pcm_mmap,
};

static int s5p_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size;
	unsigned char *vaddr;
	dma_addr_t paddr;

	s3cdbg("Entered %s\n", __FUNCTION__);

	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;

	if(stream == SNDRV_PCM_STREAM_PLAYBACK)
	   size = s3c_pcm_pdat.pcm_hw_tx.buffer_bytes_max;
	else
	   size = s3c_pcm_pdat.pcm_hw_rx.buffer_bytes_max;

	if(s3c_pcm_pdat.lp_mode){
		/* Map Tx/Rx buff from LP_Audio SRAM */
		paddr = s3c_pcm_pdat.lp_buffs.dma_addr[stream]; /* already assigned */
		vaddr = (unsigned char *)ioremap(paddr, size);

		s3c_pcm_pdat.lp_buffs.cpu_addr[stream] = vaddr;
		s3c_pcm_pdat.lp_buffs.dma_addr[stream] = paddr;

		/* Assign PCM buffer pointers */
		buf->dev.type = SNDRV_DMA_TYPE_CONTINUOUS;
		buf->area = s3c_pcm_pdat.lp_buffs.cpu_addr[stream];
		buf->addr = s3c_pcm_pdat.lp_buffs.dma_addr[stream];
	}else{
		vaddr = dma_alloc_writecombine(pcm->card->dev, size,
					   &paddr, GFP_KERNEL);
		if (!vaddr)
			return -ENOMEM;
		s3c_pcm_pdat.nm_buffs.cpu_addr[stream] = vaddr;
		s3c_pcm_pdat.nm_buffs.dma_addr[stream] = paddr;

		/* Assign PCM buffer pointers */
		buf->dev.type = SNDRV_DMA_TYPE_DEV;
		buf->area = s3c_pcm_pdat.nm_buffs.cpu_addr[stream];
		buf->addr = s3c_pcm_pdat.nm_buffs.dma_addr[stream];
	}

	printk("%s mode: preallocate buffer(%s):  VA-%p  PA-%X  %ubytes\n", 
			s3c_pcm_pdat.lp_mode ? "LowPower" : "Normal", 
			stream ? "Capture": "Playback", 
			vaddr, paddr, size);

	buf->bytes = size;
	return 0;
}

static void s5p_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	s3cdbg("Entered %s\n", __FUNCTION__);

	for (stream = SNDRV_PCM_STREAM_PLAYBACK; stream <= SNDRV_PCM_STREAM_CAPTURE; stream++) {

		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		if(s3c_pcm_pdat.lp_mode){
			iounmap(s3c_pcm_pdat.lp_buffs.cpu_addr[stream]);
			s3c_pcm_pdat.lp_buffs.cpu_addr[stream] = NULL;
		}else{
			dma_free_writecombine(pcm->card->dev, buf->bytes,
				      s3c_pcm_pdat.nm_buffs.cpu_addr[stream], s3c_pcm_pdat.nm_buffs.dma_addr[stream]);
			s3c_pcm_pdat.nm_buffs.cpu_addr[stream] = NULL;
			s3c_pcm_pdat.nm_buffs.dma_addr[stream] = 0;
		}

		buf->area = NULL;
		buf->addr = 0;
	}
}

static u64 s5p_pcm_dmamask = DMA_32BIT_MASK;

static int s5p_pcm_new(struct snd_card *card, 
	struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	int ret = 0;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &s5p_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	if (dai->playback.channels_min) {
		ret = s5p_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (dai->capture.channels_min) {
		ret = s5p_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
 out:
	return ret;
}

static void s3c_pcm_setmode(int lpmd, void *ptr)
{
	s3ci2s_func = (struct s5p_i2s_pdata *) ptr;
	s3c_pcm_pdat.lp_mode = lpmd;

	if(s3c_pcm_pdat.lp_mode){
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->hw_params = s3c_pcm_hw_params_lp;
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->hw_free = s3c_pcm_hw_free_lp;
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->prepare = s3c_pcm_prepare_lp;

		/* Configure Playback Channel */
		/* LP-Audio is not for playing clicks and tones.
		 * It is for lengthy audio playback where the user isn't 
		 * interacting much with the system. Here we can, and should, 
		 * preserve power at the cost of responsiveness.
		 * Period size should be as large as possible but small enough to give 
		 * enough time to system to wakeup-refillbuffer-sleep 
		 * without causing underruns.
		 * We use maximum possible period size while keeping 
		 * the least possible periods i.e, 1 period.
		 * The only assumptions LP mode make is that the application 
		 * use MAX_LP_BUFF size buffers _and_ LP_DMA_PERIOD > MAX_LP_BUFF/2.
		 */
		s3c_pcm_pdat.pcm_hw_tx.buffer_bytes_max = MAX_LP_BUFF;
		s3c_pcm_pdat.pcm_hw_tx.period_bytes_min = LP_DMA_PERIOD;
		s3c_pcm_pdat.pcm_hw_tx.period_bytes_max = LP_DMA_PERIOD;
		s3c_pcm_pdat.pcm_hw_tx.periods_min = 1;
		s3c_pcm_pdat.pcm_hw_tx.periods_max = 2;
		/* Configure Capture Channel */
		s3c_pcm_pdat.pcm_hw_rx.buffer_bytes_max = 0;
		s3c_pcm_pdat.pcm_hw_rx.channels_min = 0;
		s3c_pcm_pdat.pcm_hw_rx.channels_max = 0;
	}else{
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->hw_params = s5p_pcm_hw_params_nm;
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->hw_free = s5p_pcm_hw_free_nm;
		s3c_pcm_pdat.pcm_pltfm.pcm_ops->prepare = s5p_pcm_prepare_nm;

		/* Configure Playback Channel */
		s3c_pcm_pdat.pcm_hw_tx.buffer_bytes_max = MAX_LP_BUFF/2;
		s3c_pcm_pdat.pcm_hw_tx.period_bytes_min = PAGE_SIZE;
		s3c_pcm_pdat.pcm_hw_tx.period_bytes_max = PAGE_SIZE * 2;
		s3c_pcm_pdat.pcm_hw_tx.periods_min = 2;
		s3c_pcm_pdat.pcm_hw_tx.periods_max = 128;
		/* Configure Capture Channel */
		s3c_pcm_pdat.pcm_hw_rx.buffer_bytes_max = MAX_LP_BUFF;
		s3c_pcm_pdat.pcm_hw_rx.channels_min = 2;
		s3c_pcm_pdat.pcm_hw_rx.channels_max = 2;
	}
	s3cdbg("PrdsMn=%d PrdsMx=%d PrdSzMn=%d PrdSzMx=%d\n", 
					s3c_pcm_pdat.pcm_hw_tx.periods_min, s3c_pcm_pdat.pcm_hw_tx.periods_max,
					s3c_pcm_pdat.pcm_hw_tx.period_bytes_min, s3c_pcm_pdat.pcm_hw_tx.period_bytes_max);
}

struct s5p_pcm_pdata s3c_pcm_pdat = {
	.lp_mode   = 0,
	.lp_buffs = {
		.dma_addr[SNDRV_PCM_STREAM_PLAYBACK] = LP_TXBUFF_ADDR,
		//.dma_addr[SNDRV_PCM_STREAM_CAPTURE]  = LP_RXBUFF_ADDR,
	},
	.set_mode = s3c_pcm_setmode,
	.pcm_pltfm = {
		.name		= "s3c-audio",
		.pcm_ops 	= &s5p_pcm_ops,
		.pcm_new	= s5p_pcm_new,
		.pcm_free	= s5p_pcm_free_dma_buffers,
	},
	.pcm_hw_tx = {
		.info			= SNDRV_PCM_INFO_INTERLEAVED |
					    SNDRV_PCM_INFO_BLOCK_TRANSFER |
					    SNDRV_PCM_INFO_PAUSE |
					    //SNDRV_PCM_INFO_RESUME |
					    SNDRV_PCM_INFO_MMAP |
					    SNDRV_PCM_INFO_MMAP_VALID,
		.formats		= SNDRV_PCM_FMTBIT_S16_LE |
					    SNDRV_PCM_FMTBIT_U16_LE |
					    SNDRV_PCM_FMTBIT_U8 |
					    SNDRV_PCM_FMTBIT_S24_LE |
					    SNDRV_PCM_FMTBIT_S8,
		.channels_min		= 2,
		.channels_max		= 2,
		.period_bytes_min	= PAGE_SIZE,
		.fifo_size		= 64,
	},
	.pcm_hw_rx = {
		.info			= SNDRV_PCM_INFO_INTERLEAVED |
					    SNDRV_PCM_INFO_BLOCK_TRANSFER |
					    SNDRV_PCM_INFO_PAUSE |
					    //SNDRV_PCM_INFO_RESUME |
					    SNDRV_PCM_INFO_MMAP |
					    SNDRV_PCM_INFO_MMAP_VALID,
		.formats		= SNDRV_PCM_FMTBIT_S16_LE |
					    SNDRV_PCM_FMTBIT_U16_LE |
					    SNDRV_PCM_FMTBIT_U8 |
					    SNDRV_PCM_FMTBIT_S24_LE |
					    SNDRV_PCM_FMTBIT_S8,
		.period_bytes_min	= PAGE_SIZE,
		.period_bytes_max	= PAGE_SIZE*2,
		.periods_min		= 2,
		.periods_max		= 128,
		.fifo_size		= 64,
	},
};
EXPORT_SYMBOL_GPL(s3c_pcm_pdat);

static int __init s5p_soc_platform_init(void)
{
	return snd_soc_register_platform(&s3c_pcm_pdat.pcm_pltfm);
}
module_init(s5p_soc_platform_init);

static void __exit s5p_soc_platform_exit(void)
{
	snd_soc_unregister_platform(&s3c_pcm_pdat.pcm_pltfm);
}
module_exit(s5p_soc_platform_exit);

MODULE_AUTHOR("Ben Dooks, Jassi");
MODULE_DESCRIPTION("Samsung S5P PCM module");
MODULE_LICENSE("GPL");
