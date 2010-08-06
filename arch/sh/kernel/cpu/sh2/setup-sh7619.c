/*
 * SH7619 Setup
 *
 *  Copyright (C) 2006  Yoshinori Sato
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/serial_sci.h>

enum {
	UNUSED = 0,

	/* interrupt sources */
	IRQ0, IRQ1, IRQ2, IRQ3, IRQ4, IRQ5, IRQ6, IRQ7,
	WDT, EDMAC, CMT0, CMT1,
	SCIF0_ERI, SCIF0_RXI, SCIF0_BRI, SCIF0_TXI,
	SCIF1_ERI, SCIF1_RXI, SCIF1_BRI, SCIF1_TXI,
	SCIF2_ERI, SCIF2_RXI, SCIF2_BRI, SCIF2_TXI,
	HIF_HIFI, HIF_HIFBI,
	DMAC0, DMAC1, DMAC2, DMAC3,
	SIOF,

	/* interrupt groups */
	SCIF0, SCIF1, SCIF2,
};

static struct intc_vect vectors[] __initdata = {
	INTC_IRQ(IRQ0, 64), INTC_IRQ(IRQ1, 65),
	INTC_IRQ(IRQ2, 66), INTC_IRQ(IRQ3, 67),
	INTC_IRQ(IRQ4, 80), INTC_IRQ(IRQ5, 81),
	INTC_IRQ(IRQ6, 82), INTC_IRQ(IRQ7, 83),
	INTC_IRQ(WDT, 84), INTC_IRQ(EDMAC, 85),
	INTC_IRQ(CMT0, 86), INTC_IRQ(CMT1, 87),
	INTC_IRQ(SCIF0_ERI, 88), INTC_IRQ(SCIF0_RXI, 89),
	INTC_IRQ(SCIF0_BRI, 90), INTC_IRQ(SCIF0_TXI, 91),
	INTC_IRQ(SCIF1_ERI, 92), INTC_IRQ(SCIF1_RXI, 93),
	INTC_IRQ(SCIF1_BRI, 94), INTC_IRQ(SCIF1_TXI, 95),
	INTC_IRQ(SCIF2_ERI, 96), INTC_IRQ(SCIF2_RXI, 97),
	INTC_IRQ(SCIF2_BRI, 98), INTC_IRQ(SCIF2_TXI, 99),
	INTC_IRQ(HIF_HIFI, 100), INTC_IRQ(HIF_HIFBI, 101),
	INTC_IRQ(DMAC0, 104), INTC_IRQ(DMAC1, 105),
	INTC_IRQ(DMAC2, 106), INTC_IRQ(DMAC3, 107),
	INTC_IRQ(SIOF, 108),
};

static struct intc_group groups[] __initdata = {
	INTC_GROUP(SCIF0, SCIF0_ERI, SCIF0_RXI, SCIF0_BRI, SCIF0_TXI),
	INTC_GROUP(SCIF1, SCIF1_ERI, SCIF1_RXI, SCIF1_BRI, SCIF1_TXI),
	INTC_GROUP(SCIF2, SCIF2_ERI, SCIF2_RXI, SCIF2_BRI, SCIF2_TXI),
};

static struct intc_prio_reg prio_registers[] __initdata = {
	{ 0xf8140006, 0, 16, 4, /* IPRA */ { IRQ0, IRQ1, IRQ2, IRQ3 } },
	{ 0xf8140008, 0, 16, 4, /* IPRB */ { IRQ4, IRQ5, IRQ6, IRQ7 } },
	{ 0xf8080000, 0, 16, 4, /* IPRC */ { WDT, EDMAC, CMT0, CMT1 } },
	{ 0xf8080002, 0, 16, 4, /* IPRD */ { SCIF0, SCIF1, SCIF2 } },
	{ 0xf8080004, 0, 16, 4, /* IPRE */ { HIF_HIFI, HIF_HIFBI } },
	{ 0xf8080006, 0, 16, 4, /* IPRF */ { DMAC0, DMAC1, DMAC2, DMAC3 } },
	{ 0xf8080008, 0, 16, 4, /* IPRG */ { SIOF } },
};

static DECLARE_INTC_DESC(intc_desc, "sh7619", vectors, groups,
			 NULL, prio_registers, NULL);

static struct plat_sci_port sci_platform_data[] = {
	{
		.mapbase	= 0xf8400000,
		.flags		= UPF_BOOT_AUTOCONF,
		.type		= PORT_SCIF,
		.irqs		=  { 88, 89, 91, 90},
	}, {
		.mapbase	= 0xf8410000,
		.flags		= UPF_BOOT_AUTOCONF,
		.type		= PORT_SCIF,
		.irqs		=  { 92, 93, 95, 94},
	}, {
		.mapbase	= 0xf8420000,
		.flags		= UPF_BOOT_AUTOCONF,
		.type		= PORT_SCIF,
		.irqs		=  { 96, 97, 99, 98},
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

static struct resource eth_resources[] = {
	[0] = {
		.start = 0xfb000000,
		.end =   0xfb0001c8,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = 85,
		.end = 85,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device eth_device = {
	.name = "sh-eth",
	.id	= -1,
	.dev = {
		.platform_data = (void *)1,
	},
	.num_resources = ARRAY_SIZE(eth_resources),
	.resource = eth_resources,
};

static struct platform_device *sh7619_devices[] __initdata = {
	&sci_device,
	&eth_device,
};

static int __init sh7619_devices_setup(void)
{
	return platform_add_devices(sh7619_devices,
				    ARRAY_SIZE(sh7619_devices));
}
__initcall(sh7619_devices_setup);

void __init plat_irq_setup(void)
{
	register_intc_controller(&intc_desc);
}
