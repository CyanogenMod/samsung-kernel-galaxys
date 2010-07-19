# arch/arm/mach-s5pc100/Makefile
#
# Copyright 2008 Openmoko, Inc.
# Copyright 2008 Simtec Electronics
#
# Licensed under GPLv2

obj-y				:=
led-y                           := leds.o
obj-m				:=
obj-n				:=
obj-				:=

# Core support for S5PC100 system

obj-$(CONFIG_CPU_S5PC100)	+= cpu.o
obj-$(CONFIG_CPU_S5PC100)	+= dma.o

# Helper and device support
obj-$(CONFIG_PM)    += pm.o
obj-$(CONFIG_S5PC1XX_SETUP_SDHCI)       += setup-sdhci.o

# machine support

obj-$(CONFIG_MACH_SMDKC100)	+= mach-smdkc100.o

# LEDs support
led-$(CONFIG_MACH_SMDKC100)     += leds-s5pc100.o
obj-$(CONFIG_LEDS)              += $(led-y)
