/* linux/arch/arm/mach-s3c6410/setup-sdhci.c
 *
 * Copyright 2008 Simtec Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C6410 - Helper functions for settign up SDHCI device(s) (HSMMC)
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

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-sdhci.h>
#include <plat/sdhci.h>
#include <mach/map.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-g2.h>


/* clock sources for the mmc bus clock, order as for the ctrl2[5..4] */
char *s3c6410_hsmmc_clksrcs[4] = {
	[0] = "mmc_bus",
	[1] = "mmc_bus",
	[2] = "hsmmc",
};

void s3c6410_setup_sdhci0_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

        /* Channel 0 supports 1,4 and 8-bit bus width */
        end = S5PC1XX_GPG0(2 + width);

        /* Set all the necessary GPG0 ins to special-function 2 */
        for (gpio = S5PC1XX_GPG0(0); gpio < end; gpio++) {
                s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
                s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
        }

        /* GPG1 chip Detect */
        s3c_gpio_setpull(S5PC1XX_GPG1(2), S3C_GPIO_PULL_UP);
        s3c_gpio_cfgpin(S5PC1XX_GPG1(2), S3C_GPIO_SFN(2));
        writel(0xf, S5PC1XX_GPG0DRV);
}

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

	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

void s3c6410_setup_sdhci1_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

	/* Channel 1 supports 1 and 4-bit bus width */
	end = S5PC1XX_GPG2(2 + width);

	/* Set all the necessary GPG2 pins to special-function 2 */
	for (gpio = S5PC1XX_GPG2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	/* GPG2 chip Detect */
	s3c_gpio_setpull(S5PC1XX_GPG2(6), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(S5PC1XX_GPG2(6), S3C_GPIO_SFN(2));
}

void s3c6410_setup_sdhci2_cfg_gpio(struct platform_device *dev, int width)
{
        unsigned int gpio;
        unsigned int end;

        /* Channel 1 supports 1 and 4-bit bus width */
        end = S5PC1XX_GPG3(2 + width);

        /* Set all the necessary GPG3 pins to special-function 2 */
        for (gpio = S5PC1XX_GPG3(0); gpio < end; gpio++) {
                s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
                s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
        }

        /* GPG3 chip Detect */
        s3c_gpio_setpull(S5PC1XX_GPG3(6), S3C_GPIO_PULL_UP);
        s3c_gpio_cfgpin(S5PC1XX_GPG3(6), S3C_GPIO_SFN(2));
}
