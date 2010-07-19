/*
 * spi_sam.c - Samsung SOC SPI controller driver.
 * By -- Jaswinder Singh <jassi.brar@samsung.com>
 *
 * Copyright (C) 2009 Samsung Electronics Ltd.
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <asm/dma.h>

#include "spi_sam.h"

//#define DEBUGSPI

#ifdef DEBUGSPI

#define dbg_printk(x...)	printk(x)

static void dump_regs(struct samspi_bus *sspi)
{
	u32 val;

	val = readl(sspi->regs + SAMSPI_CH_CFG);
	printk("CHN-%x\t", val);
	val = readl(sspi->regs + SAMSPI_CLK_CFG);
	printk("CLK-%x\t", val);
	val = readl(sspi->regs + SAMSPI_MODE_CFG);
	printk("MOD-%x\t", val);
	val = readl(sspi->regs + SAMSPI_SLAVE_SEL);
	printk("SLVSEL-%x\t", val);
	val = readl(sspi->regs + SAMSPI_SPI_STATUS);
	printk("STATUS-%x\t", val);	
	if(val & SPI_STUS_TX_DONE)
	   printk("TX_done\t");
	if(val & SPI_STUS_TRAILCNT_ZERO)
	   printk("TrailZ\t");
	if(val & SPI_STUS_RX_OVERRUN_ERR)
	   printk("RX_Ovrn\t");
	if(val & SPI_STUS_RX_UNDERRUN_ERR)
	   printk("Rx_Unrn\t");
	if(val & SPI_STUS_TX_OVERRUN_ERR)
	   printk("Tx_Ovrn\t");
	if(val & SPI_STUS_TX_UNDERRUN_ERR)
	   printk("Tx_Unrn\t");
	if(val & SPI_STUS_RX_FIFORDY)
	   printk("Rx_Rdy\t");
	if(val & SPI_STUS_TX_FIFORDY)
	   printk("Tx_Rdy\t");
	printk("Rx/TxLvl=%d,%d\t", (val>>13)&0x7f, (val>>6)&0x7f);

	val = readl(sspi->regs + SAMSPI_PACKET_CNT);
	if(val)  printk("PACKET_CNT-%x\t", val);
	val = readl(sspi->regs + SAMSPI_SWAP_CFG);
	if(val) printk("SWAP_CFG-%x\t", val);	
	val = readl(sspi->regs + SAMSPI_FB_CLK);
	if(val) printk("FB_CLK-%x\n", val);	

	printk("\n");	
}
#else

#define dump_regs(sspi) 	do{}while(0)
#define dbg_printk(x...)	do{}while(0)

#endif

static void dump_spi_regs(struct samspi_bus *sspi)
{
	u32 val;
	struct platform_device *pdev = sspi->pdev;

	printk("\n");
	val = readl(sspi->regs + SAMSPI_CH_CFG);
	printk("CHN%1d-%x\t", pdev->id, val);
	val = readl(sspi->regs + SAMSPI_CLK_CFG);
	printk("CLK%1d-%x\t", pdev->id, val);
	val = readl(sspi->regs + SAMSPI_MODE_CFG);
	printk("MOD%1d-%x\t", pdev->id, val);
	val = readl(sspi->regs + SAMSPI_SLAVE_SEL);
	printk("SLVSEL%1d-%x\t", pdev->id, val);
	val = readl(sspi->regs + SAMSPI_SPI_STATUS);
	printk("STATUS%1d-%x\t", pdev->id, val);	
	if(val & SPI_STUS_TX_DONE)
	   printk("TX_done\t");
	if(val & SPI_STUS_TRAILCNT_ZERO)
	   printk("TrailZ\t");
	if(val & SPI_STUS_RX_OVERRUN_ERR)
	   printk("RX_Ovrn\t");
	if(val & SPI_STUS_RX_UNDERRUN_ERR)
	   printk("Rx_Unrn\t");
	if(val & SPI_STUS_TX_OVERRUN_ERR)
	   printk("Tx_Ovrn\t");
	if(val & SPI_STUS_TX_UNDERRUN_ERR)
	   printk("Tx_Unrn\t");
	if(val & SPI_STUS_RX_FIFORDY)
	   printk("Rx_Rdy\t");
	if(val & SPI_STUS_TX_FIFORDY)
	   printk("Tx_Rdy\t");
	printk("Rx/TxLvl=%d,%d\t", (val>>13)&0x7f, (val>>6)&0x7f);

	val = readl(sspi->regs + SAMSPI_PACKET_CNT);
	if(val)  printk("PACKET_CNT%1d-%x\t", pdev->id, val);
	val = readl(sspi->regs + SAMSPI_SWAP_CFG);
	if(val) printk("SWAP_CFG%1d-%x\t", pdev->id, val);	
	val = readl(sspi->regs + SAMSPI_FB_CLK);
	if(val) printk("FB_CLK%1d-%x\n", pdev->id, val);	

	printk("\n");	
}

void samspi_dma_rxcb(struct s3c2410_dma_chan *chan, void *buf_id, int size, enum s3c2410_dma_buffresult res);
void samspi_dma_txcb(struct s3c2410_dma_chan *chan, void *buf_id, int size, enum s3c2410_dma_buffresult res);

static struct s3c2410_dma_client samspi_dma_client = {
	.name = "samspi-dma",
};

static inline void enable_spidma(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	u32 val;

	val = readl(sspi->regs + SAMSPI_MODE_CFG);
	val &= ~(SPI_MODE_TXDMA_ON | SPI_MODE_RXDMA_ON);
	if(xfer->tx_buf != NULL)
	   val |= SPI_MODE_TXDMA_ON;
	if(xfer->rx_buf != NULL)
	   val |= SPI_MODE_RXDMA_ON;
	writel(val, sspi->regs + SAMSPI_MODE_CFG);
}

static inline void flush_dma(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	if(xfer->tx_buf != NULL)
	   s3c2410_dma_ctrl(sspi->tx_dmach, S3C2410_DMAOP_FLUSH);
	if(xfer->rx_buf != NULL)
	   s3c2410_dma_ctrl(sspi->rx_dmach, S3C2410_DMAOP_FLUSH);
}

static inline void flush_spi(struct samspi_bus *sspi)
{
	u32 val;

	val = readl(sspi->regs + SAMSPI_CH_CFG);
	val |= SPI_CH_SW_RST;

/*
	val &= ~SPI_CH_HS_EN;
	if((sspi->cur_speed > 30000000UL) && !(sspi->cur_mode & SPI_SLAVE))
	   val |= SPI_CH_HS_EN;
*/	
	writel(val, sspi->regs + SAMSPI_CH_CFG);

	/* Flush TxFIFO*/
	do{
	   val = readl(sspi->regs + SAMSPI_SPI_STATUS);
	   val = (val>>6) & 0x7f;
	}while(val);

	/* Flush RxFIFO*/
	val = readl(sspi->regs + SAMSPI_SPI_STATUS);
	val = (val>>13) & 0x7f;
	while(val){
	   readl(sspi->regs + SAMSPI_SPI_RX_DATA);
	   val = readl(sspi->regs + SAMSPI_SPI_STATUS);
	   val = (val>>13) & 0x7f;
	}

	val = readl(sspi->regs + SAMSPI_CH_CFG);
	val &= ~SPI_CH_SW_RST;
	writel(val, sspi->regs + SAMSPI_CH_CFG);
}

static inline void enable_spichan(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	u32 val;

	val = readl(sspi->regs + SAMSPI_CH_CFG);
	val &= ~(SPI_CH_RXCH_ON | SPI_CH_TXCH_ON | SPI_CH_HS_EN );

	if(!(sspi->cur_mode & SPI_SLAVE)){
		val |= SPI_CH_HS_EN;	
	}

	writel(0x0, sspi->regs + SAMSPI_PACKET_CNT);
	
	if(xfer->tx_buf != NULL){
	   val |= SPI_CH_TXCH_ON;
	}
	if(xfer->rx_buf != NULL){
	   if(!(sspi->cur_mode & SPI_SLAVE)){
	      flush_spi(sspi);
	      writel((xfer->len & 0xffff) | SPI_PACKET_CNT_EN, 
			sspi->regs + SAMSPI_PACKET_CNT); /* XXX TODO Bytes or number of SPI-Words? */
	   }
	   val |= SPI_CH_RXCH_ON;
	}
	writel(val, sspi->regs + SAMSPI_CH_CFG);
}

static inline void enable_spiintr(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	u32 val = 0;

       writel(0x1f, sspi->regs + SAMSPI_PENDING_CLR);
	   
	if(xfer->tx_buf != NULL){
	   val |= SPI_INT_TX_OVERRUN_EN;
	   if(!(sspi->cur_mode & SPI_SLAVE))
	      val |= SPI_INT_TX_UNDERRUN_EN;
	}
	if(xfer->rx_buf != NULL){
	   val |= (SPI_INT_RX_UNDERRUN_EN | SPI_INT_RX_OVERRUN_EN | SPI_INT_TRAILING_EN);
	}
	writel(val, sspi->regs + SAMSPI_SPI_INT_EN);
}

static inline void enable_spienqueue(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	if(xfer->rx_buf != NULL){
	   sspi->rx_done = BUSY;
	   s3c2410_dma_config(sspi->rx_dmach, sspi->cur_bpw/8, 0);
	   s3c2410_dma_enqueue(sspi->rx_dmach, (void *)sspi, xfer->rx_dma, xfer->len);
	}
	if(xfer->tx_buf != NULL){
	   sspi->tx_done = BUSY;
	   s3c2410_dma_config(sspi->tx_dmach, sspi->cur_bpw/8, 0);
	   s3c2410_dma_enqueue(sspi->tx_dmach, (void *)sspi, xfer->tx_dma, xfer->len);
	}
}

static inline void enable_spics(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	u32 val = 0;

	val = readl(sspi->regs + SAMSPI_SLAVE_SEL);
	if( !(sspi->cur_mode & SPI_SLAVE) ){  // if master  mode
	   val &= ~SPI_SLAVE_AUTO; 		/* Manual Mode */
	   val &= ~SPI_SLAVE_SIG_INACT; 	/* Activate CS */
	}
#if 0	
	if(sspi->cur_mode & SPI_SLAVE){
	  val |= SPI_SLAVE_AUTO; /* Auto Mode */
	  val |= SPI_SLAVE_SIG_INACT;
	}else{
	   val &= ~SPI_SLAVE_AUTO; /* Manual Mode */
	   val &= ~SPI_SLAVE_SIG_INACT; /* Activate CS */
	}
#endif
	writel(val, sspi->regs + SAMSPI_SLAVE_SEL);
}

static inline void set_polarity(struct samspi_bus *sspi)
{
	u32 val;

	val = readl(sspi->regs + SAMSPI_CH_CFG);
	val &= ~(SPI_CH_SLAVE | SPI_CPOL_L | SPI_CPHA_B);
	if(sspi->cur_mode & SPI_SLAVE)
	   val |= SPI_CH_SLAVE;
	if(sspi->cur_mode & SPI_CPOL)
	   val |= SPI_CPOL_L;
	if(sspi->cur_mode & SPI_CPHA)
	   val |= SPI_CPHA_B;
	writel(val, sspi->regs + SAMSPI_CH_CFG);
}

static inline void set_clock(struct samspi_bus *sspi)
{
	u32 val;

	val = readl(sspi->regs + SAMSPI_CLK_CFG);
	val &= ~(SPI_CLKSEL_SRCMSK | SPI_ENCLK_ENABLE | 0xff);
	val |= SPI_CLKSEL_SRC;
	if(!(sspi->cur_mode & SPI_SLAVE)){
	   val |= ((clk_get_rate(sspi->clk) / sspi->cur_speed / 2 - 1) << 0);	// PCLK and PSR
	   val |= SPI_ENCLK_ENABLE;
	}
	writel(val, sspi->regs + SAMSPI_CLK_CFG);
}

static inline void set_dmachan(struct samspi_bus *sspi)
{
	u32 val;

	val = readl(sspi->regs + SAMSPI_MODE_CFG);
	val &= ~((0x3<<17) | (0x3<<29));
	if(sspi->cur_bpw == 8){
	   val |= SPI_MODE_CH_TSZ_BYTE;
	   val |= SPI_MODE_BUS_TSZ_BYTE;
	}else if(sspi->cur_bpw == 16){
	   val |= SPI_MODE_CH_TSZ_HALFWORD;
	   val |= SPI_MODE_BUS_TSZ_HALFWORD;
	}else if(sspi->cur_bpw == 32){
	   val |= SPI_MODE_CH_TSZ_WORD;
	   val |= SPI_MODE_BUS_TSZ_WORD;
	}else{
	   printk("Invalid Bits/Word!\n");
	}
	val &= ~(SPI_MODE_4BURST | SPI_MODE_TXDMA_ON | SPI_MODE_RXDMA_ON);
	writel(val, sspi->regs + SAMSPI_MODE_CFG);
}

static inline void set_cs(struct samspi_bus *sspi)
{
	u32 val;

	val = readl(sspi->regs + SAMSPI_SLAVE_SEL);
	val &=  ~((0x30&0x3f)<<4); 

	  //val |= SPI_SLAVE_AUTO; 			/* Auto Mode */
	val &= ~SPI_SLAVE_AUTO; 			/* Manual Mode */
	val |= SPI_SLAVE_SIG_INACT;
	
	if( !(sspi->cur_mode & SPI_SLAVE) ){
		val |= (0x15&0x3f)<<4; 			// NSSOUT inactive time;	  
	}
	writel(val, sspi->regs + SAMSPI_SLAVE_SEL);

}


static void config_sspi(struct samspi_bus *sspi)
{
	/* Set Polarity and Phase */
	set_polarity(sspi);

	/* Configure Clock */
	set_clock(sspi);

	/* Set Channel & DMA Mode */
	set_dmachan(sspi);

	/* Set Slave selection signal */
	set_cs(sspi); 

}

static int samspi_dma_request(struct samspi_bus *sspi)
{
	int ret = -ENODEV;
	
	if(s3c2410_dma_request(sspi->rx_dmach, &samspi_dma_client, NULL)){
		printk(KERN_ERR "cannot get RxDMA\n");
		ret = -EBUSY;

		goto err1;
	}
	s3c2410_dma_set_buffdone_fn(sspi->rx_dmach, samspi_dma_rxcb);
	s3c2410_dma_devconfig(sspi->rx_dmach, S3C2410_DMASRC_HW, 0, sspi->sfr_phyaddr + SAMSPI_SPI_RX_DATA);
	s3c2410_dma_config(sspi->rx_dmach, sspi->cur_bpw/8, 0);
	s3c2410_dma_setflags(sspi->rx_dmach, S3C2410_DMAF_AUTOSTART);

	if(s3c2410_dma_request(sspi->tx_dmach, &samspi_dma_client, NULL)){
		printk(KERN_ERR "cannot get TxDMA\n");
		ret = -EBUSY;
		goto err2;
	}
	s3c2410_dma_set_buffdone_fn(sspi->tx_dmach, samspi_dma_txcb);
	s3c2410_dma_devconfig(sspi->tx_dmach, S3C2410_DMASRC_MEM, 0, sspi->sfr_phyaddr + SAMSPI_SPI_TX_DATA);
	s3c2410_dma_config(sspi->tx_dmach, sspi->cur_bpw/8, 0);
	s3c2410_dma_setflags(sspi->tx_dmach, S3C2410_DMAF_AUTOSTART);

	return 0;
	
err2:
	s3c2410_dma_free(sspi->rx_dmach, &samspi_dma_client);
	
err1:

	return ret;

}

static void samspi_dma_release(struct samspi_bus *sspi)
{
	s3c2410_dma_free(sspi->tx_dmach, &samspi_dma_client);
	s3c2410_dma_free(sspi->rx_dmach, &samspi_dma_client);
}

static void samspi_hwinit(struct samspi_bus *sspi, int channel)
{
	unsigned int val;

	writel(SPI_SLAVE_SIG_INACT, sspi->regs + SAMSPI_SLAVE_SEL);

	/* Disable Interrupts */
	writel(0, sspi->regs + SAMSPI_SPI_INT_EN);

#ifdef CONFIG_CPU_S3C6410
	writel((readl(S3C64XX_SPC_BASE) & ~(3<<28)) | (3<<28), S3C64XX_SPC_BASE);
	writel((readl(S3C64XX_SPC_BASE) & ~(3<<18)) | (3<<18), S3C64XX_SPC_BASE);
#elif defined (CONFIG_CPU_S5P6440)
	writel((readl(S5P64XX_SPC_BASE) & ~(3<<28)) | (3<<28), S5P64XX_SPC_BASE);
	writel((readl(S5P64XX_SPC_BASE) & ~(3<<18)) | (3<<18), S5P64XX_SPC_BASE);
#elif defined (CONFIG_CPU_S5P6440)
	/* How to control drive strength, if we must? */
#endif

	writel(SPI_CLKSEL_SRC, sspi->regs + SAMSPI_CLK_CFG);
	writel(0, sspi->regs + SAMSPI_MODE_CFG);
	writel(SPI_SLAVE_SIG_INACT, sspi->regs + SAMSPI_SLAVE_SEL);
	writel(0, sspi->regs + SAMSPI_PACKET_CNT);
	writel(readl(sspi->regs + SAMSPI_PENDING_CLR), sspi->regs + SAMSPI_PENDING_CLR);
	writel(SPI_FBCLK_0NS, sspi->regs + SAMSPI_FB_CLK);

	flush_spi(sspi);

	writel(0, sspi->regs + SAMSPI_SWAP_CFG);
	writel(SPI_FBCLK_9NS, sspi->regs + SAMSPI_FB_CLK);

	val = readl(sspi->regs + SAMSPI_MODE_CFG);
	val &= ~(SPI_MAX_TRAILCNT << SPI_TRAILCNT_OFF);
	if(channel == 0)
	   SET_MODECFG(val, 0);
	else 
	   SET_MODECFG(val, 1);
	val |= (SPI_TRAILCNT << SPI_TRAILCNT_OFF);
	writel(val, sspi->regs + SAMSPI_MODE_CFG);

	config_sspi(sspi);
}

static irqreturn_t samspi_interrupt(int irq, void *dev_id)
{
	u32 val;
	struct samspi_bus *sspi = (struct samspi_bus *)dev_id;

	dump_regs(sspi);
	val = readl(sspi->regs + SAMSPI_PENDING_CLR);
	dbg_printk("PENDING=%x\n", val);
	writel(val, sspi->regs + SAMSPI_PENDING_CLR);

	/* We get interrupted only for bad news */
	if(sspi->tx_done != PASS){
	   sspi->tx_done = FAIL;
	}
	if(sspi->rx_done != PASS){
	   sspi->rx_done = FAIL;
	}
	sspi->state = STOPPED;
	complete(&sspi->xfer_completion);

	return IRQ_HANDLED;
}

void samspi_dma_rxcb(struct s3c2410_dma_chan *chan, void *buf_id, int size, enum s3c2410_dma_buffresult res)
{
	struct samspi_bus *sspi = (struct samspi_bus *)buf_id;
	struct platform_device *pdev = sspi->pdev;

	if(res == S3C2410_RES_OK){
	   sspi->rx_done = PASS;
	   dbg_printk(KERN_INFO "DmaRx-%d \n", size);
	}else{
	   sspi->rx_done = FAIL;
	   dbg_printk(KERN_ERR "DmaAbrtRx-%d \n", size);

	  /* Setup Deufult Mode */
	  samspi_hwinit(sspi, pdev->id);
	  samspi_dma_release(sspi);
	  samspi_dma_request(sspi);
  	}

	if(sspi->tx_done != BUSY && sspi->state != STOPPED) /* If other done and all OK */
	   complete(&sspi->xfer_completion);
}

void samspi_dma_txcb(struct s3c2410_dma_chan *chan, void *buf_id, int size, enum s3c2410_dma_buffresult res)
{
	struct samspi_bus *sspi = (struct samspi_bus *)buf_id;
	struct platform_device *pdev = sspi->pdev;
	
	if(res == S3C2410_RES_OK){
	   sspi->tx_done = PASS;
	   dbg_printk(KERN_INFO "DmaTx-%d \n", size);
	}else{
	   sspi->tx_done = FAIL;
	   dbg_printk(KERN_ERR "DmaAbrtTx-%d \n", size);

	  /* Setup Deufult Mode */
	   samspi_hwinit(sspi, pdev->id);
	   samspi_dma_release(sspi);
	   samspi_dma_request(sspi);	   
	}

	if(sspi->rx_done != BUSY && sspi->state != STOPPED) /* If other done and all OK */
	   complete(&sspi->xfer_completion);
}

static int wait_for_txshiftout(struct samspi_bus *sspi, unsigned long t)
{
	unsigned long timeout;

	timeout = jiffies + t;
	while((__raw_readl(sspi->regs + SAMSPI_SPI_STATUS) >> 6) & 0x7f){
	   if(time_after(jiffies, timeout))
	      return -1;
	   cpu_relax();
	}
	return 0;
}

static int wait_for_xfer(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	int status;
	u32 val;

	val = msecs_to_jiffies(xfer->len / (sspi->min_speed / 8 / 1000)); /* time to xfer data at min. speed */
	if(sspi->cur_mode & SPI_SLAVE)
	   val += msecs_to_jiffies(5000); /* 5secs to switch on the Master */
	else
	   val += msecs_to_jiffies(10); /* just some more */
	status = wait_for_completion_interruptible_timeout(&sspi->xfer_completion, val);

	if(status == 0)
	   status = -ETIMEDOUT;
	else if(status == -ERESTARTSYS)
	   status = -EINTR;
	else if((sspi->tx_done != PASS) || (sspi->rx_done != PASS)) /* Some Xfer failed */
	   status = -EIO;
	else
	   status = 0;	/* All OK */

	/* When TxLen <= SPI-FifoLen in Slave mode, DMA returns naively */
	if(!status && (sspi->cur_mode & SPI_SLAVE) && (xfer->tx_buf != NULL)){
	   val = msecs_to_jiffies(xfer->len / (sspi->min_speed / 8 / 1000)); /* Be lenient */
	   val += msecs_to_jiffies(5000); /* 5secs to switch on the Master */
	   status = wait_for_txshiftout(sspi, val);
	   if(status == -1)
	      status = -ETIMEDOUT;
	   else
	      status = 0;
	}

	return status;
}

#define INVALID_DMA_ADDRESS	0xffffffff
/*  First, try to map buf onto phys addr as such.
 *   If xfer->r/tx_buf was not on contiguous memory,
 *   allocate from our preallocated DMA buffer.
 */
static int samspi_map_xfer(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	struct device *dev = &sspi->pdev->dev;

	sspi->rx_tmp = NULL;
	sspi->tx_tmp = NULL;

	xfer->tx_dma = xfer->rx_dma = INVALID_DMA_ADDRESS;
	if(xfer->tx_buf != NULL){
		xfer->tx_dma = dma_map_single(dev,
				(void *) xfer->tx_buf, xfer->len,
				DMA_TO_DEVICE);
		if(dma_mapping_error(dev, xfer->tx_dma))
			goto alloc_from_buffer;
	}
	if(xfer->rx_buf != NULL){
		xfer->rx_dma = dma_map_single(dev,
				xfer->rx_buf, xfer->len,
				DMA_FROM_DEVICE);
		if(dma_mapping_error(dev, xfer->rx_dma)){
			if(xfer->tx_buf)
				dma_unmap_single(dev,
						xfer->tx_dma, xfer->len,
						DMA_TO_DEVICE);
			goto alloc_from_buffer;
		}
	}
	return 0;

alloc_from_buffer: /* If the xfer->[r/t]x_buf was not on contiguous memory */

	if(xfer->len <= SAMSPI_DMABUF_LEN){
	   if(xfer->rx_buf != NULL){
	      xfer->rx_dma = sspi->rx_dma_phys;
	      sspi->rx_tmp = (void *)sspi->rx_dma_cpu;
	   }
	   if(xfer->tx_buf != NULL){
	      xfer->tx_dma = sspi->tx_dma_phys;
	      sspi->tx_tmp = (void *)sspi->tx_dma_cpu;
	   }
	}else{
	   dbg_printk("If you plan to use this Xfer size often, increase SAMSPI_DMABUF_LEN\n");
	   if(xfer->rx_buf != NULL){
	      sspi->rx_tmp = dma_alloc_coherent(&sspi->pdev->dev, SAMSPI_DMABUF_LEN, 
							&xfer->rx_dma, GFP_KERNEL | GFP_DMA);
		if(sspi->rx_tmp == NULL)
		   return -ENOMEM;
	   }
	   if(xfer->tx_buf != NULL){
	      sspi->tx_tmp = dma_alloc_coherent(&sspi->pdev->dev, 
						SAMSPI_DMABUF_LEN, &xfer->tx_dma, GFP_KERNEL | GFP_DMA);
		if(sspi->tx_tmp == NULL){
		   if(xfer->rx_buf != NULL)
		      dma_free_coherent(&sspi->pdev->dev, 
						SAMSPI_DMABUF_LEN, sspi->rx_tmp, xfer->rx_dma);
		   return -ENOMEM;
		}
	   }
	}

	if(xfer->tx_buf != NULL)
	   memcpy(sspi->tx_tmp, xfer->tx_buf, xfer->len);

	return 0;
}

static void samspi_unmap_xfer(struct samspi_bus *sspi, struct spi_transfer *xfer)
{
	if((sspi->rx_tmp == NULL) && (sspi->tx_tmp == NULL)) /* if map_single'd */
	   return;

	if(xfer->rx_buf != NULL)
	      memcpy(xfer->rx_buf, sspi->rx_tmp, xfer->len);

	if(xfer->len > SAMSPI_DMABUF_LEN){
	   if(xfer->rx_buf != NULL)
	      dma_free_coherent(&sspi->pdev->dev, SAMSPI_DMABUF_LEN, sspi->rx_tmp, xfer->rx_dma);
	   if(xfer->tx_buf != NULL)
	      dma_free_coherent(&sspi->pdev->dev, SAMSPI_DMABUF_LEN, sspi->tx_tmp, xfer->tx_dma);
	}else{
	   sspi->rx_tmp = NULL;
	   sspi->tx_tmp = NULL;
	}
}

static void samspi_handle_msg(struct samspi_bus *sspi, struct spi_message *msg)
{
	u8 bpw;
	u32 speed, val;
	int status = 0;
	struct spi_transfer *xfer;
	struct spi_device *spi = msg->spi;
	struct platform_device *pdev = sspi->pdev;

	
	dev_dbg(&spi->dev, "[%s() : %d  ] +++ ", __func__, __LINE__);

       samspi_hwinit(sspi, pdev->id);
     
	config_sspi(sspi);
	
	dump_regs(sspi);
	list_for_each_entry (xfer, &msg->transfers, transfer_list) {

		if(!msg->is_dma_mapped && samspi_map_xfer(sspi, xfer)){
		   dev_err(&spi->dev, "Xfer: Unable to allocate DMA buffer!\n");
		   status = -ENOMEM;
		   goto out;
		}

		INIT_COMPLETION(sspi->xfer_completion);

		/* Only BPW and Speed may change across transfers */
		bpw = xfer->bits_per_word ? : spi->bits_per_word;
		speed = xfer->speed_hz ? : spi->max_speed_hz;

		if(sspi->cur_bpw != bpw || sspi->cur_speed != speed){
			sspi->cur_bpw = bpw;
			sspi->cur_speed = speed;
			config_sspi(sspi);
		}

		/* Pending only which is to be done */
		sspi->rx_done = PASS;
		sspi->tx_done = PASS;
		sspi->state = RUNNING;

	    //dbg_printk("%s(): tx_dmach :%d ,  tx_dma : 0x%x, tx_buf :0x%x,  len : %d \n", __func__, sspi->tx_dmach, (u32)xfer->tx_buf, (u32)xfer->tx_dma, (u32)xfer->len);
		//dbg_printk("%s(): rx_dmach :%d ,  rx_dma : 0x%x, rx_buf :0x%x, len : %d \n", __func__, sspi->rx_dmach, (u32)xfer->rx_buf, (u32)xfer->rx_dma, (u32)xfer->len);

		/* Enable Interrupts */
		enable_spiintr(sspi, xfer);
		
		/* Enqueue data on DMA */
		enable_spienqueue(sspi, xfer);

		/* Enable DMA */
		enable_spidma(sspi, xfer);

		/* Enable TX/RX */
		enable_spichan(sspi, xfer);

		/* Slave Select */
		enable_spics(sspi, xfer);

		status = wait_for_xfer(sspi, xfer);

		/**************
		 * Block Here *
		 **************/

		if(status == -ETIMEDOUT){
		   dev_err(&spi->dev, "Xfer: Timeout!\n");
		   dump_spi_regs(sspi);	
		   sspi->state = STOPPED;
		   /* DMA Disable*/
		   val = readl(sspi->regs + SAMSPI_MODE_CFG);
		   val &= ~(SPI_MODE_TXDMA_ON | SPI_MODE_RXDMA_ON);
		   writel(val, sspi->regs + SAMSPI_MODE_CFG);
		   flush_dma(sspi, xfer);
		   flush_spi(sspi);
		   if(!msg->is_dma_mapped)
		      samspi_unmap_xfer(sspi, xfer);
		   goto out;
		}
		if(status == -EINTR){
		   dev_err(&spi->dev, "Xfer: Interrupted!\n");
		   dump_spi_regs(sspi);	
		   sspi->state = STOPPED;

		   /* DMA Disable*/
		   val = readl(sspi->regs + SAMSPI_MODE_CFG);
		   val &= ~(SPI_MODE_TXDMA_ON | SPI_MODE_RXDMA_ON);
		   writel(val, sspi->regs + SAMSPI_MODE_CFG);
		   flush_dma(sspi, xfer);
		   flush_spi(sspi);
		   if(!msg->is_dma_mapped)
		      samspi_unmap_xfer(sspi, xfer);
		   goto out;
		}
		if(status == -EIO){ /* Some Xfer failed */
		   dev_err(&spi->dev, "Xfer: Failed!\n");
		   dump_spi_regs(sspi);	
		   sspi->state = STOPPED;

		   /* DMA Disable*/
		   val = readl(sspi->regs + SAMSPI_MODE_CFG);
		   val &= ~(SPI_MODE_TXDMA_ON | SPI_MODE_RXDMA_ON);
		   writel(val, sspi->regs + SAMSPI_MODE_CFG);
		   flush_dma(sspi, xfer);
		   flush_spi(sspi);
		   if(!msg->is_dma_mapped)
		      samspi_unmap_xfer(sspi, xfer);
		   goto out;
		}

		dump_regs(sspi);
		msleep(1);

		if(xfer->delay_usecs){
		   udelay(xfer->delay_usecs);
		   dbg_printk("xfer-delay=%u\n", xfer->delay_usecs);
		}

		if(xfer->cs_change && !(sspi->cur_mode & SPI_SLAVE)){
		   writel(readl(sspi->regs + SAMSPI_SLAVE_SEL) | SPI_SLAVE_SIG_INACT,
				 sspi->regs + SAMSPI_SLAVE_SEL);
		   dbg_printk("xfer-cs_chng=%u\n", xfer->cs_change);
		}

		msg->actual_length += xfer->len;

		if(!msg->is_dma_mapped)
		   samspi_unmap_xfer(sspi, xfer);
	}

out:
	/* Slave Deselect */
	//val = readl(sspi->regs + SAMSPI_SLAVE_SEL);
	//val &= ~SPI_SLAVE_AUTO;
	//val |= SPI_SLAVE_SIG_INACT;
	val = SPI_SLAVE_SIG_INACT;
	writel(val, sspi->regs + SAMSPI_SLAVE_SEL);

	/* Disable Interrupts */
	writel(0, sspi->regs + SAMSPI_SPI_INT_EN);

	/* Tx/Rx Disable */
	val = readl(sspi->regs + SAMSPI_CH_CFG);
	val &= ~(SPI_CH_RXCH_ON | SPI_CH_TXCH_ON);
	writel(val, sspi->regs + SAMSPI_CH_CFG);

	/* DMA Disable*/
	val = readl(sspi->regs + SAMSPI_MODE_CFG);
	val &= ~(SPI_MODE_TXDMA_ON | SPI_MODE_RXDMA_ON);
	writel(val, sspi->regs + SAMSPI_MODE_CFG);

	msg->status = status;
	if(msg->complete)
	   msg->complete(msg->context);

	writel(0x1F, sspi->regs + SAMSPI_PENDING_CLR);
	dump_regs(sspi);


	dev_dbg(&spi->dev, "[%s() : %d  ] --- ", __func__, __LINE__);
}

static void samspi_work(struct work_struct *work)
{
	struct samspi_bus *sspi = container_of(work, struct samspi_bus, work);
	unsigned long flags;

	spin_lock_irqsave(&sspi->lock, flags);
	while (!list_empty(&sspi->queue)) {
		struct spi_message *msg;

		msg = container_of(sspi->queue.next, struct spi_message, queue);
		list_del_init(&msg->queue);
		spin_unlock_irqrestore(&sspi->lock, flags);

		samspi_handle_msg(sspi, msg);

		spin_lock_irqsave(&sspi->lock, flags);
	}
	spin_unlock_irqrestore(&sspi->lock, flags);
}

static void samspi_cleanup(struct spi_device *spi)
{
	dbg_printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static int samspi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_master *master = spi->master;
	struct samspi_bus *sspi = spi_master_get_devdata(master);
	unsigned long flags;

	spin_lock_irqsave(&sspi->lock, flags);
	msg->actual_length = 0;
	list_add_tail(&msg->queue, &sspi->queue);
	queue_work(sspi->workqueue, &sspi->work);

	spin_unlock_irqrestore(&sspi->lock, flags);

	return 0;
}

/* the spi->mode bits understood by this driver: */
#define MODEBITS	(SPI_CPOL | SPI_CPHA | SPI_SLAVE)

/*
 * Here we only check the validity of requested configuration and 
 * save the configuration in a local data-structure.
 * The controller is actually configured only just before
 * we get a message to transfer _and_ if no other message is pending(already configured).
 */
static int samspi_setup(struct spi_device *spi)
{
	unsigned long flags;
	unsigned int psr;
	struct samspi_bus *sspi = spi_master_get_devdata(spi->master);

	spin_lock_irqsave(&sspi->lock, flags);
	if(!list_empty(&sspi->queue)){	/* Any pending message? */
		spin_unlock_irqrestore(&sspi->lock, flags);
		dev_dbg(&spi->dev, "setup: attempt while messages in queue!\n");
		return -EBUSY;
	}
	spin_unlock_irqrestore(&sspi->lock, flags);

	if (spi->chip_select > spi->master->num_chipselect) {
		dev_dbg(&spi->dev, "setup: invalid chipselect %u (%u defined)\n",
				spi->chip_select, spi->master->num_chipselect);
		return -EINVAL;
	}

	spi->bits_per_word = spi->bits_per_word ? : 8;

	if((spi->bits_per_word != 8) && 
			(spi->bits_per_word != 16) && 
			(spi->bits_per_word != 32)){
		dev_err(&spi->dev, "setup: %dbits/wrd not supported!\n", spi->bits_per_word);
		return -EINVAL;
	}

	spi->max_speed_hz = spi->max_speed_hz ? : sspi->max_speed;

	/* Round-off max_speed_hz */
	psr = clk_get_rate(sspi->clk) / spi->max_speed_hz / 2 - 1;
	psr &= 0xff;
	if(spi->max_speed_hz < clk_get_rate(sspi->clk) / 2 / (psr + 1))
	   psr = (psr+1) & 0xff;

	spi->max_speed_hz = clk_get_rate(sspi->clk) / 2 / (psr + 1);

	if (spi->max_speed_hz > sspi->max_speed
			|| spi->max_speed_hz < sspi->min_speed){
		dev_err(&spi->dev, "setup: req speed(%u) out of range[%u-%u]\n", 
				spi->max_speed_hz, sspi->min_speed, sspi->max_speed);
		return -EINVAL;
	}

	if (spi->mode & ~MODEBITS) {
		dev_dbg(&spi->dev, "setup: unsupported mode bits %x\n",	spi->mode & ~MODEBITS);
		return -EINVAL;
	}

#ifdef DEBUGSPI_SETUP
	printk("@%s spi-%p sspi-%p\n", __func__, spi, sspi);
	printk("@%s spi-%p sspi-%p\n", __func__, spi, sspi);
	printk("Asked to setup mode:-");
	printk("max_speed_hz = %d\n", spi->max_speed_hz);
	printk("chip_select = %d\n", spi->chip_select);
	printk("bits_per_word = %d\n", spi->bits_per_word);
	printk("irq = %d\n", spi->irq);
	printk("Clk Phs = %d\n", (spi->mode & SPI_CPHA)? 1 : 0 );
	printk("Clk Pol = %d\n", (spi->mode & SPI_CPOL)? 1 : 0 );
	printk("ChipSelct = %s\n", (spi->mode & (1<<2)) ? "high" : "low" );
	printk("Mode = %s\n", (spi->mode & SPI_SLAVE) ? "Slave" : "Master");
#endif

	if((sspi->cur_bpw == spi->bits_per_word) && 
		(sspi->cur_speed == spi->max_speed_hz) && 
		(sspi->cur_mode == spi->mode)) /* If no change in configuration, do nothing */
	    return 0;

	sspi->cur_bpw = spi->bits_per_word;
	sspi->cur_speed = spi->max_speed_hz;
	sspi->cur_mode = spi->mode;

	return 0;
}

static int __init samspi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct samspi_bus *sspi;
	int ret = -ENODEV;

	dbg_printk("%s:%s:%d ID=%d\n", __FILE__, __func__, __LINE__, pdev->id);
	master = spi_alloc_master(&pdev->dev, sizeof(struct samspi_bus)); /* Allocate contiguous SPI controller */
	if (master == NULL)
		return ret;

	sspi = spi_master_get_devdata(master);
	sspi->pdev = pdev;
	sspi->master = master;
	platform_set_drvdata(pdev, master);

	INIT_WORK(&sspi->work, samspi_work);
	spin_lock_init(&sspi->lock);
	INIT_LIST_HEAD(&sspi->queue);
	init_completion(&sspi->xfer_completion);

	sspi->clk = clk_get(&pdev->dev, "spi");
	if (IS_ERR(sspi->clk)) {
		sspi->clk = NULL;
		dev_err(&pdev->dev, "cannot acquire clock \n");
		ret = -EBUSY;
		goto lb1;
	}
	ret = clk_enable(sspi->clk);
	if (ret) {
		clk_put(sspi->clk);
		sspi->clk = NULL;
		dev_err(&pdev->dev, "cannot enable clock \n");
		ret = -EBUSY;
		goto lb2;
	}

	sspi->cur_mode = SPI_SLAVE | SPI_MODE_2; /* Start in Slave mode */
	sspi->cur_bpw = 8;
	sspi->max_speed = clk_get_rate(sspi->clk) / 2 / (0x0 + 1);
	sspi->min_speed = clk_get_rate(sspi->clk) / 2 / (0xff + 1);

	/* Get and Map Resources */
	sspi->iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (sspi->iores == NULL) {
		dev_err(&pdev->dev, "cannot find IO resource\n");
		ret = -ENOENT;
		goto lb3;
	}

	sspi->ioarea = request_mem_region(sspi->iores->start, sspi->iores->end - sspi->iores->start + 1, pdev->name);
	if (sspi->ioarea == NULL) {
		dev_err(&pdev->dev, "cannot request IO\n");
		ret = -ENXIO;
		goto lb4;
	}

	sspi->regs = ioremap(sspi->iores->start, sspi->iores->end - sspi->iores->start + 1);
	if (sspi->regs == NULL) {
		dev_err(&pdev->dev, "cannot map IO\n");
		ret = -ENXIO;
		goto lb5;
	}

	sspi->tx_dma_cpu = dma_alloc_coherent(&pdev->dev, SAMSPI_DMABUF_LEN, &sspi->tx_dma_phys, GFP_KERNEL | GFP_DMA);
	if(sspi->tx_dma_cpu == NULL){
		dev_err(&pdev->dev, "Unable to allocate TX DMA buffers\n");
		ret = -ENOMEM;
		goto lb6;
	}

	sspi->rx_dma_cpu = dma_alloc_coherent(&pdev->dev, SAMSPI_DMABUF_LEN, &sspi->rx_dma_phys, GFP_KERNEL | GFP_DMA);
	if(sspi->rx_dma_cpu == NULL){
		dev_err(&pdev->dev, "Unable to allocate RX DMA buffers\n");
		ret = -ENOMEM;
		goto lb7;
	}

	sspi->irqres = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if(sspi->irqres == NULL){
		dev_err(&pdev->dev, "cannot find IRQ\n");
		ret = -ENOENT;
		goto lb8;
	}

	ret = request_irq(sspi->irqres->start, samspi_interrupt, IRQF_DISABLED,
			pdev->name, sspi);
	if(ret){
		dev_err(&pdev->dev, "cannot acquire IRQ\n");
		ret = -EBUSY;
		goto lb9;
	}

	sspi->workqueue = create_singlethread_workqueue(master->dev.parent->bus_id);
	if(!sspi->workqueue){
		dev_err(&pdev->dev, "cannot create workqueue\n");
		ret = -EBUSY;
		goto lb10;
	}

	master->bus_num = pdev->id;
	master->setup = samspi_setup;
	master->transfer = samspi_transfer;
	master->cleanup = samspi_cleanup;
	master->num_chipselect = 1; /* Only 1 Slave connected on SMDK */

	if(spi_register_master(master)){
		dev_err(&pdev->dev, "cannot register SPI master\n");
		ret = -EBUSY;
		goto lb11;
	}

	/* Configure GPIOs */
	if(pdev->id == 0)
		SETUP_SPI(sspi, 0);
	else if(pdev->id == 1)
		SETUP_SPI(sspi, 1);

	if(s3c2410_dma_request(sspi->rx_dmach, &samspi_dma_client, NULL)){
		dev_err(&pdev->dev, "cannot get RxDMA\n");
		ret = -EBUSY;
		goto lb12;
	}
	s3c2410_dma_set_buffdone_fn(sspi->rx_dmach, samspi_dma_rxcb);
	s3c2410_dma_devconfig(sspi->rx_dmach, S3C2410_DMASRC_HW, 0, sspi->sfr_phyaddr + SAMSPI_SPI_RX_DATA);
	s3c2410_dma_config(sspi->rx_dmach, sspi->cur_bpw/8, 0);
	s3c2410_dma_setflags(sspi->rx_dmach, S3C2410_DMAF_AUTOSTART);

	if(s3c2410_dma_request(sspi->tx_dmach, &samspi_dma_client, NULL)){
		dev_err(&pdev->dev, "cannot get TxDMA\n");
		ret = -EBUSY;
		goto lb13;
	}
	s3c2410_dma_set_buffdone_fn(sspi->tx_dmach, samspi_dma_txcb);
	s3c2410_dma_devconfig(sspi->tx_dmach, S3C2410_DMASRC_MEM, 0, sspi->sfr_phyaddr + SAMSPI_SPI_TX_DATA);
	s3c2410_dma_config(sspi->tx_dmach, sspi->cur_bpw/8, 0);
	s3c2410_dma_setflags(sspi->tx_dmach, S3C2410_DMAF_AUTOSTART);

	/* Setup Deufult Mode */
	samspi_hwinit(sspi, pdev->id);

	printk("Samsung SoC SPI Driver loaded for Bus SPI-%d with %d Slaves attached\n", pdev->id, master->num_chipselect);
	printk("\tMax,Min-Speed [%d, %d]Hz\n", sspi->max_speed, sspi->min_speed);
	printk("\tIrq=%d\tIOmem=[0x%x-0x%x]\tDMA=[Rx-%d, Tx-%d]\n",
			sspi->irqres->start,
			sspi->iores->end, sspi->iores->start,
			sspi->rx_dmach, sspi->tx_dmach);
	return 0;

lb13:
	s3c2410_dma_free(sspi->rx_dmach, &samspi_dma_client);
lb12:
	spi_unregister_master(master);
lb11:
	destroy_workqueue(sspi->workqueue);
lb10:
	free_irq(sspi->irqres->start, sspi);
lb9:
lb8:
	dma_free_coherent(&pdev->dev, SAMSPI_DMABUF_LEN, sspi->rx_dma_cpu, sspi->rx_dma_phys);
lb7:
	dma_free_coherent(&pdev->dev, SAMSPI_DMABUF_LEN, sspi->tx_dma_cpu, sspi->tx_dma_phys);
lb6:
	iounmap((void *) sspi->regs);
lb5:
	release_mem_region(sspi->iores->start, sspi->iores->end - sspi->iores->start + 1);
lb4:
lb3:
	clk_disable(sspi->clk);
lb2:
	clk_put(sspi->clk);
lb1:
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return ret;
}

static int __exit samspi_remove(struct platform_device *pdev)
{
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct samspi_bus *sspi = spi_master_get_devdata(master);

	s3c2410_dma_free(sspi->tx_dmach, &samspi_dma_client);
	s3c2410_dma_free(sspi->rx_dmach, &samspi_dma_client);
	spi_unregister_master(master);
	destroy_workqueue(sspi->workqueue);
	free_irq(sspi->irqres->start, sspi);
	dma_free_coherent(&pdev->dev, SAMSPI_DMABUF_LEN, sspi->rx_dma_cpu, sspi->rx_dma_phys);
	dma_free_coherent(&pdev->dev, SAMSPI_DMABUF_LEN, sspi->tx_dma_cpu, sspi->tx_dma_phys);
	iounmap((void *) sspi->regs);
	release_mem_region(sspi->iores->start, sspi->iores->end - sspi->iores->start + 1);
	clk_disable(sspi->clk);
	clk_put(sspi->clk);
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return 0;
}

static struct platform_driver sam_spi_driver = {
	.driver = {
		.name	= "sam-spi",
		.owner = THIS_MODULE,
		.bus    = &platform_bus_type,
	},
//	.remove = sam_spi_remove,
//	.shutdown = sam_spi_shutdown,
//	.suspend = sam_spi_suspend,
//	.resume = sam_spi_resume,
};

static int __init sam_spi_init(void)
{
	dbg_printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);
	return platform_driver_probe(&sam_spi_driver, samspi_probe);
}
module_init(sam_spi_init);

static void __exit sam_spi_exit(void)
{
	platform_driver_unregister(&sam_spi_driver);
}
module_exit(sam_spi_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jaswinder Singh Brar <jassi.brar@samsung.com>");
MODULE_DESCRIPTION("Samsung SOC SPI Controller");
MODULE_ALIAS("platform:sam-spi");
