#
#   Makefile for eXtended Sector Remapper (FSR) driver
#
#   Copyright(c) 2004-2009, Samsung Electronics, Co., Ltd.
#

EXTRA_CFLAGS	+= -I$(TOPDIR)/drivers/tfsr/Inc \
		   -I$(TOPDIR)/drivers/tfsr

# For FSR 1.5 RC5 and below version
EXTRA_CFLAGS	+= -DFSR_LINUX_OAM 
EXTRA_CFLAGS	+= -DTINY_FSR 
#EXTRA_CFLAGS	+= -DFSR_OAM_RTLMSG_DISABLE 
EXTRA_CFLAGS	+= -DFSR_OAM_DBGMSG_ENABLE #-DFSR_OAM_ALL_DBGMSG
#EXTRA_CFLAGS	+= -DFSR_ASSERT
#EXTRA_CFLAGS	+= -DFSR_MAMMOTH_POWEROFF

ifeq ($(CONFIG_TINY_FSR),y)
EXTRA_CFLAGS    += 
endif

ifeq ($(CONFIG_ARM),y)
EXTRA_CFLAGS	+= -D__CC_ARM
endif


ifeq ($(CONFIG_RFS_FSR_DEBUG),y)
#EXTRA_CFLAGS	+= -D_RFS_INTERNAL_RESET
#EXTRA_CFLAGS	+= -D_RFS_INTERNAL_STAT_BH
endif

# Note: The following options are only used for development purpose
#	We don't guarantee these options on production
EXTRA_CFLAGS	+= -D__LINUSTOREIII_INTERNAL_PM__

# Kernel gcov
ifeq ($(CONFIG_GCOV_PROFILE),y)
ifeq ($(PATCHLEVEL),4)
include Makefile.gcov
else
include $(srctree)/drivers/tfsr/Makefile.gcov
endif
endif
ifeq ($(PATCHLEVEL),4)
include Makefile.24
else
include $(srctree)/drivers/tfsr/Makefile.26
endif
