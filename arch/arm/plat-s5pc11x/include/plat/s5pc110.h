/* arch/arm/plat-s5pc1xx/include/plat/s5pc110.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Header file for s5pc110 cpu support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

/* Common init code for S5PC110 related SoCs */

extern void s5pc110_common_init_uarts(struct s3c2410_uartcfg *cfg, int no);
extern void s5pc110_register_clocks(void);
extern void s5pc110_setup_clocks(void);

extern  int s5pc110_init(void);
extern void s5pc110_init_irq(void);
extern void s5pc110_map_io(void);
extern void s5pc110_init_clocks(int xtal);

#define s5pc110_init_uarts s5pc110_common_init_uarts


