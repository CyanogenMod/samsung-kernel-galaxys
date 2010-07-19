/* linux/arch/arm/plat-s5pc1xx/devs.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Base S5PC1XX resource and device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/devs.h>
#include <plat/adc.h>


/* SMC9115 LAN via ROM interface */

static struct resource s3c_smc911x_resources[] = {
      [0] = {
              .start  = S5PC1XX_PA_SMC9115,
              .end    = S5PC1XX_PA_SMC9115 + 0x1fffffff,
              .flags  = IORESOURCE_MEM,
      },
      [1] = {
              .start = IRQ_EINT10,
              .end   = IRQ_EINT10,
              .flags = IORESOURCE_IRQ,
        },
};

struct platform_device s3c_device_smc911x = {
      .name           = "smc911x",
      .id             =  -1,
      .num_resources  = ARRAY_SIZE(s3c_smc911x_resources),
      .resource       = s3c_smc911x_resources,
};

/* FIMV MFC interface */
static struct resource s3c_mfc_resources[] = {
	[0] = {
		.start  = S5PC1XX_PA_MFC,
		.end    = S5PC1XX_PA_MFC + S5PC1XX_SZ_MFC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_MFC,
		.end    = IRQ_MFC,
		.flags  = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_mfc = {
	.name           = "s3c-mfc",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s3c_mfc_resources),
	.resource       = s3c_mfc_resources,
};

/* LCD Controller */

static struct resource s3c_lcd_resource[] = {
	[0] = {
		.start = S5PC1XX_PA_LCD,
		.end   = S5PC1XX_PA_LCD + SZ_1M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_LCD1,
		.end   = IRQ_LCD3,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_lcd_dmamask = 0xffffffffUL;

struct platform_device s3c_device_lcd = {
	.name		  = "s3c-lcd",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_lcd_resource),
	.resource	  = s3c_lcd_resource,
	.dev              = {
		.dma_mask		= &s3c_device_lcd_dmamask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

/* FIMG-2D controller */
static struct resource s3c_g2d_resource[] = {
	[0] = {
		.start	= S5PC1XX_PA_G2D,
		.end	= S5PC1XX_PA_G2D + S5PC1XX_SZ_G2D - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_2D,
		.end	= IRQ_2D,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_g2d = {
	.name		= "s3c-g2d",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_g2d_resource),
	.resource	= s3c_g2d_resource
};
EXPORT_SYMBOL(s3c_device_g2d);

/* ADC */
static struct resource s3c_adc_resource[] = {
	[0] = {
		.start = S3C_PA_ADC,
		.end   = S3C_PA_ADC + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_PENDN,
		.end   = IRQ_PENDN,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_ADC,
		.end   = IRQ_ADC,
		.flags = IORESOURCE_IRQ,
	}

};

struct platform_device s3c_device_adc = {
	.name		  = "s3c-adc",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_adc_resource),
	.resource	  = s3c_adc_resource,
};

void __init s3c_adc_set_platdata(struct s3c_adc_mach_info *pd)
{
	struct s3c_adc_mach_info *npd;

	npd = kmalloc(sizeof(*npd), GFP_KERNEL);
	if (npd) {
		memcpy(npd, pd, sizeof(*npd));
		s3c_device_adc.dev.platform_data = npd;
	} else {
		printk(KERN_ERR "no memory for ADC platform data\n");
	}
}

/* WATCHDOG TIMER*/

static struct resource s3c_wdt_resource[] = {
        [0] = {
                .start = S3C_PA_WDT,
                .end   = S3C_PA_WDT + 0xff,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_WDT,
                .end   = IRQ_WDT,
                .flags = IORESOURCE_IRQ,
        },
};

struct platform_device s3c_device_wdt = {
        .name             = "s3c2410-wdt",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_wdt_resource),
        .resource         = s3c_wdt_resource,
};

EXPORT_SYMBOL(s3c_device_wdt);


/* NAND Controller */

static struct resource s3c_nand_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_NAND,
                .end   = S5PC1XX_PA_NAND + S5PC1XX_SZ_NAND - 1,
                .flags = IORESOURCE_MEM,
        }
};

struct platform_device s3c_device_nand = {
        .name             = "s3c-nand",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_nand_resource),
        .resource         = s3c_nand_resource,
};

EXPORT_SYMBOL(s3c_device_nand);

/* USB Host Controller */

static struct resource s3c_usb_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_USBHOST,
                .end   = S5PC1XX_PA_USBHOST + S5PC1XX_SZ_USBHOST - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_UHOST,
                .end   = IRQ_UHOST,
                .flags = IORESOURCE_IRQ,
        }
};

static u64 s3c_device_usb_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb = {
        .name             = "s3c2410-ohci",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_usb_resource),
        .resource         = s3c_usb_resource,
        .dev              = {
                .dma_mask = &s3c_device_usb_dmamask,
                .coherent_dma_mask = 0xffffffffUL
        }
};

EXPORT_SYMBOL(s3c_device_usb);

/* USB Device (Gadget)*/

static struct resource s3c_usbgadget_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_OTG,
                .end   = S5PC1XX_PA_OTG+S5PC1XX_SZ_OTG-1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_OTG,
                .end   = IRQ_OTG,
                .flags = IORESOURCE_IRQ,
        }
};

struct platform_device s3c_device_usbgadget = {
        .name             = "s3c-usbgadget",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_usbgadget_resource),
        .resource         = s3c_usbgadget_resource,
};

EXPORT_SYMBOL(s3c_device_usbgadget);

/* USB Device (OTG hcd)*/

static struct resource s3c_usb_otghcd_resource[] = {
	[0] = {
		.start = S3C_PA_OTG,
		.end   = S3C_PA_OTG + S3C_SZ_OTG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_OTG,
		.end   = IRQ_OTG,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_otghcd_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_otghcd = {
	.name		= "s3c_otghcd",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb_otghcd_resource),
	.resource	= s3c_usb_otghcd_resource,
        .dev              = {
                .dma_mask = &s3c_device_usb_otghcd_dmamask,
                .coherent_dma_mask = 0xffffffffUL
        }
};

EXPORT_SYMBOL(s3c_device_usb_otghcd);

/* RTC */

static struct resource s3c_rtc_resource[] = {
        [0] = {
                .start = S3C_PA_RTC,
                .end   = S3C_PA_RTC + 0xff,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_RTC_ALARM,
                .end   = IRQ_RTC_ALARM,
                .flags = IORESOURCE_IRQ,
        },
        [2] = {
                .start = IRQ_RTC_TIC,
                .end   = IRQ_RTC_TIC,
                .flags = IORESOURCE_IRQ
        }
};

struct platform_device s3c_device_rtc = {
        .name             = "s3c2410-rtc",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_rtc_resource),
        .resource         = s3c_rtc_resource,
};

/* OneNAND Controller */
static struct resource s3c_onenand_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_ONENAND,
                .end   = S5PC1XX_PA_ONENAND + S5PC1XX_SZ_ONENAND - 1,
                .flags = IORESOURCE_MEM,
        }
};

struct platform_device s3c_device_onenand = {
        .name             = "onenand",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_onenand_resource),
        .resource         = s3c_onenand_resource,
};

EXPORT_SYMBOL(s3c_device_onenand);

/* Keypad interface */
static struct resource s3c_keypad_resource[] = {
	[0] = {
		.start = S3C_PA_KEYPAD,
		.end   = S3C_PA_KEYPAD+ S3C_SZ_KEYPAD - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_KEYPAD,
		.end   = IRQ_KEYPAD,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_keypad = {
	.name		  = "s3c-keypad",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_keypad_resource),
	.resource	  = s3c_keypad_resource,
};

EXPORT_SYMBOL(s3c_device_keypad);

/* AC97 */

static struct resource s3c_ac97_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_AC97,
                .end   = S5PC1XX_PA_AC97 + S5PC1XX_SZ_AC97 -1,
                .flags = IORESOURCE_MEM,
        }
};

static u64 s3c_device_ac97_dmamask = 0xffffffffUL;

struct platform_device s3c_device_ac97 = {
        .name             = "s3c-ac97",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_ac97_resource),
        .resource         = s3c_ac97_resource,
        .dev              = {
                .dma_mask = &s3c_device_ac97_dmamask,
                .coherent_dma_mask = 0xffffffffUL
        }
};

EXPORT_SYMBOL(s3c_device_ac97);

static struct resource s3c_g3d_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_G3D,
                .end   = S5PC1XX_PA_G3D + S5PC1XX_SZ_G3D - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_3D,
                .end   = IRQ_3D,
                .flags = IORESOURCE_IRQ,
        }
};

static u64 s3c_device_g3d_dmamask = 0xffffffffUL;

struct platform_device s3c_device_g3d = {
        .name             = "s3c-g3d",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_g3d_resource),
        .resource         = s3c_g3d_resource,
        .dev              = {
                .dma_mask = &s3c_device_g3d_dmamask,
                .coherent_dma_mask = 0xffffffffUL
        }
};

EXPORT_SYMBOL(s3c_device_g3d);

/* JPEG controller  */
static struct resource s3c_jpeg_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_JPEG,
                .end   = S5PC1XX_PA_JPEG + S5PC1XX_SZ_JPEG - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_JPEG,
                .end   = IRQ_JPEG,
                .flags = IORESOURCE_IRQ,
        }

};

struct platform_device s3c_device_jpeg = {
        .name             = "s3c-jpg",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_jpeg_resource),
        .resource         = s3c_jpeg_resource,
};
EXPORT_SYMBOL(s3c_device_jpeg);

/* rotator interface */
static struct resource s3c_rotator_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_ROTATOR,
                .end   = S5PC1XX_PA_ROTATOR + S5PC1XX_SZ_ROTATOR - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_ROTATOR,
                .end   = IRQ_ROTATOR,
                .flags = IORESOURCE_IRQ,
        }
};

struct platform_device s3c_device_rotator = {
        .name             = "s3c-rotator",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_rotator_resource),
        .resource         = s3c_rotator_resource
};

EXPORT_SYMBOL(s3c_device_rotator);


/* TVOUT interface */
static struct resource s5p_tvout_resources[] = {
	[0] = {
		.start  = S5PC1XX_PA_TVENC,
		.end    = S5PC1XX_PA_TVENC + S5PC1XX_SZ_TVENC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = S5PC1XX_PA_VP,
		.end    = S5PC1XX_PA_VP + S5PC1XX_SZ_VP - 1,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {
		.start  = S5PC1XX_PA_MIXER,
		.end    = S5PC1XX_PA_MIXER + S5PC1XX_SZ_MIXER - 1,
		.flags  = IORESOURCE_MEM,
	},
	[3] = {
		.start  = S5PC1XX_PA_HDMI,
		.end    = S5PC1XX_PA_HDMI + S5PC1XX_SZ_HDMI - 1,
		.flags  = IORESOURCE_MEM,
	},
	[4] = {
		.start  = S5PC1XX_PA_CLK_OTHER,
		.end    = S5PC1XX_PA_CLK_OTHER + S5PC1XX_SZ_CLK_OTHER - 1,
		.flags  = IORESOURCE_MEM,
	},
	[5] = {
		.start  = IRQ_MIXER,
		.end    = IRQ_MIXER,
		.flags  = IORESOURCE_IRQ,
	},
	[6] = {
		.start  = IRQ_HDMI,
		.end    = IRQ_HDMI,
		.flags  = IORESOURCE_IRQ,
	},
	[7] = {
		.start  = IRQ_TVENC,
		.end    = IRQ_TVENC,
		.flags  = IORESOURCE_IRQ,
	},
	[8] = {
		.start  = IRQ_EINT5,
		.end    = IRQ_EINT5,
		.flags  = IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_tvout = {
	.name           = "s5p-tvout",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_tvout_resources),
	.resource       = s5p_tvout_resources,
};
EXPORT_SYMBOL(s5p_device_tvout);

/* CFCON */
static struct resource s3c_cfcon_resource[] = {
        [0] = {
                .start = S5PC1XX_PA_CFCON,
                .end   = S5PC1XX_PA_CFCON + SZ_1M - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_CFC,
                .end   = IRQ_CFC,
                .flags = IORESOURCE_IRQ,
        },
};

struct platform_device s3c_device_cfcon = {
        .name             = "s3c-ide",
        .id               = 0,
        .num_resources    = ARRAY_SIZE(s3c_cfcon_resource),
        .resource         = s3c_cfcon_resource,
};
EXPORT_SYMBOL(s3c_device_cfcon);
