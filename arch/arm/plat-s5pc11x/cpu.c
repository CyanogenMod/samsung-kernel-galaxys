/* linux/arch/arm/plat-s5pc1xx/cpu.c
 *
 * Copyright 2008 Samsung Electonics Co.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5PC11X CPU Support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/regs-serial.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/s5pc110.h>

/* table of supported CPUs */

static const char name_s5pc110[] = "S5PC110";

static struct cpu_table cpu_ids[] __initdata = {
	{
		.idcode		= 0x43110000,
		.idmask		= 0xfffff000,
		.map_io		= s5pc110_map_io,
		.init_clocks    = s5pc110_init_clocks,
		.init_uarts	= s5pc110_init_uarts,
		.init		= s5pc110_init,
		.name		= name_s5pc110
	},	
};

/* minimal IO mapping */

/* see notes on uart map in arch/arm/mach-s3c6400/include/mach/debug-macro.S */
#define UART_OFFS (S3C_PA_UART & 0xfffff)

static struct map_desc s3c_iodesc[] __initdata = {
	{
		.virtual	= (unsigned long)S3C_VA_SYS,
		.pfn		= __phys_to_pfn(S5PC11X_PA_SYSCON),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)(S3C_VA_UART + UART_OFFS),
		.pfn		= __phys_to_pfn(S3C_PA_UART),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC0,
		.pfn		= __phys_to_pfn(S5PC11X_PA_VIC0),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC1,
		.pfn		= __phys_to_pfn(S5PC11X_PA_VIC1),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC2,
		.pfn		= __phys_to_pfn(S5PC11X_PA_VIC2),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC3,
		.pfn		= __phys_to_pfn(S5PC11X_PA_VIC3),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_TIMER,
		.pfn		= __phys_to_pfn(S3C_PA_TIMER),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PC11X_VA_GPIO,
		.pfn		= __phys_to_pfn(S5PC11X_PA_GPIO),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PC11X_VA_CHIPID,
		.pfn		= __phys_to_pfn(S5PC11X_PA_CHIPID),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_DMC0,
		.pfn		= __phys_to_pfn(S5P_PA_DMC0),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_DMC1,
		.pfn		= __phys_to_pfn(S5P_PA_DMC1),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_WATCHDOG,
		.pfn		= __phys_to_pfn(S3C_PA_WDT),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual    = (unsigned long)ONEDRAM1G_SHARED_AREA_VIRT,
         	.pfn        = __phys_to_pfn(ONEDRAM1G_SHARED_AREA_PHYS),
         	.length     = SZ_16M,
        	 .type       = MT_DEVICE,

	}, 
};

/* read cpu identification code */

void __init s5pc11x_init_io(struct map_desc *mach_desc, int size)
{
	unsigned long idcode;

	/* initialise the io descriptors we need for initialisation */
	iotable_init(s3c_iodesc, ARRAY_SIZE(s3c_iodesc));
	iotable_init(mach_desc, size);

	idcode = __raw_readl(S5PC11X_VA_CHIPID);

	s3c_init_cpu(idcode, cpu_ids, ARRAY_SIZE(cpu_ids));
}
