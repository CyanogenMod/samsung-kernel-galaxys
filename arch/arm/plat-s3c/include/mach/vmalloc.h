/* arch/arm/plat-s3c/include/mach/vmalloc.h
 *
 * from arch/arm/mach-iop3xx/include/mach/vmalloc.h
 *
 * Copyright (c) 2003 Simtec Electronics <linux@simtec.co.uk>
 *		      http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410 vmalloc definition
*/

#ifndef __ASM_ARCH_VMALLOC_H
#define __ASM_ARCH_VMALLOC_H
#ifdef CONFIG_DDR_RAM_3G
#define VMALLOC_END	  (0xfA000000)
#else
#define VMALLOC_END	  (0xF0000000)
#endif

#endif /* __ASM_ARCH_VMALLOC_H */
