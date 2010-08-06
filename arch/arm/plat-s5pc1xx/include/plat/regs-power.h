/* arch/arm/plat-s5pc1xx/include/plat/regs-power.h
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S5PC1XX power control register definitions
*/

#ifndef __ASM_ARM_REGS_PWR
#define __ASM_ARM_REGS_PWR __FILE__

/* register for EINT on PM Driver */
#define S5P_APM_GPIO		(S5PC1XX_PA_GPIO + 0xC00)
#define S5P_APM_REG(x)		((x) + S5P_APM_GPIO)

#define S5P_APM_BASE	 	S5P_APM_REG(0x000)
#define S5P_APM_GPH1CON 	(0x020)
#define S5P_APM_GPH1DAT 	(0x024)
#define S5P_APM_GPH1PUD 	(0x028)
#define S5P_APM_GPH1DRV 	(0x02C)
#define S5P_APM_GPH2CON 	(0x040)
#define S5P_APM_GPH2DAT 	(0x044)
#define S5P_APM_GPH2PUD 	(0x048)
#define S5P_APM_GPH2DRV 	(0x04C)
#define S5P_APM_GPH3CON 	(0x060)
#define S5P_APM_GPH3DAT 	(0x064)
#define S5P_APM_GPH3PUD 	(0x068)
#define S5P_APM_GPH3DRV 	(0x06C)
#define S5P_APM_WEINT0_CON 	(0x200)
#define S5P_APM_WEINT1_CON 	(0x204)
#define S5P_APM_WEINT2_CON 	(0x208)
#define S5P_APM_WEINT3_CON 	(0x20C)
#define S5P_APM_WEINT0_FLTCON0 	(0x280)
#define S5P_APM_WEINT0_FLTCON1 	(0x284)
#define S5P_APM_WEINT1_FLTCON0 	(0x288)
#define S5P_APM_WEINT1_FLTCON1 	(0x28C)
#define S5P_APM_WEINT2_FLTCON0 	(0x290)
#define S5P_APM_WEINT2_FLTCON1 	(0x294)
#define S5P_APM_WEINT3_FLTCON0 	(0x298)
#define S5P_APM_WEINT3_FLTCON1 	(0x29C)
#define S5P_APM_WEINT0_MASK 	(0x300)
#define S5P_APM_WEINT1_MASK 	(0x304)
#define S5P_APM_WEINT2_MASK 	(0x308)
#define S5P_APM_WEINT3_MASK 	(0x30C)
#define S5P_APM_WEINT0_PEND 	(0x340)
#define S5P_APM_WEINT1_PEND 	(0x344)
#define S5P_APM_WEINT2_PEND 	(0x348)
#define S5P_APM_WEINT3_PEND 	(0x34C)

#endif /* __ASM_ARM_REGS_PWR */

