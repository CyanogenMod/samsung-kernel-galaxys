# arch/arm/plat-s5pc1xx/Makefile
#
# Copyright 2008 Openmoko, Inc.
# Copyright 2008 Simtec Electronics
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:= dummy.o
obj-				:=

# Core files

obj-y				+= dev-uart.o devs.o
obj-y				+= cpu.o
obj-y				+= s5pc1xx-time.o
obj-y				+= irq.o
obj-y				+= irq-eint.o
obj-y				+= clock.o
obj-y				+= gpiolib.o
obj-y				+= bootmem.o

# CPU support

obj-$(CONFIG_CPU_S5PC100_INIT)	+= s5pc100-init.o
obj-$(CONFIG_CPU_S5PC100_CLOCK)	+= s5pc100-clock.o changediv.o
obj-$(CONFIG_PM)                += pm.o
obj-$(CONFIG_PM)                += sleep.o
obj-$(CONFIG_CPU_FREQ)		+= s5pc1xx-cpufreq.o ltc3714.o

# Device setup
obj-$(CONFIG_S5PC1XX_SETUP_I2C0) += setup-i2c0.o
obj-$(CONFIG_S5PC1XX_SETUP_I2C1) += setup-i2c1.o
obj-$(CONFIG_S5PC1XX_ADC)        += adc.o
obj-$(CONFIG_HAVE_PWM)		 += pwm.o

obj-$(CONFIG_SPI_S5PC100)   	 += dev-spi.o

# Device setup - FIMC
obj-$(CONFIG_S5PC1XX_DEV_FIMC0)	  += dev-fimc0.o
obj-$(CONFIG_S5PC1XX_DEV_FIMC1)	  += dev-fimc1.o
obj-$(CONFIG_S5PC1XX_DEV_FIMC2)	  += dev-fimc2.o
obj-$(CONFIG_S5PC1XX_SETUP_FIMC0) += setup-fimc0.o
obj-$(CONFIG_S5PC1XX_SETUP_FIMC1) += setup-fimc1.o
obj-$(CONFIG_S5PC1XX_SETUP_FIMC2) += setup-fimc2.o

# Device setup - FIMD
obj-$(CONFIG_S5PC1XX_DEV_FB)	  += dev-fb.o

# Device setup - MIPI-CSI2
obj-$(CONFIG_S5PC1XX_DEV_CSIS)	  += dev-csis.o
obj-$(CONFIG_S5PC1XX_SETUP_CSIS)  += setup-csis.o

