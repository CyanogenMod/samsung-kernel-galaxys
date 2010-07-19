/* arch/arm/plat-s5pc11x/gpiolib.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5PC11X - GPIOlib support 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/module.h>

#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/gpio-core.h>

#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>
#include <plat/regs-gpio.h>

#define OFF_GPCON	(0x00)
#define OFF_GPDAT	(0x04)

#define con_4bit_shift(__off) ((__off) * 4)

#define gpio_dbg(x...) do { } while(0)

/* The s5pc11x_gpiolib routines are to control the gpio banks where
 * the gpio configuration register (GPxCON) has 4 bits per GPIO, as the
 * following example:
 *
 * base + 0x00: Control register, 4 bits per gpio
 *	        gpio n: 4 bits starting at (4*n)
 *		0000 = input, 0001 = output, others mean special-function
 * base + 0x04: Data register, 1 bit per gpio
 *		bit n: data bit n
 *
 * Note, since the data register is one bit per gpio and is at base + 0x4
 * we can use s3c_gpiolib_get and s3c_gpiolib_set to change the state of
 * the output.
*/

static int s5pc11x_gpiolib_input(struct gpio_chip *chip, unsigned offset)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	unsigned long con;

	con = __raw_readl(base + OFF_GPCON);
	con &= ~(0xf << con_4bit_shift(offset));
	__raw_writel(con, base + OFF_GPCON);
#ifdef  S5PC11X_ALIVEGPIO_STORE
	con = __raw_readl(base + OFF_GPCON);
#endif

	gpio_dbg("%s: %p: CON now %08lx\n", __func__, base, con);

	return 0;
}

static int s5pc11x_gpiolib_output(struct gpio_chip *chip,
				       unsigned offset, int value)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	unsigned long con;
	unsigned long dat,tempdat;

	con = __raw_readl(base + OFF_GPCON);
	con &= ~(0xf << con_4bit_shift(offset));
	con |= 0x1 << con_4bit_shift(offset);

	dat = __raw_readl(base + OFF_GPDAT);
	if (value)
		dat |= 1 << offset;
	else
		dat &= ~(1 << offset);

	__raw_writel(dat, base + OFF_GPDAT);
#ifdef  S5PC11X_ALIVEGPIO_STORE
	tempdat = __raw_readl(base + OFF_GPDAT);
#endif

	__raw_writel(con, base + OFF_GPCON);
#ifdef  S5PC11X_ALIVEGPIO_STORE
	con = __raw_readl(base + OFF_GPCON);
#endif

	__raw_writel(dat, base + OFF_GPDAT);
#ifdef  S5PC11X_ALIVEGPIO_STORE
	dat = __raw_readl(base + OFF_GPDAT);
#endif

	gpio_dbg("%s: %p: CON %08lx, DAT %08lx\n", __func__, base, con, dat);

	return 0;
}

int s3c_gpio_slp_cfgpin(unsigned int pin, unsigned int config)
{
        struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
        void __iomem *reg;
        unsigned long flags;
        int offset;
        u32 con;
        int shift;

        if (!chip)
                return -EINVAL;

	if((pin <= S5PC11X_GPH3(7)) && (pin >= S5PC11X_GPH0(0))) {
                return -EINVAL;
        }

        if(config > S3C_GPIO_SLP_PREV)
        {
                 return -EINVAL;
        }

        reg = chip->base + 0x10;

        offset = pin - chip->chip.base;
        shift = offset * 2;

        local_irq_save(flags);

        con = __raw_readl(reg);
        con &= ~(3 << shift);
        con |= config << shift;
         __raw_writel(con, reg);

        local_irq_restore(flags);
        return 0;
}


s3c_gpio_pull_t s3c_gpio_get_slp_cfgpin(unsigned int pin)
{
        struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
        void __iomem *reg;
        unsigned long flags;
        int offset;
        u32 con;
        int shift;

        if (!chip)
                return -EINVAL;

	if((pin <= S5PC11X_GPH3(7)) && (pin >= S5PC11X_GPH0(0))) {
                return -EINVAL;
        }

        reg = chip->base + 0x10;

        offset = pin - chip->chip.base;
        shift = offset * 2;

        local_irq_save(flags);

        con = __raw_readl(reg);
        con >>= shift;
        con &= 0x3;

        local_irq_restore(flags);

        return (__force s3c_gpio_pull_t)con;
}
EXPORT_SYMBOL(s3c_gpio_slp_cfgpin);


int s3c_gpio_slp_setpull_updown(unsigned int pin, unsigned int config)
{
        struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
        void __iomem *reg;
        unsigned long flags;
        int offset;
        u32 con;
        int shift;

        if (!chip)
                return -EINVAL;

	if((pin <= S5PC11X_GPH3(7)) && (pin >= S5PC11X_GPH0(0))) {
                return -EINVAL;
        }

        if(config > S3C_GPIO_PULL_UP)
        {
                return -EINVAL;
        }
        reg = chip->base + 0x14;

        offset = pin - chip->chip.base;
        shift = offset * 2;

        local_irq_save(flags);

        con = __raw_readl(reg);
        con &= ~(3 << shift);
        con |= config << shift;
        __raw_writel(con, reg);

        local_irq_restore(flags);
                           
	return 0;
}
EXPORT_SYMBOL(s3c_gpio_slp_setpull_updown);


int s3c_gpio_set_drvstrength(unsigned int pin, unsigned int config)
{
        struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
        void __iomem *reg;
        unsigned long flags;
        int offset;
        u32 con;
        int shift;

        if (!chip)
                return -EINVAL;

        if(config  > S3C_GPIO_DRVSTR_4X)
        {
                return -EINVAL;
        }

        reg = chip->base + 0x0c;

        offset = pin - chip->chip.base;
        shift = offset * 2; 

        local_irq_save(flags);

        con = __raw_readl(reg);
        con &= ~(3 << shift);
        con |= config << shift;

        __raw_writel(con, reg);
#ifdef  S5PC11X_ALIVEGPIO_STORE
        con = __raw_readl(reg);
#endif

        local_irq_restore(flags);
                           
	return 0;
}

int s3c_gpio_set_slewrate(unsigned int pin, unsigned int config)
{
        struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
        void __iomem *reg;
        unsigned long flags;
        int offset;
        u32 con;
        int shift;

        if (!chip)
                return -EINVAL;

        if(config > S3C_GPIO_SLEWRATE_SLOW)
        {
                return -EINVAL;
        }

        reg = chip->base + 0x0c;

        offset = pin - chip->chip.base;
        shift = offset; 

        local_irq_save(flags);

        con = __raw_readl(reg);
        con &= ~(1<< shift);
        con |= config << shift;

        __raw_writel(con, reg);
#ifdef  S5PC11X_ALIVEGPIO_STORE
        con = __raw_readl(reg);
#endif

        local_irq_restore(flags);
                           
	return 0;
}

static struct s3c_gpio_cfg gpio_cfg = {
	.cfg_eint	= 0xf,
	.set_config	= s3c_gpio_setcfg_s5pc11x,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
        .set_pin        = s3c_gpio_setpin_updown
};

static struct s3c_gpio_cfg gpio_cfg_noint = {
	.set_config	= s3c_gpio_setcfg_s5pc11x,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
        .set_pin        = s3c_gpio_setpin_updown
};

static struct s3c_gpio_chip gpio_chips[] = {
	{
		.base	= S5PC11X_GPA0_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPA0(0),
			.ngpio	= S5PC11X_GPIO_A0_NR,
			.label	= "GPA0",
		},
	}, {
		.base	= S5PC11X_GPA1_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPA1(0),
			.ngpio	= S5PC11X_GPIO_A1_NR,
			.label	= "GPA1",
		},
	}, {
		.base	= S5PC11X_GPB_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPB(0),
			.ngpio	= S5PC11X_GPIO_B_NR,
			.label	= "GPB",
		},
	}, 
	{
		.base	= S5PC11X_GPC0_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPC0(0),
			.ngpio	= S5PC11X_GPIO_C0_NR,
			.label	= "GPC0",
		},
	}, {
		.base	= S5PC11X_GPC1_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPC1(0),
			.ngpio	= S5PC11X_GPIO_C1_NR,
			.label	= "GPC1",
		},
	}, {
		.base	= S5PC11X_GPD0_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPD0(0),
			.ngpio	= S5PC11X_GPIO_D0_NR,
			.label	= "GPD0",
		},
	}, {
		.base	= S5PC11X_GPD1_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPD1(0),
			.ngpio	= S5PC11X_GPIO_D1_NR,
			.label	= "GPD1",
		},
	}, 
	{
		.base	= S5PC11X_GPE0_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPE0(0),
			.ngpio	= S5PC11X_GPIO_E0_NR,
			.label	= "GPE0",
		},
	}, {
		.base	= S5PC11X_GPE1_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPE1(0),
			.ngpio	= S5PC11X_GPIO_E1_NR,
			.label	= "GPE1",
		},
	}, {
		.base	= S5PC11X_GPF0_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPF0(0),
			.ngpio	= S5PC11X_GPIO_F0_NR,
			.label	= "GPF0",
		},
	}, {
		.base	= S5PC11X_GPF1_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPF1(0),
			.ngpio	= S5PC11X_GPIO_F1_NR,
			.label	= "GPF1",
		},
	}, {
		.base	= S5PC11X_GPF2_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPF2(0),
			.ngpio	= S5PC11X_GPIO_F2_NR,
			.label	= "GPF2",
		},
	}, {
		.base	= S5PC11X_GPF3_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPF3(0),
			.ngpio	= S5PC11X_GPIO_F3_NR,
			.label	= "GPF3",
		},
	}, {
		.base	= S5PC11X_GPG0_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPG0(0),
			.ngpio	= S5PC11X_GPIO_G0_NR,
			.label	= "GPG0",
		},
	}, {
		.base	= S5PC11X_GPG1_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPG1(0),
			.ngpio	= S5PC11X_GPIO_G1_NR,
			.label	= "GPG1",
		},
	}, {
		.base	= S5PC11X_GPG2_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPG2(0),
			.ngpio	= S5PC11X_GPIO_G2_NR,
			.label	= "GPG2",
		},
	}, {
		.base	= S5PC11X_GPG3_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPG3(0),
			.ngpio	= S5PC11X_GPIO_G3_NR,
			.label	= "GPG3",
		},
	}, {
		.base	= S5PC11X_GPH0_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_GPH0(0),
			.ngpio	= S5PC11X_GPIO_H0_NR,
			.label	= "GPH0",
		},
	}, {
		.base	= S5PC11X_GPH1_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_GPH1(0),
			.ngpio	= S5PC11X_GPIO_H1_NR,
			.label	= "GPH1",
		},
	}, {
		.base	= S5PC11X_GPH2_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_GPH2(0),
			.ngpio	= S5PC11X_GPIO_H2_NR,
			.label	= "GPH2",
		},
	}, {
		.base	= S5PC11X_GPH3_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_GPH3(0),
			.ngpio	= S5PC11X_GPIO_H3_NR,
			.label	= "GPH3",
		},
	}, {
		.base	= S5PC11X_GPI_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPI(0),
			.ngpio	= S5PC11X_GPIO_I_NR,
			.label	= "GPI",
		},
	}, {
		.base	= S5PC11X_GPJ0_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPJ0(0),
			.ngpio	= S5PC11X_GPIO_J0_NR,
			.label	= "GPJ0",
		},
	}, {
		.base	= S5PC11X_GPJ1_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPJ1(0),
			.ngpio	= S5PC11X_GPIO_J1_NR,
			.label	= "GPJ1",
		},
	}, {
		.base	= S5PC11X_GPJ2_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPJ2(0),
			.ngpio	= S5PC11X_GPIO_J2_NR,
			.label	= "GPJ2",
		},
	}, {
		.base	= S5PC11X_GPJ3_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPJ3(0),
			.ngpio	= S5PC11X_GPIO_J3_NR,
			.label	= "GPJ3",
		},
	}, {
		.base	= S5PC11X_GPJ4_BASE,
		.config	= &gpio_cfg,
		.chip	= {
			.base	= S5PC11X_GPJ4(0),
			.ngpio	= S5PC11X_GPIO_J4_NR,
			.label	= "GPJ4",
		},
	}, 
	{
		.base	= S5PC11X_MP01_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP01(0),
			.ngpio	= S5PC11X_GPIO_MP01_NR,
			.label	= "MP01",
		},
	}, {
		.base	= S5PC11X_MP02_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP02(0),
			.ngpio	= S5PC11X_GPIO_MP02_NR,
			.label	= "MP02",
		},
	}, {
		.base	= S5PC11X_MP03_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP03(0),
			.ngpio	= S5PC11X_GPIO_MP03_NR,
			.label	= "MP03",
		},
	}, {
		.base	= S5PC11X_MP04_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP04(0),
			.ngpio	= S5PC11X_GPIO_MP04_NR,
			.label	= "MP04",
		},
	}, 
	{
		.base	= S5PC11X_MP05_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP05(0),
			.ngpio	= S5PC11X_GPIO_MP05_NR,
			.label	= "MP05",
		},
	}, {
		.base	= S5PC11X_MP06_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP06(0),
			.ngpio	= S5PC11X_GPIO_MP06_NR,
			.label	= "MP06",
		},
	}, {
		.base	= S5PC11X_MP07_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP07(0),
			.ngpio	= S5PC11X_GPIO_MP07_NR,
			.label	= "MP07",
		},
	}, {
		.base	= S5PC11X_MP10_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP10(0),
			.ngpio	= S5PC11X_GPIO_MP10_NR,
			.label	= "MP10",
		},
	}, {
		.base	= S5PC11X_MP11_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP11(0),
			.ngpio	= S5PC11X_GPIO_MP11_NR,
			.label	= "MP11",
		},
	}, {
		.base	= S5PC11X_MP12_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP12(0),
			.ngpio	= S5PC11X_GPIO_MP12_NR,
			.label	= "MP12",
		},
	}, {
		.base	= S5PC11X_MP13_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP13(0),
			.ngpio	= S5PC11X_GPIO_MP13_NR,
			.label	= "MP13",
		},
	}, {
		.base	= S5PC11X_MP14_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP14(0),
			.ngpio	= S5PC11X_GPIO_MP14_NR,
			.label	= "MP14",
		},
	}, {
		.base	= S5PC11X_MP15_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP15(0),
			.ngpio	= S5PC11X_GPIO_MP15_NR,
			.label	= "MP15",
		},
	}, {
		.base	= S5PC11X_MP16_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP16(0),
			.ngpio	= S5PC11X_GPIO_MP16_NR,
			.label	= "MP16",
		},
	}, {
		.base	= S5PC11X_MP17_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP17(0),
			.ngpio	= S5PC11X_GPIO_MP17_NR,
			.label	= "MP17",
		},
	}, {
		.base	= S5PC11X_MP18_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP18(0),
			.ngpio	= S5PC11X_GPIO_MP18_NR,
			.label	= "MP18",
		},
	}, {
		.base	= S5PC11X_MP20_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP20(0),
			.ngpio	= S5PC11X_GPIO_MP20_NR,
			.label	= "MP120",
		},
	}, {
		.base	= S5PC11X_MP21_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP21(0),
			.ngpio	= S5PC11X_GPIO_MP21_NR,
			.label	= "MP21",
		},
	}, {
		.base	= S5PC11X_MP22_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP22(0),
			.ngpio	= S5PC11X_GPIO_MP22_NR,
			.label	= "MP22",
		},
	}, {
		.base	= S5PC11X_MP23_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP23(0),
			.ngpio	= S5PC11X_GPIO_MP23_NR,
			.label	= "MP23",
		},
	}, {
		.base	= S5PC11X_MP24_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP24(0),
			.ngpio	= S5PC11X_GPIO_MP24_NR,
			.label	= "MP24",
		},
	}, {
		.base	= S5PC11X_MP25_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP25(0),
			.ngpio	= S5PC11X_GPIO_MP25_NR,
			.label	= "MP25",
		},
	}, {
		.base	= S5PC11X_MP26_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP26(0),
			.ngpio	= S5PC11X_GPIO_MP26_NR,
			.label	= "MP26",
		},
	}, {
		.base	= S5PC11X_MP27_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP27(0),
			.ngpio	= S5PC11X_GPIO_MP27_NR,
			.label	= "MP27",
		},
	}, {
		.base	= S5PC11X_MP28_BASE,
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PC11X_MP28(0),
			.ngpio	= S5PC11X_GPIO_MP28_NR,
			.label	= "MP28",
		},
	},
};

static __init void s5pc11x_gpiolib_link(struct s3c_gpio_chip *chip)
{
	chip->chip.direction_input = s5pc11x_gpiolib_input;
	chip->chip.direction_output = s5pc11x_gpiolib_output;
}

static __init void s5pc11x_gpiolib_add(struct s3c_gpio_chip *chips,
				       int nr_chips,
				       void (*fn)(struct s3c_gpio_chip *))
{
	for (; nr_chips > 0; nr_chips--, chips++) {
		if (fn)
			(fn)(chips);
		s3c_gpiolib_add(chips);
	}
}

__init int s5pc11x_gpiolib_init(void)
{
	s5pc11x_gpiolib_add(gpio_chips, ARRAY_SIZE(gpio_chips),
			    s5pc11x_gpiolib_link);

	return 0;
}
