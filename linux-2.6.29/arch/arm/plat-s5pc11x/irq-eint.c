/* arch/arm/plat-s5pc11x/irq-eint.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5PC11X - Interrupt handling for IRQ_EINT(x)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <asm/hardware/vic.h>

#include <plat/regs-irqtype.h>

#include <mach/map.h>
#include <plat/cpu.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>


#define eint_offset(irq)		((irq) < IRQ_EINT16_31 ? ((irq)-IRQ_EINT0) :  \
					(irq-S3C_IRQ_EINT_BASE))
					
#define eint_irq_to_bit(irq)		(1 << (eint_offset(irq) & 0x7))

#define eint_conf_reg(irq)		((eint_offset(irq)) >> 3)
#define eint_filt_reg(irq)		((eint_offset(irq)) >> 2)
#define eint_mask_reg(irq)		((eint_offset(irq)) >> 3)
#define eint_pend_reg(irq)		((eint_offset(irq)) >> 3)

static inline void s3c_irq_eint_mask(unsigned int irq)
{
	u32 mask;

	mask = __raw_readl(S5PC11X_EINTMASK(eint_mask_reg(irq)));
	mask |= eint_irq_to_bit(irq);
	__raw_writel(mask, S5PC11X_EINTMASK(eint_mask_reg(irq)));
#ifdef  S5PC11X_ALIVEGPIO_STORE
	mask = __raw_readl(S5PC11X_EINTMASK(eint_mask_reg(irq)));
#endif
}

static void s3c_irq_eint_unmask(unsigned int irq)
{
	u32 mask;

	mask = __raw_readl(S5PC11X_EINTMASK(eint_mask_reg(irq)));
	mask &= ~(eint_irq_to_bit(irq));
	__raw_writel(mask, S5PC11X_EINTMASK(eint_mask_reg(irq)));
#ifdef  S5PC11X_ALIVEGPIO_STORE
	mask = __raw_readl(S5PC11X_EINTMASK(eint_mask_reg(irq)));
#endif
}

static inline void s3c_irq_eint_ack(unsigned int irq)
{
	__raw_writel(eint_irq_to_bit(irq), S5PC11X_EINTPEND(eint_pend_reg(irq)));
#ifdef  S5PC11X_ALIVEGPIO_STORE
	u32 pending;
	pending = __raw_readl(S5PC11X_EINTPEND(eint_pend_reg(irq)));
#endif
}

static void s3c_irq_eint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_eint_mask(irq);
	s3c_irq_eint_ack(irq);
}

static int s3c_irq_eint_set_type(unsigned int irq, unsigned int type)
{
	int offs = eint_offset(irq);
	int shift;
	u32 ctrl, mask;
	u32 newvalue = 0;

	switch (type) {
	case IRQ_TYPE_NONE:
		printk(KERN_WARNING "No edge setting!\n");
		break;

	case IRQ_TYPE_EDGE_RISING:
		newvalue = S5P_EXTINT_RISEEDGE;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		newvalue = S5P_EXTINT_FALLEDGE;
		break;

	case IRQ_TYPE_EDGE_BOTH:
		newvalue = S5P_EXTINT_BOTHEDGE;
		break;

	case IRQ_TYPE_LEVEL_LOW:
		newvalue = S5P_EXTINT_LOWLEV;
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		newvalue = S5P_EXTINT_HILEV;
		break;

	default:
		printk(KERN_ERR "No such irq type %d", type);
		return -1;
	}

	shift = (offs & 0x7) * 4;
	mask = 0x7 << shift;

	ctrl = __raw_readl(S5PC11X_EINTCON(eint_conf_reg(irq)));
	ctrl &= ~mask;
	ctrl |= newvalue << shift;
	__raw_writel(ctrl, S5PC11X_EINTCON(eint_conf_reg(irq)));
#ifdef  S5PC11X_ALIVEGPIO_STORE
	ctrl = __raw_readl(S5PC11X_EINTCON(eint_conf_reg(irq)));
#endif

	if((0 <= offs) && (offs < 8))
		s3c_gpio_cfgpin(S5PC11X_GPH0(offs&0x7), 0xf<<((offs&0x7)*4));
	else if((8 <= offs) && (offs < 16))
		s3c_gpio_cfgpin(S5PC11X_GPH1(offs&0x7), 0xf<<((offs&0x7)*4));
	else if((16 <= offs) && (offs < 24))
		s3c_gpio_cfgpin(S5PC11X_GPH2(offs&0x7), 0xf<<((offs&0x7)*4));
	else if((24 <= offs) && (offs < 32))
		s3c_gpio_cfgpin(S5PC11X_GPH3(offs&0x7), 0xf<<((offs&0x7)*4));
	else
		printk(KERN_ERR "No such irq number %d", offs);

	return 0;
}


static struct irq_chip s3c_irq_eint = {
	.name		= "s3c-eint",
	.mask		= s3c_irq_eint_mask,
	.unmask		= s3c_irq_eint_unmask,
	.mask_ack	= s3c_irq_eint_maskack,
	.ack		= s3c_irq_eint_ack,
	.set_type	= s3c_irq_eint_set_type,
};

/* s3c_irq_demux_eint
 *
 * This function demuxes the IRQ from the group0 external interrupts,
 * from IRQ_EINT(16) to IRQ_EINT(31). It is designed to be inlined into
 * the specific handlers s3c_irq_demux_eintX_Y.
 */
static inline void s3c_irq_demux_eint(unsigned int start, unsigned int end)
{
	u32 status = __raw_readl(S5PC11X_EINTPEND((start >> 3)));
	u32 mask = __raw_readl(S5PC11X_EINTMASK((start >> 3)));
	unsigned int irq;

	status &= ~mask;
	status &= (1 << (end - start + 1)) - 1;

	for (irq = IRQ_EINT(start); irq <= IRQ_EINT(end); irq++) {
		if (status & 1)
			generic_handle_irq(irq);

		status >>= 1;
	}
}

static void s3c_irq_demux_eint16_31(unsigned int irq, struct irq_desc *desc)
{
	s3c_irq_demux_eint(16, 23);
	s3c_irq_demux_eint(24, 31);
}

/*---------------------------- EINT0 ~ EINT15 -------------------------------------*/
static void s3c_irq_vic_eint_mask(unsigned int irq)
{
	void __iomem *base = get_irq_chip_data(irq);
	
	s3c_irq_eint_mask(irq);
	
	irq &= 31;
	writel(1 << irq, base + VIC_INT_ENABLE_CLEAR);
}


static void s3c_irq_vic_eint_unmask(unsigned int irq)
{
	void __iomem *base = get_irq_chip_data(irq);
	
	s3c_irq_eint_unmask(irq);
	
	irq &= 31;
	writel(1 << irq, base + VIC_INT_ENABLE);
}


static inline void s3c_irq_vic_eint_ack(unsigned int irq)
{
	__raw_writel(eint_irq_to_bit(irq), S5PC11X_EINTPEND(eint_pend_reg(irq)));
#ifdef  S5PC11X_ALIVEGPIO_STORE
	u32 pending;
	pending = __raw_readl(S5PC11X_EINTPEND(eint_pend_reg(irq)));
#endif
}


static void s3c_irq_vic_eint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_vic_eint_mask(irq);
	s3c_irq_vic_eint_ack(irq);
}


static struct irq_chip s3c_irq_vic_eint = {
	.name	= "s3c_vic_eint",
	.mask	= s3c_irq_vic_eint_mask,
	.unmask	= s3c_irq_vic_eint_unmask,
	.mask_ack = s3c_irq_vic_eint_maskack,
	.ack = s3c_irq_vic_eint_ack,
	.set_type = s3c_irq_eint_set_type,
};


int __init s5pc11x_init_irq_eint(void)
{
	int irq;

	for (irq = IRQ_EINT0; irq <= IRQ_EINT15; irq++) {
		set_irq_chip(irq, &s3c_irq_vic_eint);
	}

	for (irq = IRQ_EINT(16); irq <= IRQ_EINT(31); irq++) {
		set_irq_chip(irq, &s3c_irq_eint);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	set_irq_chained_handler(IRQ_EINT16_31, s3c_irq_demux_eint16_31);
	return 0;
}

arch_initcall(s5pc11x_init_irq_eint);

#define S5PC11X_GPIOINT_CONBASE		S5PC11X_GPIOREG(0x700)	
#define S5PC11X_GPIOINT_FLTBASE		S5PC11X_GPIOREG(0x800)	
#define S5PC11X_GPIOINT_MASKBASE	S5PC11X_GPIOREG(0x900)	
#define S5PC11X_GPIOINT_PENDBASE	S5PC11X_GPIOREG(0xa00)	
#define S5PC11X_GPIOINT_SERVICE		S5PC11X_GPIOREG(0xB08)	
#define S5PC11X_GPIOINT_SERVICEPEND	S5PC11X_GPIOREG(0xB0C)


struct s3c_gpioint_cfg {
	unsigned int irq_start;	
	unsigned int gpio_start;	
	unsigned int irq_nums;	
	unsigned int reg_offset;	
};

#define gpioint_irq_to_bit(irq,cfg)		(1 << (irq - cfg->irq_start))
#define  S3C_GPIOINT_MAXCFGS          22

#define gpio_print(...)

struct s3c_gpioint_cfg s3c_gpioint_info[S3C_GPIOINT_MAXCFGS] = 
	{
		{
			.reg_offset  = 0x0,
			.irq_start   = S3C_IRQ_GPIO_A0_INT_BASE,
			.gpio_start  = S5PC11X_GPA0(0),
			.irq_nums    = S5PC11X_GPIO_A0_INTS,
		},
		{
			.reg_offset  = 0x4,
			.irq_start   = S3C_IRQ_GPIO_A1_INT_BASE,
			.gpio_start  = S5PC11X_GPA1(0),
			.irq_nums    = S5PC11X_GPIO_A1_INTS,
		},
		{
			.reg_offset  = 0x8,
			.irq_start   = S3C_IRQ_GPIO_B_INT_BASE,
			.gpio_start  = S5PC11X_GPB(0),
			.irq_nums    = S5PC11X_GPIO_B_INTS,
		},
		{
			.reg_offset  = 0xc,
			.irq_start   = S3C_IRQ_GPIO_C0_INT_BASE,
			.gpio_start  = S5PC11X_GPC0(0),
			.irq_nums    = S5PC11X_GPIO_C0_INTS,
		},
		{
			.reg_offset  = 0x10,
			.irq_start   = S3C_IRQ_GPIO_C1_INT_BASE,
			.gpio_start  = S5PC11X_GPC1(0),
			.irq_nums    = S5PC11X_GPIO_C1_INTS,
		},
		{
			.reg_offset  = 0x14,
			.irq_start   = S3C_IRQ_GPIO_D0_INT_BASE,
			.gpio_start  = S5PC11X_GPD0(0),
			.irq_nums    = S5PC11X_GPIO_D0_INTS,
		},
		{
			.reg_offset  = 0x18,
			.irq_start   = S3C_IRQ_GPIO_D1_INT_BASE,
			.gpio_start  = S5PC11X_GPD1(0),
			.irq_nums    = S5PC11X_GPIO_D1_INTS,
		},
		{
			.reg_offset  = 0x1c,
			.irq_start   = S3C_IRQ_GPIO_E0_INT_BASE,
			.gpio_start  = S5PC11X_GPE0(0),
			.irq_nums    = S5PC11X_GPIO_E0_INTS,
		},
		{
			.reg_offset  = 0x20,
			.irq_start   = S3C_IRQ_GPIO_E1_INT_BASE,
			.gpio_start  = S5PC11X_GPE1(0),
			.irq_nums    = S5PC11X_GPIO_E1_INTS,
		},
		{
			.reg_offset  = 0x24,
			.irq_start   = S3C_IRQ_GPIO_F0_INT_BASE,
			.gpio_start  = S5PC11X_GPF0(0),
			.irq_nums    = S5PC11X_GPIO_F0_INTS,
		},
		{
			.reg_offset  = 0x28,
			.irq_start   = S3C_IRQ_GPIO_F1_INT_BASE,
			.gpio_start  = S5PC11X_GPF1(0),
			.irq_nums    = S5PC11X_GPIO_F1_INTS,
		},
		{
			.reg_offset  = 0x2c,
			.irq_start   = S3C_IRQ_GPIO_F2_INT_BASE,
			.gpio_start  = S5PC11X_GPF2(0),
			.irq_nums    = S5PC11X_GPIO_F2_INTS,
		},
		{
			.reg_offset  = 0x30,
			.irq_start   = S3C_IRQ_GPIO_F3_INT_BASE,
			.gpio_start  = S5PC11X_GPF3(0),
			.irq_nums    = S5PC11X_GPIO_F3_INTS,
		},
		{
			.reg_offset  = 0x34,
			.irq_start   = S3C_IRQ_GPIO_G0_INT_BASE,
			.gpio_start  = S5PC11X_GPG0(0),
			.irq_nums    = S5PC11X_GPIO_G0_INTS,
		},
		{
			.reg_offset  = 0x38,
			.irq_start   = S3C_IRQ_GPIO_G1_INT_BASE,
			.gpio_start  = S5PC11X_GPG1(0),
			.irq_nums    = S5PC11X_GPIO_G1_INTS,
		},
		{
			.reg_offset  = 0x3c,
			.irq_start   = S3C_IRQ_GPIO_G2_INT_BASE,
			.gpio_start  = S5PC11X_GPG2(0),
			.irq_nums    = S5PC11X_GPIO_G2_INTS,
		},
		{
			.reg_offset  = 0x40,
			.irq_start   = S3C_IRQ_GPIO_G3_INT_BASE,
			.gpio_start  = S5PC11X_GPG3(0),
			.irq_nums    = S5PC11X_GPIO_G3_INTS,
		},
		{
			.reg_offset  = 0x44,
			.irq_start   = S3C_IRQ_GPIO_J0_INT_BASE,
			.gpio_start  = S5PC11X_GPJ0(0),
			.irq_nums    = S5PC11X_GPIO_J0_INTS,
		},
		{
			.reg_offset  = 0x48,
			.irq_start   = S3C_IRQ_GPIO_J1_INT_BASE,
			.gpio_start  = S5PC11X_GPJ1(0),
			.irq_nums    = S5PC11X_GPIO_J1_INTS,
		},
		{
			.reg_offset  = 0x4c,
			.irq_start   = S3C_IRQ_GPIO_J2_INT_BASE,
			.gpio_start  = S5PC11X_GPJ2(0),
			.irq_nums    = S5PC11X_GPIO_J2_INTS,
		},
		{
			.reg_offset  = 0x50,
			.irq_start   = S3C_IRQ_GPIO_J3_INT_BASE,
			.gpio_start  = S5PC11X_GPJ3(0),
			.irq_nums    = S5PC11X_GPIO_J3_INTS,
		},
		{
			.reg_offset  = 0x54,
			.irq_start   = S3C_IRQ_GPIO_J4_INT_BASE,
			.gpio_start  = S5PC11X_GPJ4(0),
			.irq_nums    = S5PC11X_GPIO_J4_INTS,
		},
	

	};


static struct s3c_gpioint_cfg *s3c_gpioint_cfgs[S3C_IRQ_GPIO_INT_NUMS];

static struct s3c_gpioint_cfg* s3c_irq_gpioint_tocfg(unsigned int irq)
{
	unsigned int i;	
	struct s3c_gpioint_cfg *cfg;
	
	for(i = 0; i < S3C_GPIOINT_MAXCFGS; i++) {
		cfg = &s3c_gpioint_info[i];

		if(irq < (cfg->irq_start + cfg->irq_nums))
			return cfg;
	}
		 
		
	return NULL;

}

static inline struct s3c_gpioint_cfg* s3c_irq_gpioint_getcfg(unsigned int irq)
{
	return s3c_gpioint_cfgs[irq - S3C_IRQ_GPIOINT_BASE];

}

static inline void s3c_irq_gpioint_mask(unsigned int irq)
{
	u32 mask;
	void __iomem *reg;
	struct s3c_gpioint_cfg *cfg;

	cfg = s3c_irq_gpioint_getcfg(irq);

        reg = S5PC11X_GPIOINT_MASKBASE + cfg->reg_offset;


	mask = __raw_readl(reg);
	mask |= gpioint_irq_to_bit(irq,cfg);
	__raw_writel(mask, reg);
	gpio_print("mask irq %d, mask  %x, off %d,reg %x\n",irq,mask,irq - cfg->irq_start,reg);
}

static void s3c_irq_gpioint_unmask(unsigned int irq)
{
	u32 mask;
	void __iomem *reg;
	struct s3c_gpioint_cfg *cfg;

	cfg = s3c_irq_gpioint_getcfg(irq);
        reg = S5PC11X_GPIOINT_MASKBASE + cfg->reg_offset;

	mask = __raw_readl(reg);
	mask &= ~(gpioint_irq_to_bit(irq,cfg));
	__raw_writel(mask, reg);
	gpio_print("unmask irq %d, mask  %x, off %d,reg %x\n",irq,mask,irq - cfg->irq_start,reg);

}

static inline void s3c_irq_gpioint_ack(unsigned int irq)
{
	struct s3c_gpioint_cfg *cfg;
	void __iomem *reg;

	cfg = s3c_irq_gpioint_getcfg(irq);
        reg = S5PC11X_GPIOINT_PENDBASE + cfg->reg_offset;

	__raw_writel(gpioint_irq_to_bit(irq,cfg),reg);
}

static void s3c_irq_gpioint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_gpioint_mask(irq);
	s3c_irq_gpioint_ack(irq);
}

static int s3c_irq_gpioint_set_type(unsigned int irq, unsigned int type)
{
	int offs;
	int shift;
	u32 ctrl, mask;
	u32 newvalue = 0;
	struct s3c_gpioint_cfg *cfg;
	void __iomem *reg;

	if( (irq > S3C_IRQ_GPIOINT_END) || (irq < S3C_IRQ_GPIOINT_BASE))
		__WARN_printf("GPIO Interrupt number is not valid \n");


	cfg = s3c_irq_gpioint_getcfg(irq);

	switch (type) {
	case IRQ_TYPE_NONE:
		printk(KERN_WARNING "No edge setting!\n");
		break;

	case IRQ_TYPE_EDGE_RISING:
		newvalue = S5P_EXTINT_RISEEDGE;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		newvalue = S5P_EXTINT_FALLEDGE;
		break;

	case IRQ_TYPE_EDGE_BOTH:
		newvalue = S5P_EXTINT_BOTHEDGE;
		break;

	case IRQ_TYPE_LEVEL_LOW:
		newvalue = S5P_EXTINT_LOWLEV;
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		newvalue = S5P_EXTINT_HILEV;
		break;

	default:
		printk(KERN_ERR "No such irq type %d", type);
		return -1;
	}

	offs = irq - cfg->irq_start;

	shift = (offs & 0x7) * 4;
	mask = 0x7 << shift;

        reg = S5PC11X_GPIOINT_CONBASE + cfg->reg_offset;

	s3c_gpio_cfgpin(cfg->gpio_start + offs, S3C_GPIO_SFN(0xf));

	ctrl = __raw_readl(reg);
	ctrl &= ~mask;
	ctrl |= newvalue << shift;
	__raw_writel(ctrl, reg);

	gpio_print("type irq %d, ctrl  %x, off %d,reg %x gpioreg %x \n",
		irq,ctrl,offs,reg, readl(S5PC11X_GPIOREG(0x240)));

	return 0;
}

static void s3c_irq_demux_gpioint(unsigned int gpioirq, struct irq_desc *desc)
{

	u32 status = __raw_readl(S5PC11X_GPIOINT_SERVICE);
	u32 off,index ;
	unsigned int irq;

	off = status & 0x7;
	index = ((status >> 3) & 0x1f) -1; 

	irq = s3c_gpioint_info[index].irq_start + off;
	
	gpio_print("demux irq %d, index %d, off %d,stat %x\n",irq,index,off,status);
 
	generic_handle_irq(irq);


}


static struct irq_chip s3c_irq_gpioint = {
	.name		= "s3c-gpioint",
	.mask		= s3c_irq_gpioint_mask,
	.unmask		= s3c_irq_gpioint_unmask,
	.mask_ack	= s3c_irq_gpioint_maskack,
	.ack		= s3c_irq_gpioint_ack,
	.set_type	= s3c_irq_gpioint_set_type,
};


int __init s5pc11x_init_irq_gpioint(void)
{
	int irq;
	struct s3c_gpioint_cfg *cfg;

	for (irq = S3C_IRQ_GPIOINT_BASE; irq <= S3C_IRQ_GPIOINT_END; irq++) {
		cfg = s3c_irq_gpioint_tocfg(irq);
		s3c_gpioint_cfgs[irq - S3C_IRQ_GPIOINT_BASE] = cfg;
		set_irq_chip(irq, &s3c_irq_gpioint);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	set_irq_chained_handler(IRQ_GPIOINT, s3c_irq_demux_gpioint);
	return 0;
}

arch_initcall(s5pc11x_init_irq_gpioint);
