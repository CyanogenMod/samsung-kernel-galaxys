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
# modified by james to support tickless timer
ifdef CONFIG_HIGH_RES_TIMERS
obj-y				+= hr-time.o
else
obj-y				+= s5pc11x-time.o
endif
obj-y				+= irq.o
obj-y				+= irq-eint.o
obj-y				+= clock.o
obj-y				+= gpiolib.o
obj-y				+= bootmem.o

# CPU support

obj-$(CONFIG_CPU_S5PC110_INIT)	+= s5pc110-init.o
obj-$(CONFIG_CPU_S5PC110_CLOCK)	+= s5pc110-clock.o changediv.o
obj-$(CONFIG_PM)                += pm.o
obj-$(CONFIG_PM)                += sleep.o
obj-$(CONFIG_REGULATOR_MAX8698) += max8698_consumer.o
obj-$(CONFIG_REGULATOR_MAX8998) += max8998_consumer.o
obj-$(CONFIG_CPU_FREQ)          += s5pc11x-dvfs.o

# Device setup
obj-$(CONFIG_S5PC11X_SETUP_I2C0) += setup-i2c0.o
obj-$(CONFIG_S5PC11X_SETUP_I2C1) += setup-i2c1.o
obj-$(CONFIG_S5PC11X_SETUP_I2C2) += setup-i2c2.o
obj-$(CONFIG_S5PC11X_ADC)        += adc.o
obj-$(CONFIG_HAVE_PWM)		 += pwm.o
obj-$(CONFIG_SPI_S5PC110)        += dev-spi.o

# Device setup - FIMC
obj-$(CONFIG_S5PC11X_DEV_FIMC0)	  += dev-fimc0.o
obj-$(CONFIG_S5PC11X_DEV_FIMC1)	  += dev-fimc1.o
obj-$(CONFIG_S5PC11X_DEV_FIMC2)	  += dev-fimc2.o
obj-$(CONFIG_S5PC11X_SETUP_FIMC0) += setup-fimc0.o
obj-$(CONFIG_S5PC11X_SETUP_FIMC1) += setup-fimc1.o
obj-$(CONFIG_S5PC11X_SETUP_FIMC2) += setup-fimc2.o

# Device setup - FIMD
obj-$(CONFIG_S5PC11X_DEV_FB)	  += dev-fb.o

# Device setup - MIPI-CSI2
obj-$(CONFIG_S5PC11X_DEV_CSIS)	  += dev-csis.o
obj-$(CONFIG_S5PC11X_SETUP_CSIS)  += setup-csis.o

# Device setup - IPC
obj-$(CONFIG_S5PC11X_DEV_IPC)	  += dev-ipc.o
obj-$(CONFIG_S5PC11X_SETUP_IPC)  += setup-ipc.o
