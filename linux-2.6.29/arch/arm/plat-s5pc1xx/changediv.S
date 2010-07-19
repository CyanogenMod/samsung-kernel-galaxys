/* linux/arch/arm/plat-s5pc1xx/changediv.S
 *
 * Copyright (c) 2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S5PC100 Clock Change (DVFS) support
 *
 * Based on PXA/SA1100 sleep code by:
 *	Nicolas Pitre, (c) 2002 Monta Vista Software Inc
 *	Cliff Brake, (c) 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>
#include <mach/regs-mem.h>
#include <plat/regs-serial.h>


/* CONFIG_DEBUG_RESUME is dangerous if your bootloader does not
 * reset the UART configuration, only enable if you really need this!
*/
	.text

	/* ChangeClkDiv0
	 *
	 * save enough of the CPU state to allow us to re-start
	 * pm.c code. as we store items like the sp/lr, we will
	 * end up returning from this function when the cpu resumes
	 * so the return value is set to mark this.
	 *
	 * This arangement means we avoid having to flush the cache
	 * from this code.
	 *
	 * entry:
	 *	r0 = pointer to save block
	 *
	 * exit:
	 *	r0 = 0 => we stored everything
	 *	     1 => resumed from sleep
	*/

ENTRY(ChangeClkDiv0)
	stmfd	sp!, { r0 - r5 }
	
	ldr r1, =S5P_CLK_DIV0
	ldr r2, [r1]
	mov r3, #0
	
loopcd:
  add r3, r3, #1
  mov r4, #0
  
  mcr p15, 0, r2, c7, c10, 4 @ data synchronization barrier instruction
  mcr p15, 0, r2, c7, c10, 5 @ data memory barrier operation

  cmp r3, #2
  streq r0, [r1]

loop1000:
  add r4, r4, #1
  cmp r4, #0x2000
  bne loop1000
  cmp r3, #2
  bne loopcd
  
  ldmfd sp!, { r0 - r5 }
  mov pc, lr  

  .ltorg

