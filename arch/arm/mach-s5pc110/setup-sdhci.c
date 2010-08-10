/* linux/arch/arm/mach-s5pc110/setup-sdhci.c
 *
 * Copyright 2008 Simtec Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5PC110 - Helper functions for settign up SDHCI device(s) (HSMMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-sdhci.h>
#include <plat/sdhci.h>
#include <mach/map.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-g2.h>
#include <plat/gpio-bank-h3.h>

#include <plat/regs-gpio.h>

extern unsigned int HWREV;

/* clock sources for the mmc bus clock, order as for the ctrl2[5..4] */
char *s3c6410_hsmmc_clksrcs[4] = {
	[0] = "hsmmc",
	[1] = "hsmmc",
	[2] = "mmc_bus",
};

void s3c6410_setup_sdhci0_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

	if(width < 4)
		width = 4;

	/* Channel 0 supports 1,4 and 8-bit bus width */
	end = S5PC11X_GPG0(3 + width);

	/* Set all the necessary GPG0 ins to special-function 2 */
	for (gpio = S5PC11X_GPG0(0); gpio < end; gpio++) {
		if(gpio != S5PC11X_GPG0(2)) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
	}

	writel(0x2aaa, S5PC11X_GPG0DRV);	

	/* Chip detect pin Pull up*/
	s3c_gpio_setpull(S5PC11X_GPG0(2), S3C_GPIO_PULL_NONE);

#if !(defined CONFIG_JUPITER_VER_B4) 
	s3c_gpio_cfgpin(S5PC11X_GPJ2(7), S3C_GPIO_OUTPUT);
    	s3c_gpio_setpull(S5PC11X_GPJ2(7), S3C_GPIO_PULL_NONE);
    	gpio_set_value(S5PC11X_GPJ2(7), 1);
#endif	
}
	
#define S3C_SDHCI_CTRL3_FCSELTX_INVERT  (0)
#define S3C_SDHCI_CTRL3_FCSELTX_BASIC   (S3C_SDHCI_CTRL3_FCSEL3 | S3C_SDHCI_CTRL3_FCSEL2)
#define S3C_SDHCI_CTRL3_FCSELRX_INVERT  (0)
#define S3C_SDHCI_CTRL3_FCSELRX_BASIC   (S3C_SDHCI_CTRL3_FCSEL1 | S3C_SDHCI_CTRL3_FCSEL0)

void s3c6410_setup_sdhci0_cfg_card(struct platform_device *dev,
				    void __iomem *r,
				    struct mmc_ios *ios,
				    struct mmc_card *card)
{
	u32 ctrl2, ctrl3 = 0;

	/* don't need to alter anything acording to card-type */

	writel(S3C64XX_SDHCI_CONTROL4_DRIVE_9mA, r + S3C64XX_SDHCI_CONTROL4);

	/* No need for any delay values in the HS-MMC interface */
	ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
	ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
		  S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
		  S3C_SDHCI_CTRL2_DFCNT_NONE |
		  S3C_SDHCI_CTRL2_ENCLKOUTHOLD);

	if (ios->clock <= (400 * 1000)) {
		ctrl2 &= ~(S3C_SDHCI_CTRL2_ENFBCLKTX | S3C_SDHCI_CTRL2_ENFBCLKRX);
		ctrl3 = 0;
	} else {
		u32 range_start;
		u32 range_end;

		ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKTX | S3C_SDHCI_CTRL2_ENFBCLKRX;

		if (card->type & MMC_TYPE_MMC)	/* MMC */
			range_start = 20 * 1000 * 1000;
		else	/* SD, SDIO */
			range_start = 25 * 1000 * 1000;

		range_end = 37 * 1000 * 1000;

		if ((ios->clock > range_start) && (ios->clock < range_end))
			ctrl3 = S3C_SDHCI_CTRL3_FCSELTX_BASIC |
				S3C_SDHCI_CTRL3_FCSELRX_BASIC;
		else
			ctrl3 = S3C_SDHCI_CTRL3_FCSELTX_BASIC |
				S3C_SDHCI_CTRL3_FCSELRX_INVERT;
	}

	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

void s3c6410_setup_sdhci1_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;
	
	if(width < 4)
		width = 4;

	/* Channel 1 supports 1 and 4-bit bus width */
	end = S5PC11X_GPG1(3 + width);

	/* Set all the necessary GPG1 pins to special-function 2 */
	for (gpio = S5PC11X_GPG1(0); gpio < end; gpio++) {
	//WLAN_nRST  set pull_none and low
		if (gpio != S5PC11X_GPG1(2)){
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
	}
	writel(0x2aaa, S5PC11X_GPG1DRV);	

	//WLAN_nRST(S5PC11X_GPG1(2)) set pull_none and low
	/* Chip detect pin Pull up*/
	s3c_gpio_setpull(S5PC11X_GPG1(2), S3C_GPIO_PULL_NONE);
}

void s3c6410_setup_sdhci2_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

	if(width < 4)
		width = 4;

	/* Channel 2 supports 1 and 4-bit bus width */
	end = S5PC11X_GPG2(3 + width);

	/* Set all the necessary GPG2 pins to special-function 2 */
	for (gpio = S5PC11X_GPG2(0); gpio < end; gpio++) {
		if(gpio != S5PC11X_GPG2(2)) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
	}

	writel(0x2aaa, S5PC11X_GPG2DRV);	

	/* Chip detect pin Pull up*/
	s3c_gpio_setpull(S5PC11X_GPG2(2), S3C_GPIO_PULL_NONE);
}

void s3c6410_setup_sdhci3_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

	if(width < 4)
		width = 4;

	/* Channel 3 supports 1 and 4-bit bus width */
	end = S5PC11X_GPG3(3 + width);

	/* Set all the necessary GPG3 pins to special-function 2 */
	for (gpio = S5PC11X_GPG3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	writel(0x2aaa, S5PC11X_GPG3DRV);	

	/* Chip detect pin Pull up*/
	s3c_gpio_setpull(S5PC11X_GPG3(2), S3C_GPIO_PULL_NONE);
}

#if defined (CONFIG_MACH_S5PC110_JUPITER) || defined(CONFIG_MACH_S5PC110_P1P2)
unsigned int universal_sdhci2_detect_ext_cd(void)
{
	unsigned int card_status = 0;

#ifdef CONFIG_MMC_DEBUG
	printk(" Universal :SD Detect function \n");
	printk("eint conf %x  eint filter conf %x",readl(S5PC11X_EINT3CON),
			readl(S5PC11X_EINT3FLTCON1));
	printk("eint pend %x  eint mask %x",readl(S5PC11X_EINT3PEND),
			readl(S5PC11X_EINT3MASK));
#endif
#if defined(CONFIG_MACH_S5PC110_P1P2)
	card_status = (readl(S5PC11X_GPH2DAT)) & (1 << 4);
	printk(KERN_DEBUG " Universal : Card status  %d\n",card_status?0:1);
	return card_status ? 0 : 1;
	
#else
	card_status = (readl(S5PC11X_GPH3DAT)) & (1 << 4);
	printk(KERN_DEBUG " Universal : Card status  %d\n",card_status?1:0);
#if 1 // Behold3 and Kepler
	return card_status ? 0 : 1;
#else
	if(((HWREV >= 7) || (HWREV == 0x3)) && (HWREV !=8))
	{
		return card_status ? 0 : 1;
	}
	else
	{
		return card_status ? 1 : 0;
	}
#endif
#endif
}

void universal_sdhci2_cfg_ext_cd(void)
{
	int err;

	printk(" Universal :SD Detect configuration \n");

#if defined(CONFIG_MACH_S5PC110_P1P2)
	err = gpio_request(S5PC11X_GPH2(4),"GPH24");

	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		//s3c_gpio_setpull(S5PC11X_GPH2(4), S3C_GPIO_PULL_UP);
	}
	set_irq_type(IRQ_EINT(20), IRQ_TYPE_EDGE_BOTH);
#else
	err = gpio_request(S5PC11X_GPH3(4),"GPH34");

	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		//s3c_gpio_cfgpin(S5PC11X_GPH3(4),S5PC11X_GPH3_4_EXT_INT33_4);
		//s3c_gpio_setpull(S5PC11X_GPH3(4), S3C_GPIO_PULL_DOWN);
	}
#if 1 // Behold3 and Kepler
	s3c_gpio_setpull(S5PC11X_GPH3(4), S3C_GPIO_PULL_UP);
#else
	if(((HWREV >= 7) || (HWREV == 0x3)) && (HWREV !=8))
	{
		s3c_gpio_setpull(S5PC11X_GPH3(4), S3C_GPIO_PULL_UP);
	}
#endif	
	set_irq_type(IRQ_EINT(28), IRQ_TYPE_EDGE_BOTH);
#endif
}

#ifdef CONFIG_S3C_DEV_HSMMC2
void universal_sdhci2_set_platdata(void)
{
	printk(" Universal :SD Setting platform Data \n");
#if defined(CONFIG_MACH_S5PC110_P1P2)
	s3c_hsmmc2_def_platdata.ext_cd = IRQ_EINT(20);
#else
	s3c_hsmmc2_def_platdata.ext_cd = IRQ_EINT(28);
#endif
	s3c_hsmmc2_def_platdata.cfg_ext_cd =universal_sdhci2_cfg_ext_cd;
	s3c_hsmmc2_def_platdata.detect_ext_cd = universal_sdhci2_detect_ext_cd;
}
#endif

#endif // CONFIG_MACH_S5PC110_JUPITER

