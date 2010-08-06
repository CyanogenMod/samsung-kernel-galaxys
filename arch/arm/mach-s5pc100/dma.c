/* linux/arch/arm/mach-s5pc100/dma.c
 *
 * Copyright (c) 2003-2008,2009 Samsung Electronics
 *
 * S5PC100 DMA selection
 *
 * http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>

#include <mach/dma.h>
#include <asm/io.h>

#include <plat/dma.h>
#include <plat/cpu.h>

/* M2M DMAC */
#ifndef DMA_CTRLCH
#define MAP0(x) { \
		[0]	= (x) | DMA_CH_VALID,	\
		[1]	= (x) | DMA_CH_VALID,	\
		[2]	= (x) | DMA_CH_VALID,	\
		[3]	= (x) | DMA_CH_VALID,	\
		[4]	= (x) | DMA_CH_VALID,	\
		[5]     = (x) | DMA_CH_VALID,	\
		[6]	= (x) | DMA_CH_VALID,	\
		[7]     = (x) | DMA_CH_VALID,	\
		[8]	= (x),	\
		[9]	= (x),	\
		[10]	= (x),	\
		[11]	= (x),	\
		[12]	= (x),	\
		[13]    = (x),	\
		[14]	= (x),	\
		[15]    = (x),	\
		[16]	= (x),	\
		[17]	= (x),	\
		[18]	= (x),	\
		[19]	= (x),	\
		[20]	= (x),	\
		[21]    = (x),	\
		[22]	= (x),	\
		[23]    = (x),	\
	}
#else
#define MAP0(x) { \
		[0]	= (x) | DMA_CH_VALID,	\
		[1]	= (x) | DMA_CH_VALID,	\
		[2]	= (x) | DMA_CH_VALID,	\
		[3]	= (x) | DMA_CH_VALID,	\
		[4]	= (x) | DMA_CH_VALID,	\
		[5]     = (x) | DMA_CH_VALID,	\
		[6]	= (x) | DMA_CH_VALID,	\
		[7]     = (x),	\
		[8]	= (x),	\
		[9]	= (x),	\
		[10]	= (x),	\
		[11]	= (x),	\
		[12]	= (x),	\
		[13]    = (x),	\
		[14]	= (x),	\
		[15]    = (x),	\
		[16]	= (x),	\
		[17]	= (x),	\
		[18]	= (x),	\
		[19]	= (x),	\
		[20]	= (x),	\
		[21]    = (x),	\
		[22]	= (x),	\
		[23]    = (x),	\
	}
#endif

/* Peri-DMAC 0 */
#define MAP1(x) { \
		[0]	= (x),	\
		[1]	= (x),	\
		[2]	= (x),	\
		[3]	= (x),	\
		[4]	= (x),	\
		[5]     = (x),	\
		[6]	= (x),	\
		[7]     = (x),	\
		[8]	= (x) | DMA_CH_VALID,	\
		[9]	= (x) | DMA_CH_VALID,	\
		[10]	= (x) | DMA_CH_VALID,	\
		[11]	= (x) | DMA_CH_VALID,	\
		[12]	= (x) | DMA_CH_VALID,	\
		[13]    = (x) | DMA_CH_VALID,	\
		[14]	= (x) | DMA_CH_VALID,	\
		[15]    = (x) | DMA_CH_VALID,	\
		[16]	= (x),	\
		[17]	= (x),	\
		[18]	= (x),	\
		[19]	= (x),	\
		[20]	= (x),	\
		[21]    = (x),	\
		[22]	= (x),	\
		[23]    = (x),	\
	}

/* Peri-DMAC 1 */
#define MAP2(x) { \
		[0]	= (x),	\
		[1]	= (x),	\
		[2]	= (x),	\
		[3]	= (x),	\
		[4]	= (x),	\
		[5]     = (x),	\
		[6]	= (x),	\
		[7]     = (x),	\
		[8]	= (x),	\
		[9]	= (x),	\
		[10]	= (x),	\
		[11]	= (x),	\
		[12]	= (x),	\
		[13]    = (x),	\
		[14]	= (x),	\
		[15]    = (x),	\
		[16]	= (x) | DMA_CH_VALID,	\
		[17]	= (x) | DMA_CH_VALID,	\
		[18]	= (x) | DMA_CH_VALID,	\
		[19]	= (x) | DMA_CH_VALID,	\
		[20]	= (x) | DMA_CH_VALID,	\
		[21]    = (x) | DMA_CH_VALID,	\
		[22]	= (x) | DMA_CH_VALID,	\
		[23]    = (x) | DMA_CH_VALID,	\
	}


/* DMA request sources of Peri-DMAC 1 */
#define S3C_PDMA1_UART0CH0	0
#define S3C_PDMA1_UART0CH1	1
#define S3C_PDMA1_UART1CH0	2
#define S3C_PDMA1_UART1CH1	3
#define S3C_PDMA1_UART2CH0	4
#define S3C_PDMA1_UART2CH1	5
#define S3C_PDMA1_UART3CH0	6
#define S3C_PDMA1_UART3CH1	7
#define S3C_PDMA1_IRDA		8	
#define S3C_PDMA1_I2S0_RX	9
#define S3C_PDMA1_I2S0_TX	10
#define S3C_PDMA1_I2S0S_TX	11
#define S3C_PDMA1_I2S1_RX	12
#define S3C_PDMA1_I2S1_TX	13
#define S3C_PDMA1_I2S2_RX	14
#define S3C_PDMA1_I2S2_TX	15
#define S3C_PDMA1_SPI0_RX	16
#define S3C_PDMA1_SPI0_TX	17
#define S3C_PDMA1_SPI1_RX	18
#define S3C_PDMA1_SPI1_TX	19
#define S3C_PDMA1_SPI2_RX	20
#define S3C_PDMA1_SPI2_TX	21
#define S3C_PDMA1_PCM0_RX	22
#define S3C_PDMA1_PCM0_TX	23
#define S3C_PDMA1_PCM1_RX	24
#define S3C_PDMA1_PCM1_TX	25
#define S3C_PDMA1_MSM_REQ0	26
#define S3C_PDMA1_MSM_REQ1	27
#define S3C_PDMA1_MSM_REQ2	28
#define S3C_PDMA1_MSM_REQ3	29
#define S3C_PDMA1_CG		30
#define S3C_PDMA1_Reserved	31


/* DMA request sources of Peri-DMAC 0 */
#define S3C_PDMA0_UART0CH0	0
#define S3C_PDMA0_UART0CH1	1
#define S3C_PDMA0_UART1CH0	2
#define S3C_PDMA0_UART1CH1	3
#define S3C_PDMA0_UART2CH0	4
#define S3C_PDMA0_UART2CH1	5
#define S3C_PDMA0_UART3CH0	6
#define S3C_PDMA0_UART3CH1	7
#define S3C_PDMA0_IRDA		8	
#define S3C_PDMA0_I2S0_RX	9
#define S3C_PDMA0_I2S0_TX	10
#define S3C_PDMA0_I2S0S_TX	11
#define S3C_PDMA0_I2S1_RX	12
#define S3C_PDMA0_I2S1_TX	13
#define S3C_PDMA0_I2S2_RX	14
#define S3C_PDMA0_I2S2_TX	15
#define S3C_PDMA0_SPI0_RX	16
#define S3C_PDMA0_SPI0_TX	17
#define S3C_PDMA0_SPI1_RX	18
#define S3C_PDMA0_SPI1_TX	19
#define S3C_PDMA0_SPI2_RX	20
#define S3C_PDMA0_SPI2_TX	21
#define S3C_PDMA0_AC_MICIN	22
#define S3C_PDMA0_AC_PCMIN	23
#define S3C_PDMA0_AC_PCMOUT	24
#define S3C_PDMA0_EXTERNAL	25
#define S3C_PDMA0_PWM		26
#define S3C_PDMA0_SPDIF		27
#define S3C_PDMA0_HSI_TX	28
#define S3C_PDMA0_HSI_RX	29
#define S3C_PDMA0_Reserved_1	30
#define S3C_PDMA0_Reserved	31


/* DMA request sources of M2M-DMAC */
#define S3C_DMA_SEC_TX		0
#define S3C_DMA_SEC_RX		1
#define S3C_DMA_M2M		2


static struct s3c_dma_map __initdata s5pc100_dma_mappings[] = {

    [DMACH_UART0] = {
        .name       = "s5p-uart-dma",
        .channels   = MAP1(S3C_PDMA0_UART0CH0),
        .hw_addr.to = S3C_PDMA0_UART0CH0,
    },
    [DMACH_UART0_SRC2] = {
        .name       = "s5p-uart-dma-rx",
        .channels   = MAP2(S3C_PDMA1_UART0CH1),
        .hw_addr.from = S3C_PDMA1_UART0CH1,
    },
	[DMACH_I2S_IN] = {
		.name		= "i2s0-in",	
		.channels	= MAP1(S3C_PDMA0_I2S0_RX),
		.hw_addr.from	= S3C_PDMA0_I2S0_RX,
	},
	[DMACH_I2S_OUT] = {
		.name		= "i2s0-out",
		.channels	= MAP1(S3C_PDMA0_I2S0_TX),
		.hw_addr.to	= S3C_PDMA0_I2S0_TX,
	},
	[DMACH_I2S1_IN] = {
		.name		= "i2s1-in",
		.channels	= MAP2(S3C_PDMA1_I2S1_RX),
		.hw_addr.from	= S3C_PDMA1_I2S1_RX,
	},
	[DMACH_I2S1_OUT] = {
		.name		= "i2s1-out",
		.channels	= MAP2(S3C_PDMA1_I2S1_TX),
		.hw_addr.to	= S3C_PDMA1_I2S1_TX,
	},
	[DMACH_SPI0_IN] = {
		.name		= "spi0-in",
		.channels	= MAP1(S3C_PDMA0_SPI0_RX),
		.hw_addr.from	= S3C_PDMA0_SPI0_RX,
	},
	[DMACH_SPI0_OUT] = {
		.name		= "spi0-out",
		.channels	= MAP1(S3C_PDMA0_SPI0_TX),
		.hw_addr.to	= S3C_PDMA0_SPI0_TX,
	},
	[DMACH_SPI1_IN] = {
		.name		= "spi1-in",
		.channels	= MAP2(S3C_PDMA1_SPI1_RX),
		.hw_addr.from	= S3C_PDMA1_SPI1_RX,
	},
	[DMACH_SPI1_OUT] = {
		.name		= "spi1-out",
		.channels	= MAP2(S3C_PDMA1_SPI1_TX),
		.hw_addr.to	= S3C_PDMA1_SPI1_TX,
	},
#if 0
	[DMACH_SPI2_IN] = {
		.name		= "spi2-in",
		.channels	= MAP2(S3C_PDMA1_SPI2_RX),
		.hw_addr.from	= S3C_PDMA1_SPI2_RX,
	},
	[DMACH_SPI2_OUT] = {
		.name		= "spi2-out",
		.channels	= MAP2(S3C_PDMA1_SPI2_TX),
		.hw_addr.to	= S3C_PDMA1_SPI2_TX,
	},
#endif
	[DMACH_AC97_PCM_OUT] = {
		.name		= "ac97-pcm-out",
		.channels	= MAP1(S3C_PDMA0_AC_PCMOUT),
		.hw_addr.to	= S3C_PDMA0_AC_PCMOUT,
	},
	[DMACH_AC97_PCM_IN] = {
		.name		= "ac97-pcm-in",
		.channels	= MAP1(S3C_PDMA0_AC_PCMIN),
		.hw_addr.from	= S3C_PDMA0_AC_PCMIN,
	},
	[DMACH_AC97_MIC_IN] = {
		.name		= "ac97-mic-in",
		.channels	= MAP1(S3C_PDMA0_AC_MICIN),
		.hw_addr.from	= S3C_PDMA0_AC_MICIN,
	},
	[DMACH_I2S_V40_IN] = {                                                           
		.name           = "i2s-v40-in",                                          
		.channels       = MAP1(S3C_PDMA0_HSI_RX),                                 
		.hw_addr.from   = S3C_PDMA0_HSI_RX,                                       
	},                                                                               
	[DMACH_I2S_V40_OUT] = {                                                          
		.name           = "i2s-v40-out",                                         
		.channels       = MAP1(S3C_PDMA0_HSI_TX),                                 
		.hw_addr.to     = S3C_PDMA0_HSI_TX,                                       
	},
	[DMACH_I2S_V50_OUT] = {                
		.name           = "i2s-v50-out",
		.channels       = MAP1(S3C_PDMA1_I2S0_TX),
		.hw_addr.to     = S3C_PDMA1_I2S0_TX,
	},
	[DMACH_I2S_V50_IN] = {                
		.name           = "i2s-v50-in",
		.channels       = MAP1(S3C_PDMA1_I2S0_RX),
		.hw_addr.from     = S3C_PDMA1_I2S0_RX,
	},
	[DMACH_ONENAND_IN] = {
		.name		= "onenand-in",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M0] = {
		.name		= "3D-M2M0",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M1] = {
		.name		= "3D-M2M1",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M2] = {
		.name		= "3D-M2M2",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M3] = {
		.name		= "3D-M2M3",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M4] = {
		.name		= "3D-M2M4",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M5] = {
		.name		= "3D-M2M5",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M6] = {
		.name		= "3D-M2M6",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M7] = {
		.name		= "3D-M2M7",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_SPDIF_OUT] = {           //add for spdif          
                .name           = "spdif-out",
                .channels       = MAP1(S3C_PDMA0_SPDIF),
                .hw_addr.to     = S3C_PDMA0_SPDIF,
        },

};

static void s5pc100_dma_select(struct s3c2410_dma_chan *chan,
			       struct s3c_dma_map *map)
{
	chan->map = map;
}

static struct s3c_dma_selection __initdata s5pc100_dma_sel = {
	.select		= s5pc100_dma_select,
	.dcon_mask	= 0,
	.map		= s5pc100_dma_mappings,
	.map_size	= ARRAY_SIZE(s5pc100_dma_mappings),
};

static int __init s5pc100_dma_add(struct sys_device *sysdev)
{
	s3c_dma_init(S3C_DMA_CHANNELS, IRQ_MDMA, 0x20);
	return s3c_dma_init_map(&s5pc100_dma_sel);
}

static struct sysdev_driver s5pc100_dma_driver = {
	.add	= s5pc100_dma_add,
};

static int __init s5pc100_dma_init(void)
{
	return sysdev_driver_register(&s5pc100_sysclass, &s5pc100_dma_driver);
}

arch_initcall(s5pc100_dma_init);


