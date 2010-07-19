/*
 * SH7763 Setup
 *
 *  Copyright (C) 2006  Paul Mundt
 *  Copyright (C) 2007  Yoshihiro Shimoda
 *  Copyright (C) 2008  Nobuhiro Iwamatsu
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/io.h>
#include <linux/serial_sci.h>

static struct resource rtc_resources[] = {
	[0] = {
		.start	= 0xffe80000,
		.end	= 0xffe80000 + 0x58 - 1,
		.flags	= IORESOURCE_IO,
	},
	[1] = {
		/* Period IRQ */
		.start	= 21,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		/* Carry IRQ */
		.start	= 22,
		.flags	= IORESOURCE_IRQ,
	},
	[3] = {
		/* Alarm IRQ */
		.start	= 20,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device rtc_device = {
	.name		= "sh-rtc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(rtc_resources),
	.resource	= rtc_resources,
};

static struct plat_sci_port sci_platform_data[] = {
	{
		.mapbase	= 0xffe00000,
		.flags		= UPF_BOOT_AUTOCONF,
		.type		= PORT_SCIF,
		.irqs		= { 40, 41, 43, 42 },
	}, {
		.mapbase	= 0xffe08000,
		.flags		= UPF_BOOT_AUTOCONF,
		.type		= PORT_SCIF,
		.irqs		= { 76, 77, 79, 78 },
	}, {
		.mapbase	= 0xffe10000,
		.flags		= UPF_BOOT_AUTOCONF,
		.type		= PORT_SCIF,
		.irqs		= { 104, 105, 107, 106 },
	}, {
		.flags = 0,
	}
};

static struct platform_device sci_device = {
	.name		= "sh-sci",
	.id		= -1,
	.dev		= {
		.platform_data	= sci_platform_data,
	},
};

static struct resource usb_ohci_resources[] = {
	[0] = {
		.start	= 0xffec8000,
		.end	= 0xffec80ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= 83,
		.end	= 83,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 usb_ohci_dma_mask = 0xffffffffUL;
static struct platform_device usb_ohci_device = {
	.name		= "sh_ohci",
	.id		= -1,
	.dev = {
		.dma_mask		= &usb_ohci_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(usb_ohci_resources),
	.resource	= usb_ohci_resources,
};

static struct resource usbf_resources[] = {
	[0] = {
		.start	= 0xffec0000,
		.end	= 0xffec00ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= 84,
		.end	= 84,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device usbf_device = {
	.name		= "sh_udc",
	.id		= -1,
	.dev = {
		.dma_mask		= NULL,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(usbf_resources),
	.resource	= usbf_resources,
};

static struct platform_device *sh7763_devices[] __initdata = {
	&rtc_device,
	&sci_device,
	&usb_ohci_device,
	&usbf_device,
};

static int __init sh7763_devices_setup(void)
{
	return platform_add_devices(sh7763_devices,
				    ARRAY_SIZE(sh7763_devices));
}
__initcall(sh7763_devices_setup);

enum {
	UNUSED = 0,

	/* interrupt sources */

	IRL_LLLL, IRL_LLLH, IRL_LLHL, IRL_LLHH,
	IRL_LHLL, IRL_LHLH, IRL_LHHL, IRL_LHHH,
	IRL_HLLL, IRL_HLLH, IRL_HLHL, IRL_HLHH,
	IRL_HHLL, IRL_HHLH, IRL_HHHL,

	IRQ0, IRQ1, IRQ2, IRQ3, IRQ4, IRQ5, IRQ6, IRQ7,
	RTC_ATI, RTC_PRI, RTC_CUI,
	WDT, TMU0, TMU1, TMU2, TMU2_TICPI,
	HUDI, LCDC,
	DMAC0_DMINT0, DMAC0_DMINT1, DMAC0_DMINT2, DMAC0_DMINT3, DMAC0_DMAE,
	SCIF0_ERI, SCIF0_RXI, SCIF0_BRI, SCIF0_TXI,
	DMAC0_DMINT4, DMAC0_DMINT5,
	IIC0, IIC1,
	CMT,
	GEINT0, GEINT1, GEINT2,
	HAC,
	PCISERR, PCIINTA, PCIINTB, PCIINTC, PCIINTD,
	PCIERR, PCIPWD3, PCIPWD2, PCIPWD1, PCIPWD0,
	STIF0, STIF1,
	SCIF1_ERI, SCIF1_RXI, SCIF1_BRI, SCIF1_TXI,
	SIOF0, SIOF1, SIOF2,
	USBH, USBFI0, USBFI1,
	TPU, PCC,
	MMCIF_FSTAT, MMCIF_TRAN, MMCIF_ERR, MMCIF_FRDY,
	SIM_ERI, SIM_RXI, SIM_TXI, SIM_TEND,
	TMU3, TMU4, TMU5, ADC, SSI0, SSI1, SSI2, SSI3,
	SCIF2_ERI, SCIF2_RXI, SCIF2_BRI, SCIF2_TXI,
	GPIO_CH0, GPIO_CH1, GPIO_CH2, GPIO_CH3,

	/* interrupt groups */

	TMU012, TMU345, RTC, DMAC, SCIF0, GETHER, PCIC5,
	SCIF1, USBF, MMCIF, SIM, SCIF2, GPIO,
};

static struct intc_vect vectors[] __initdata = {
	INTC_VECT(RTC_ATI, 0x480), INTC_VECT(RTC_PRI, 0x4a0),
	INTC_VECT(RTC_CUI, 0x4c0),
	INTC_VECT(WDT, 0x560), INTC_VECT(TMU0, 0x580),
	INTC_VECT(TMU1, 0x5a0), INTC_VECT(TMU2, 0x5c0),
	INTC_VECT(TMU2_TICPI, 0x5e0), INTC_VECT(HUDI, 0x600),
	INTC_VECT(LCDC, 0x620),
	INTC_VECT(DMAC0_DMINT0, 0x640), INTC_VECT(DMAC0_DMINT1, 0x660),
	INTC_VECT(DMAC0_DMINT2, 0x680), INTC_VECT(DMAC0_DMINT3, 0x6a0),
	INTC_VECT(DMAC0_DMAE, 0x6c0),
	INTC_VECT(SCIF0_ERI, 0x700), INTC_VECT(SCIF0_RXI, 0x720),
	INTC_VECT(SCIF0_BRI, 0x740), INTC_VECT(SCIF0_TXI, 0x760),
	INTC_VECT(DMAC0_DMINT4, 0x780), INTC_VECT(DMAC0_DMINT5, 0x7a0),
	INTC_VECT(IIC0, 0x8A0), INTC_VECT(IIC1, 0x8C0),
	INTC_VECT(CMT, 0x900), INTC_VECT(GEINT0, 0x920),
	INTC_VECT(GEINT1, 0x940), INTC_VECT(GEINT2, 0x960),
	INTC_VECT(HAC, 0x980),
	INTC_VECT(PCISERR, 0xa00), INTC_VECT(PCIINTA, 0xa20),
	INTC_VECT(PCIINTB, 0xa40), INTC_VECT(PCIINTC, 0xa60),
	INTC_VECT(PCIINTD, 0xa80), INTC_VECT(PCIERR, 0xaa0),
	INTC_VECT(PCIPWD3, 0xac0), INTC_VECT(PCIPWD2, 0xae0),
	INTC_VECT(PCIPWD1, 0xb00), INTC_VECT(PCIPWD0, 0xb20),
	INTC_VECT(STIF0, 0xb40), INTC_VECT(STIF1, 0xb60),
	INTC_VECT(SCIF1_ERI, 0xb80), INTC_VECT(SCIF1_RXI, 0xba0),
	INTC_VECT(SCIF1_BRI, 0xbc0), INTC_VECT(SCIF1_TXI, 0xbe0),
	INTC_VECT(SIOF0, 0xc00), INTC_VECT(SIOF1, 0xc20),
	INTC_VECT(USBH, 0xc60), INTC_VECT(USBFI0, 0xc80),
	INTC_VECT(USBFI1, 0xca0),
	INTC_VECT(TPU, 0xcc0), INTC_VECT(PCC, 0xce0),
	INTC_VECT(MMCIF_FSTAT, 0xd00), INTC_VECT(MMCIF_TRAN, 0xd20),
	INTC_VECT(MMCIF_ERR, 0xd40), INTC_VECT(MMCIF_FRDY, 0xd60),
	INTC_VECT(SIM_ERI, 0xd80), INTC_VECT(SIM_RXI, 0xda0),
	INTC_VECT(SIM_TXI, 0xdc0), INTC_VECT(SIM_TEND, 0xde0),
	INTC_VECT(TMU3, 0xe00), INTC_VECT(TMU4, 0xe20),
	INTC_VECT(TMU5, 0xe40), INTC_VECT(ADC, 0xe60),
	INTC_VECT(SSI0, 0xe80), INTC_VECT(SSI1, 0xea0),
	INTC_VECT(SSI2, 0xec0), INTC_VECT(SSI3, 0xee0),
	INTC_VECT(SCIF2_ERI, 0xf00), INTC_VECT(SCIF2_RXI, 0xf20),
	INTC_VECT(SCIF2_BRI, 0xf40), INTC_VECT(SCIF2_TXI, 0xf60),
	INTC_VECT(GPIO_CH0, 0xf80), INTC_VECT(GPIO_CH1, 0xfa0),
	INTC_VECT(GPIO_CH2, 0xfc0), INTC_VECT(GPIO_CH3, 0xfe0),
};

static struct intc_group groups[] __initdata = {
	INTC_GROUP(TMU012, TMU0, TMU1, TMU2, TMU2_TICPI),
	INTC_GROUP(TMU345, TMU3, TMU4, TMU5),
	INTC_GROUP(RTC, RTC_ATI, RTC_PRI, RTC_CUI),
	INTC_GROUP(DMAC, DMAC0_DMINT0, DMAC0_DMINT1, DMAC0_DMINT2,
		   DMAC0_DMINT3, DMAC0_DMINT4, DMAC0_DMINT5, DMAC0_DMAE),
	INTC_GROUP(SCIF0, SCIF0_ERI, SCIF0_RXI, SCIF0_BRI, SCIF0_TXI),
	INTC_GROUP(GETHER, GEINT0, GEINT1, GEINT2),
	INTC_GROUP(PCIC5, PCIERR, PCIPWD3, PCIPWD2, PCIPWD1, PCIPWD0),
	INTC_GROUP(SCIF1, SCIF1_ERI, SCIF1_RXI, SCIF1_BRI, SCIF1_TXI),
	INTC_GROUP(USBF, USBFI0, USBFI1),
	INTC_GROUP(MMCIF, MMCIF_FSTAT, MMCIF_TRAN, MMCIF_ERR, MMCIF_FRDY),
	INTC_GROUP(SIM, SIM_ERI, SIM_RXI, SIM_TXI, SIM_TEND),
	INTC_GROUP(SCIF2, SCIF2_ERI, SCIF2_RXI, SCIF2_BRI, SCIF2_TXI),
	INTC_GROUP(GPIO, GPIO_CH0, GPIO_CH1, GPIO_CH2, GPIO_CH3),
};

static struct intc_mask_reg mask_registers[] __initdata = {
	{ 0xffd40038, 0xffd4003c, 32, /* INT2MSKR / INT2MSKCR */
	  { 0, 0, 0, 0, 0, 0, GPIO, 0,
	    SSI0, MMCIF, 0, SIOF0, PCIC5, PCIINTD, PCIINTC, PCIINTB,
	    PCIINTA, PCISERR, HAC, CMT, 0, 0, 0, DMAC,
	    HUDI, 0, WDT, SCIF1, SCIF0, RTC, TMU345, TMU012 } },
	{ 0xffd400d0, 0xffd400d4, 32, /* INT2MSKR1 / INT2MSKCR1 */
	  { 0, 0, 0, 0, 0, 0, SCIF2, USBF,
	    0, 0, STIF1, STIF0, 0, 0, USBH, GETHER,
	    PCC, 0, 0, ADC, TPU, SIM, SIOF2, SIOF1,
	    LCDC, 0, IIC1, IIC0, SSI3, SSI2, SSI1, 0 } },
};

static struct intc_prio_reg prio_registers[] __initdata = {
	{ 0xffd40000, 0, 32, 8, /* INT2PRI0 */ { TMU0, TMU1,
						 TMU2, TMU2_TICPI } },
	{ 0xffd40004, 0, 32, 8, /* INT2PRI1 */ { TMU3, TMU4, TMU5, RTC } },
	{ 0xffd40008, 0, 32, 8, /* INT2PRI2 */ { SCIF0, SCIF1, WDT } },
	{ 0xffd4000c, 0, 32, 8, /* INT2PRI3 */ { HUDI, DMAC, ADC } },
	{ 0xffd40010, 0, 32, 8, /* INT2PRI4 */ { CMT, HAC,
						 PCISERR, PCIINTA } },
	{ 0xffd40014, 0, 32, 8, /* INT2PRI5 */ { PCIINTB, PCIINTC,
						 PCIINTD, PCIC5 } },
	{ 0xffd40018, 0, 32, 8, /* INT2PRI6 */ { SIOF0, USBF, MMCIF, SSI0 } },
	{ 0xffd4001c, 0, 32, 8, /* INT2PRI7 */ { SCIF2, GPIO } },
	{ 0xffd400a0, 0, 32, 8, /* INT2PRI8 */ { SSI3, SSI2, SSI1, 0 } },
	{ 0xffd400a4, 0, 32, 8, /* INT2PRI9 */ { LCDC, 0, IIC1, IIC0 } },
	{ 0xffd400a8, 0, 32, 8, /* INT2PRI10 */ { TPU, SIM, SIOF2, SIOF1 } },
	{ 0xffd400ac, 0, 32, 8, /* INT2PRI11 */ { PCC } },
	{ 0xffd400b0, 0, 32, 8, /* INT2PRI12 */ { 0, 0, USBH, GETHER } },
	{ 0xffd400b4, 0, 32, 8, /* INT2PRI13 */ { 0, 0, STIF1, STIF0 } },
};

static DECLARE_INTC_DESC(intc_desc, "sh7763", vectors, groups,
			 mask_registers, prio_registers, NULL);

/* Support for external interrupt pins in IRQ mode */
static struct intc_vect irq_vectors[] __initdata = {
	INTC_VECT(IRQ0, 0x240), INTC_VECT(IRQ1, 0x280),
	INTC_VECT(IRQ2, 0x2c0), INTC_VECT(IRQ3, 0x300),
	INTC_VECT(IRQ4, 0x340), INTC_VECT(IRQ5, 0x380),
	INTC_VECT(IRQ6, 0x3c0), INTC_VECT(IRQ7, 0x200),
};

static struct intc_mask_reg irq_mask_registers[] __initdata = {
	{ 0xffd00044, 0xffd00064, 32, /* INTMSK0 / INTMSKCLR0 */
	  { IRQ0, IRQ1, IRQ2, IRQ3, IRQ4, IRQ5, IRQ6, IRQ7 } },
};

static struct intc_prio_reg irq_prio_registers[] __initdata = {
	{ 0xffd00010, 0, 32, 4, /* INTPRI */ { IRQ0, IRQ1, IRQ2, IRQ3,
					       IRQ4, IRQ5, IRQ6, IRQ7 } },
};

static struct intc_sense_reg irq_sense_registers[] __initdata = {
	{ 0xffd0001c, 32, 2, /* ICR1 */   { IRQ0, IRQ1, IRQ2, IRQ3,
					    IRQ4, IRQ5, IRQ6, IRQ7 } },
};

static struct intc_mask_reg irq_ack_registers[] __initdata = {
	{ 0xffd00024, 0, 32, /* INTREQ */
	  { IRQ0, IRQ1, IRQ2, IRQ3, IRQ4, IRQ5, IRQ6, IRQ7 } },
};

static DECLARE_INTC_DESC_ACK(intc_irq_desc, "sh7763-irq", irq_vectors,
			     NULL, irq_mask_registers, irq_prio_registers,
			     irq_sense_registers, irq_ack_registers);


/* External interrupt pins in IRL mode */
static struct intc_vect irl_vectors[] __initdata = {
	INTC_VECT(IRL_LLLL, 0x200), INTC_VECT(IRL_LLLH, 0x220),
	INTC_VECT(IRL_LLHL, 0x240), INTC_VECT(IRL_LLHH, 0x260),
	INTC_VECT(IRL_LHLL, 0x280), INTC_VECT(IRL_LHLH, 0x2a0),
	INTC_VECT(IRL_LHHL, 0x2c0), INTC_VECT(IRL_LHHH, 0x2e0),
	INTC_VECT(IRL_HLLL, 0x300), INTC_VECT(IRL_HLLH, 0x320),
	INTC_VECT(IRL_HLHL, 0x340), INTC_VECT(IRL_HLHH, 0x360),
	INTC_VECT(IRL_HHLL, 0x380), INTC_VECT(IRL_HHLH, 0x3a0),
	INTC_VECT(IRL_HHHL, 0x3c0),
};

static struct intc_mask_reg irl3210_mask_registers[] __initdata = {
	{ 0xffd40080, 0xffd40084, 32, /* INTMSK2 / INTMSKCLR2 */
	  { IRL_LLLL, IRL_LLLH, IRL_LLHL, IRL_LLHH,
	    IRL_LHLL, IRL_LHLH, IRL_LHHL, IRL_LHHH,
	    IRL_HLLL, IRL_HLLH, IRL_HLHL, IRL_HLHH,
	    IRL_HHLL, IRL_HHLH, IRL_HHHL, } },
};

static struct intc_mask_reg irl7654_mask_registers[] __initdata = {
	{ 0xffd40080, 0xffd40084, 32, /* INTMSK2 / INTMSKCLR2 */
	  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	    IRL_LLLL, IRL_LLLH, IRL_LLHL, IRL_LLHH,
	    IRL_LHLL, IRL_LHLH, IRL_LHHL, IRL_LHHH,
	    IRL_HLLL, IRL_HLLH, IRL_HLHL, IRL_HLHH,
	    IRL_HHLL, IRL_HHLH, IRL_HHHL, } },
};

static DECLARE_INTC_DESC(intc_irl7654_desc, "sh7763-irl7654", irl_vectors,
			NULL, irl7654_mask_registers, NULL, NULL);

static DECLARE_INTC_DESC(intc_irl3210_desc, "sh7763-irl3210", irl_vectors,
			NULL, irl3210_mask_registers, NULL, NULL);

#define INTC_ICR0	0xffd00000
#define INTC_INTMSK0	0xffd00044
#define INTC_INTMSK1	0xffd00048
#define INTC_INTMSK2	0xffd40080
#define INTC_INTMSKCLR1	0xffd00068
#define INTC_INTMSKCLR2	0xffd40084

void __init plat_irq_setup(void)
{
	/* disable IRQ7-0 */
	ctrl_outl(0xff000000, INTC_INTMSK0);

	/* disable IRL3-0 + IRL7-4 */
	ctrl_outl(0xc0000000, INTC_INTMSK1);
	ctrl_outl(0xfffefffe, INTC_INTMSK2);

	register_intc_controller(&intc_desc);
}

void __init plat_irq_setup_pins(int mode)
{
	switch (mode) {
	case IRQ_MODE_IRQ:
		/* select IRQ mode for IRL3-0 + IRL7-4 */
		ctrl_outl(ctrl_inl(INTC_ICR0) | 0x00c00000, INTC_ICR0);
		register_intc_controller(&intc_irq_desc);
		break;
	case IRQ_MODE_IRL7654:
		/* enable IRL7-4 but don't provide any masking */
		ctrl_outl(0x40000000, INTC_INTMSKCLR1);
		ctrl_outl(0x0000fffe, INTC_INTMSKCLR2);
		break;
	case IRQ_MODE_IRL3210:
		/* enable IRL0-3 but don't provide any masking */
		ctrl_outl(0x80000000, INTC_INTMSKCLR1);
		ctrl_outl(0xfffe0000, INTC_INTMSKCLR2);
		break;
	case IRQ_MODE_IRL7654_MASK:
		/* enable IRL7-4 and mask using cpu intc controller */
		ctrl_outl(0x40000000, INTC_INTMSKCLR1);
		register_intc_controller(&intc_irl7654_desc);
		break;
	case IRQ_MODE_IRL3210_MASK:
		/* enable IRL0-3 and mask using cpu intc controller */
		ctrl_outl(0x80000000, INTC_INTMSKCLR1);
		register_intc_controller(&intc_irl3210_desc);
		break;
	default:
		BUG();
	}
}
