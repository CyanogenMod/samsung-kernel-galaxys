# arch/arm/mach-s5pc110/Makefile
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

# Core support for S5PC110 system

obj-$(CONFIG_CPU_S5PC110)	+= cpu.o
obj-$(CONFIG_CPU_S5PC110)	+= dma.o

# Helper and device support
obj-$(CONFIG_PM)    += pm.o
obj-$(CONFIG_S5PC11X_SETUP_SDHCI)       += setup-sdhci.o

# machine support

obj-$(CONFIG_MACH_SMDKC110)	+= mach-smdkc110.o
obj-$(CONFIG_MACH_SMDKC110)	+= button-smdkc110.o

# LEDs support
led-$(CONFIG_MACH_SMDKC110)     += leds-s5pc110.o
#obj-$(CONFIG_LEDS)              += $(led-y)


obj-$(CONFIG_MACH_S5PC110_P1P2)	+= mach-p1p2.o

obj-$(CONFIG_MACH_S5PC110_JUPITER)	+= mach-jupiter.o	jupiter-rfkill.o   mach-jupiter-gps.o

#sec headset support
obj-$(CONFIG_SEC_HEADSET)	+= sec_jack.o
# LEDs support


obj-$(CONFIG_LEDS)              += $(led-y)

