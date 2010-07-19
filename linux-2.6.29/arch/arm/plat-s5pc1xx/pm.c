/* linux/arch/arm/plat-s3c24xx/pm.c
 *
 * Copyright (c) 2004,2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX Power Manager (Suspend-To-RAM) support
 *
 * See Documentation/arm/Samsung-S3C24XX/Suspend.txt for more information
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
 *
 * Parts based on arch/arm/mach-pxa/pm.c
 *
 * Thanks to Dimitry Andric for debugging
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <asm/gpio.h>

#include <asm/mach/time.h>

#include <plat/pm.h>
#include <plat/regs-power.h>

/* for external use */

unsigned long s5pc1xx_pm_flags;
void __iomem *weint_base;

enum PLL_TYPE
{
	PM_APLL,
	PM_MPLL,
	PM_EPLL,
	PM_HPLL
};

#define PFX "s5pc1xx-pm: "
static struct sleep_save core_save[] = {
	SAVE_ITEM(S5P_CLK_SRC0),
	SAVE_ITEM(S5P_CLK_SRC1),
	SAVE_ITEM(S5P_CLK_SRC2),
	SAVE_ITEM(S5P_CLK_SRC3),
	
	SAVE_ITEM(S5P_CLK_DIV0),
	SAVE_ITEM(S5P_CLK_DIV1),
	SAVE_ITEM(S5P_CLK_DIV2),
	SAVE_ITEM(S5P_CLK_DIV3),
	SAVE_ITEM(S5P_CLK_DIV4),

	SAVE_ITEM(S5P_CLK_OUT),

	SAVE_ITEM(S5P_CLKGATE_D00),
	SAVE_ITEM(S5P_CLKGATE_D01),
	SAVE_ITEM(S5P_CLKGATE_D02),

	SAVE_ITEM(S5P_CLKGATE_D10),
	SAVE_ITEM(S5P_CLKGATE_D11),
	SAVE_ITEM(S5P_CLKGATE_D12),
	SAVE_ITEM(S5P_CLKGATE_D13),
	SAVE_ITEM(S5P_CLKGATE_D14),
	SAVE_ITEM(S5P_CLKGATE_D15),

	SAVE_ITEM(S5P_CLKGATE_D20),

	SAVE_ITEM(S5P_SCLKGATE0),
	SAVE_ITEM(S5P_SCLKGATE1),

	SAVE_ITEM(S5P_MEM_SYS_CFG),
	SAVE_ITEM(S5P_CAM_MUX_SEL),
	SAVE_ITEM(S5P_MIXER_OUT_SEL),

	SAVE_ITEM(S5P_LPMP_MODE_SEL),
	SAVE_ITEM(S5P_MIPI_PHY_CON0),
	SAVE_ITEM(S5P_MIPI_PHY_CON1),
	SAVE_ITEM(S5P_HDMI_PHY_CON0),
};

static struct sleep_save gpio_save[] = {
	SAVE_ITEM(S5PC1XX_GPA0CON),
	SAVE_ITEM(S5PC1XX_GPA0DAT),
	SAVE_ITEM(S5PC1XX_GPA0PUD),
	SAVE_ITEM(S5PC1XX_GPA1CON),
	SAVE_ITEM(S5PC1XX_GPA1DAT),
	SAVE_ITEM(S5PC1XX_GPA1PUD),
	SAVE_ITEM(S5PC1XX_GPBCON),
	SAVE_ITEM(S5PC1XX_GPBDAT),
	SAVE_ITEM(S5PC1XX_GPBPUD),
	SAVE_ITEM(S5PC1XX_GPCCON),
	SAVE_ITEM(S5PC1XX_GPCDAT),
	SAVE_ITEM(S5PC1XX_GPCPUD),
	SAVE_ITEM(S5PC1XX_GPDCON),
	SAVE_ITEM(S5PC1XX_GPDDAT),
	SAVE_ITEM(S5PC1XX_GPDPUD),
	SAVE_ITEM(S5PC1XX_GPE0CON),
	SAVE_ITEM(S5PC1XX_GPE0DAT),
	SAVE_ITEM(S5PC1XX_GPE0PUD),
	SAVE_ITEM(S5PC1XX_GPE1CON),
	SAVE_ITEM(S5PC1XX_GPE1DAT),
	SAVE_ITEM(S5PC1XX_GPE1PUD),
	SAVE_ITEM(S5PC1XX_GPF0CON),
	SAVE_ITEM(S5PC1XX_GPF0DAT),
	SAVE_ITEM(S5PC1XX_GPF0PUD),
	SAVE_ITEM(S5PC1XX_GPF1CON),
	SAVE_ITEM(S5PC1XX_GPF1DAT),
	SAVE_ITEM(S5PC1XX_GPF1PUD),
	SAVE_ITEM(S5PC1XX_GPF2CON),
	SAVE_ITEM(S5PC1XX_GPF2DAT),
	SAVE_ITEM(S5PC1XX_GPF2PUD),
	SAVE_ITEM(S5PC1XX_GPF3CON),
	SAVE_ITEM(S5PC1XX_GPF3DAT),
	SAVE_ITEM(S5PC1XX_GPF3PUD),
	SAVE_ITEM(S5PC1XX_GPG0CON),
	SAVE_ITEM(S5PC1XX_GPG0DAT),
	SAVE_ITEM(S5PC1XX_GPG0PUD),
	SAVE_ITEM(S5PC1XX_GPG1CON),
	SAVE_ITEM(S5PC1XX_GPG1DAT),
	SAVE_ITEM(S5PC1XX_GPG1PUD),
	SAVE_ITEM(S5PC1XX_GPG2CON),
	SAVE_ITEM(S5PC1XX_GPG2DAT),
	SAVE_ITEM(S5PC1XX_GPG2PUD),
	SAVE_ITEM(S5PC1XX_GPG3CON),
	SAVE_ITEM(S5PC1XX_GPG3DAT),
	SAVE_ITEM(S5PC1XX_GPG3PUD),
	SAVE_ITEM(S5PC1XX_GPH0CON),
	SAVE_ITEM(S5PC1XX_GPH0DAT),
	SAVE_ITEM(S5PC1XX_GPH0PUD),
	SAVE_ITEM(S5PC1XX_GPH1CON),
	SAVE_ITEM(S5PC1XX_GPH1DAT),
	SAVE_ITEM(S5PC1XX_GPH1PUD),
	SAVE_ITEM(S5PC1XX_GPH2CON),
	SAVE_ITEM(S5PC1XX_GPH2DAT),
	SAVE_ITEM(S5PC1XX_GPH2PUD),
	SAVE_ITEM(S5PC1XX_GPH3CON),
	SAVE_ITEM(S5PC1XX_GPH3DAT),
	SAVE_ITEM(S5PC1XX_GPH3PUD),
	SAVE_ITEM(S5PC1XX_GPICON),
	SAVE_ITEM(S5PC1XX_GPIDAT),
	SAVE_ITEM(S5PC1XX_GPIPUD),
	SAVE_ITEM(S5PC1XX_GPJ0CON),
	SAVE_ITEM(S5PC1XX_GPJ0DAT),
	SAVE_ITEM(S5PC1XX_GPJ0PUD),
	SAVE_ITEM(S5PC1XX_GPJ1CON),
	SAVE_ITEM(S5PC1XX_GPJ1DAT),
	SAVE_ITEM(S5PC1XX_GPJ1PUD),
	SAVE_ITEM(S5PC1XX_GPJ2CON),
	SAVE_ITEM(S5PC1XX_GPJ2DAT),
	SAVE_ITEM(S5PC1XX_GPJ2PUD),
	SAVE_ITEM(S5PC1XX_GPJ3CON),
	SAVE_ITEM(S5PC1XX_GPJ3DAT),
	SAVE_ITEM(S5PC1XX_GPJ3PUD),
	SAVE_ITEM(S5PC1XX_GPK0CON),
	SAVE_ITEM(S5PC1XX_GPK0DAT),
	SAVE_ITEM(S5PC1XX_GPK0PUD),
	SAVE_ITEM(S5PC1XX_GPK1CON),
	SAVE_ITEM(S5PC1XX_GPK1DAT),
	SAVE_ITEM(S5PC1XX_GPK1PUD),
	SAVE_ITEM(S5PC1XX_GPK2CON),
	SAVE_ITEM(S5PC1XX_GPK2DAT),
	SAVE_ITEM(S5PC1XX_GPK2PUD),
	SAVE_ITEM(S5PC1XX_GPK3CON),
	SAVE_ITEM(S5PC1XX_GPK3DAT),
	SAVE_ITEM(S5PC1XX_GPK3PUD),
};

/* this lot should be really saved by the IRQ code */
/* VICXADDRESSXX initilaization to be needed */
static struct sleep_save irq_save[] = {
	SAVE_ITEM(S5PC100_VIC0REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC100_VIC1REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC100_VIC2REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC100_VIC0REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC100_VIC1REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC100_VIC2REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC100_VIC0REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC100_VIC1REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC100_VIC2REG(VIC_INT_SOFT)),
};

static struct sleep_save eint_save[] = {
	SAVE_ITEM(S5PC1XX_EINT0CON),
	SAVE_ITEM(S5PC1XX_EINT1CON),
	SAVE_ITEM(S5PC1XX_EINT2CON),
	SAVE_ITEM(S5PC1XX_EINT0MASK),
	SAVE_ITEM(S5PC1XX_EINT1MASK),
	SAVE_ITEM(S5PC1XX_EINT2MASK),
};

static struct sleep_save sromc_save[] = {
	SAVE_ITEM(S5PC1XX_SROM_BW),
	SAVE_ITEM(S5PC1XX_SROM_BC0),
	SAVE_ITEM(S5PC1XX_SROM_BC1),
	SAVE_ITEM(S5PC1XX_SROM_BC2),
	SAVE_ITEM(S5PC1XX_SROM_BC3),
	SAVE_ITEM(S5PC1XX_SROM_BC4),
	SAVE_ITEM(S5PC1XX_SROM_BC5),
};

/* NAND control registers */
#define PM_NFCONF             (S3C_VA_NAND + 0x00)        
#define PM_NFCONT             (S3C_VA_NAND + 0x04)        

static struct sleep_save nand_save[] = {
        SAVE_ITEM(PM_NFCONF),
        SAVE_ITEM(PM_NFCONT),
};

#define SAVE_UART(va) \
	SAVE_ITEM((va) + S3C2410_ULCON), \
	SAVE_ITEM((va) + S3C2410_UCON), \
	SAVE_ITEM((va) + S3C2410_UFCON), \
	SAVE_ITEM((va) + S3C2410_UMCON), \
	SAVE_ITEM((va) + S3C2410_UBRDIV), \
	SAVE_ITEM((va) + S3C2410_UDIVSLOT), \
	SAVE_ITEM((va) + S3C2410_UINTMSK)


static struct sleep_save uart_save[] = {
	SAVE_UART(S3C24XX_VA_UART0),
};

#define DBG(fmt...)

#define s5pc1xx_pm_debug_init() do { } while(0)
#define s5pc1xx_pm_check_prepare() do { } while(0)
#define s5pc1xx_pm_check_restore() do { } while(0)
#define s5pc1xx_pm_check_store()   do { } while(0)

/* helper functions to save and restore register state */

void s5pc1xx_pm_do_save(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		ptr->val = __raw_readl(ptr->reg);
		//DBG("saved %p value %08lx\n", ptr->reg, ptr->val);
	}
}

/* s5pc1xx_pm_do_restore
 *
 * restore the system from the given list of saved registers
 *
 * Note, we do not use DBG() in here, as the system may not have
 * restore the UARTs state yet
*/

void s5pc1xx_pm_do_restore(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		//printk(KERN_DEBUG "restore %p (restore %08lx, was %08x)\n",
		       //ptr->reg, ptr->val, __raw_readl(ptr->reg));

		__raw_writel(ptr->val, ptr->reg);
	}
}

/* s5pc1xx_pm_do_restore_core
 *
 * similar to s36410_pm_do_restore_core
 *
 * WARNING: Do not put any debug in here that may effect memory or use
 * peripherals, as things may be changing!
*/

/* s5pc1xx_pm_do_save_phy
 *
 * save register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

void s5pc1xx_pm_do_save_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		ptr->val = readl(target_reg + (ptr->reg));
	}
}

/* s5pc1xx_pm_do_restore_phy
 *
 * restore register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

void s5pc1xx_pm_do_restore_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		writel(ptr->val, (target_reg + ptr->reg));
	}
}

static void s5pc1xx_pm_do_restore_core(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		__raw_writel(ptr->val, ptr->reg);
	}
}

/* s5pc1xx_pm_show_resume_irqs
 *
 * print any IRQs asserted at resume time (ie, we woke from)
*/

static void s5pc1xx_pm_show_resume_irqs(int start, unsigned long which,
					unsigned long mask)
{
	int i;

	which &= ~mask;

	for (i = 0; i <= 31; i++) {
		if ((which) & (1L<<i)) {
			DBG("IRQ %d asserted at resume\n", start+i);
		}
	}
}

static void s5pc1xx_pm_configure_extint(void)
{
/* for each of the external interrupts (EINT0..EINT15) we
 * need to check wether it is an external interrupt source,
 * and then configure it as an input if it is not
 * And SMDKC100 has two External Interrupt Switch EINT11(GPH1_3) and EINT31(GPH3_7)
 * So System can wake up with both External interrupt source.
 */

	u32 tmp;

	/* Mask all External Interrupt */
	writel(0xff , weint_base + S5P_APM_WEINT0_MASK);
	writel(0xfb , weint_base + S5P_APM_WEINT1_MASK);
	writel(0xff , weint_base + S5P_APM_WEINT2_MASK);
	writel(0xff , weint_base + S5P_APM_WEINT3_MASK);

	/* Clear all External Interrupt Pending */
	writel(0xff , weint_base + S5P_APM_WEINT0_PEND);
	writel(0xff , weint_base + S5P_APM_WEINT1_PEND);
	writel(0xff , weint_base + S5P_APM_WEINT2_PEND);
	writel(0xff , weint_base + S5P_APM_WEINT3_PEND);

	tmp = readl(S5P_EINT_WAKEUP_MASK);
	tmp = ~((1 << 11) | (1 << 31));
	writel(tmp , S5P_EINT_WAKEUP_MASK);
}

void (*pm_cpu_prep)(void);
void (*pm_cpu_sleep)(void);

#define any_allowed(mask, allow) (((mask) & (allow)) != (allow))

static int s5pc1xx_pm_clk(enum PLL_TYPE pm_pll,u32 mdiv, u32 pdiv, u32 sdiv)
{
	u32 pll_value;
	u32 pll_addr;

	pll_value = (1 << 31) | (mdiv << 16) | (pdiv << 8) | (sdiv << 0);

	switch(pm_pll)
	{
		case PM_APLL:
			pll_addr = S5P_APLL_CON;
		case PM_MPLL:
			pll_addr = S5P_MPLL_CON;
		case PM_EPLL:
			pll_addr = S5P_EPLL_CON;
		case PM_HPLL:
			pll_addr = S5P_HPLL_CON;
	}

	writel(pll_value , pll_addr);

	while(!((readl(pll_addr) >> 30) & 0x1)){}
}

/* s5pc1xx_pm_enter
 *
 * central control for sleep/resume process
*/

static int s5pc1xx_pm_enter(suspend_state_t state)
{
	unsigned long regs_save[16];
	unsigned int tmp;

	/* ensure the debug is initialised (if enabled) */

	DBG("s5pc1xx_pm_enter(%d)\n", state);

	if (pm_cpu_prep == NULL || pm_cpu_sleep == NULL) {
		printk(KERN_ERR PFX "error: no cpu sleep functions set\n");
		return -EINVAL;
	}

	/* store the physical address of the register recovery block */
	s5pc100_sleep_save_phys = virt_to_phys(regs_save);

	DBG("s5pc1xx_sleep_save_phys=0x%08lx\n", s5pc100_sleep_save_phys);

	s5pc1xx_pm_do_save(gpio_save, ARRAY_SIZE(gpio_save));
	s5pc1xx_pm_do_save(irq_save, ARRAY_SIZE(irq_save));
	s5pc1xx_pm_do_save(core_save, ARRAY_SIZE(core_save));
	s5pc1xx_pm_do_save(sromc_save, ARRAY_SIZE(sromc_save));
	s5pc1xx_pm_do_save(nand_save, ARRAY_SIZE(nand_save));
	s5pc1xx_pm_do_save(uart_save, ARRAY_SIZE(uart_save));
	s5pc1xx_pm_do_save(eint_save, ARRAY_SIZE(eint_save));


	/* ensure INF_REG0  has the resume address */
	__raw_writel(virt_to_phys(s5pc100_cpu_resume), S5P_INFORM0);

	/* call cpu specific preperation */

	pm_cpu_prep();

	/* flush cache back to ram */
	flush_cache_all();

	/* send the cpu to sleep... */
	__raw_writel(0xffffffff, S5PC100_VIC0REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC100_VIC1REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC100_VIC2REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC100_VIC0REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC100_VIC1REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC100_VIC2REG(VIC_INT_SOFT_CLEAR));

	/* Mask all wake up source */
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= ~(0x1 << 7);
	tmp |= (0x7ff << 8);
	/* unmask alarm wakeup source */
	tmp &= ~(0x1 << 10);
	__raw_writel(tmp , S5P_PWR_CFG);
	__raw_writel(0xffffffff , S5P_EINT_WAKEUP_MASK);

	/* Wake up source setting */
	s5pc1xx_pm_configure_extint();

	/* : USB Power Control */
	/*   - USB PHY Disable */
	/*   - Make USB Tranceiver PAD to Suspend */
	tmp = __raw_readl(S5P_OTHERS);
   	tmp &= ~(1<<16);           	/* USB Signal Mask Clear */
   	__raw_writel(tmp, S5P_OTHERS);

	tmp = __raw_readl(S5PC1XX_UHOST);
	tmp |= (1<<0);
	__raw_writel(tmp, S5PC1XX_UHOST);

	/* Sleep Mode Pad Configuration */
	__raw_writel(0x2, S5PC1XX_PDNEN); /* Controlled by SLPEN Bit (You Should Clear SLPEN Bit in Wake Up Process...) */
    
	/* Set WFI instruction to SLEEP mode */
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	tmp |= S5P_CFG_WFI_SLEEP;
	__raw_writel(tmp, S5P_PWR_CFG);

	/* Clear WAKEUP_STAT register for next wakeup */
	tmp = __raw_readl(S5P_WAKEUP_STAT);
	__raw_writel(tmp, S5P_WAKEUP_STAT);

	/* Set Power Stable Count */
	tmp = __raw_readl(S5P_OTHERS);
	tmp &=~(1 << S5P_OTHER_STA_TYPE);
	tmp |= (STA_TYPE_SFR << S5P_OTHER_STA_TYPE);
	__raw_writel(tmp , S5P_OTHERS);
	
	__raw_writel(((S5P_PWR_STABLE_COUNT << S5P_PWR_STA_CNT) | (1 << S5P_PWR_STA_EXP_SCALE)), S5P_PWR_STABLE);

	/* Set Syscon Interrupt */
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= (1 << S5P_OTHER_SYS_INT);
	__raw_writel(tmp, S5P_OTHERS);

	/* Disable OSC_EN (Disable X-tal Osc Pad in Sleep mode) */
	tmp = __raw_readl(S5P_SLEEP_CFG);
	tmp &= ~(1 << 0);
	__raw_writel(tmp, S5P_SLEEP_CFG);

	/* s5pc1xx_cpu_save will also act as our return point from when
	 * we resume as it saves its own register state, so use the return
	 * code to differentiate return from save and return from sleep */

	if (s5pc100_cpu_save(regs_save) == 0) {
		flush_cache_all();
		/* This function for Chip bug on EVT0 */
		pm_cpu_sleep();
	}

	/* restore the cpu state */
	cpu_init();

	/* Sleep Mode Pad Configuration */
    	__raw_writel(0x2, S5PC1XX_PDNEN);	/* Clear SLPEN Bit for Pad back to Normal Mode */

	/* MTC IO OFF |  MTC IO SD-MMC OFF | USB Phy Enable */
	tmp = __raw_readl(S5P_OTHERS);
   	tmp |= (1<<31);
	__raw_writel(tmp, S5P_OTHERS);

	tmp = __raw_readl(S5P_OTHERS);
   	tmp |= ((1<<22)|(1<<16));
	__raw_writel(tmp, S5P_OTHERS);

	tmp = __raw_readl(S5PC1XX_UHOST);
	tmp &= ~(1<<0);
	__raw_writel(tmp, S5PC1XX_UHOST);

	
	s5pc1xx_pm_do_restore(gpio_save, ARRAY_SIZE(gpio_save));
	s5pc1xx_pm_do_restore(irq_save, ARRAY_SIZE(irq_save));
	s5pc1xx_pm_do_restore(core_save, ARRAY_SIZE(core_save));
	s5pc1xx_pm_do_restore(sromc_save, ARRAY_SIZE(sromc_save));
	s5pc1xx_pm_do_restore(nand_save, ARRAY_SIZE(nand_save));
	s5pc1xx_pm_do_restore(uart_save, ARRAY_SIZE(uart_save));
	s5pc1xx_pm_do_restore(eint_save, ARRAY_SIZE(eint_save));

	tmp = readl(weint_base + S5P_APM_WEINT1_PEND);
	writel(tmp , weint_base + S5P_APM_WEINT1_PEND);

	DBG("post sleep, preparing to return\n");

	s5pc1xx_pm_check_restore();

	/* ok, let's return from sleep */
	DBG("S3C6410 PM Resume (post-restore)\n");
	return 0;
}


static struct platform_suspend_ops s5pc1xx_pm_ops = {
	.enter		= s5pc1xx_pm_enter,
	.valid		= suspend_valid_only_mem,
};

/* s5pc1xx_pm_init
 *
 * Attach the power management functions. This should be called
 * from the board specific initialisation if the board supports
 * it.
*/

int __init s5pc1xx_pm_init(void)
{
	printk("s5pc1xx Power Management, (c) 2008 Samsung Electronics\n");

	weint_base = ioremap(S5P_APM_BASE, 0x350);

	/* set the irq configuration for wake */
	suspend_set_ops(&s5pc1xx_pm_ops);
	return 0;
}
