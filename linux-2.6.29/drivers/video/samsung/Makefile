#
# Makefile for the s3c framebuffer driver
#

ifeq ($(CONFIG_FB_S3C),y)
obj-y				+= s3cfb.o
obj-$(CONFIG_PLAT_S3C64XX)	+= s3cfb_fimd4x.o
obj-$(CONFIG_PLAT_S5P64XX)	+= s3cfb_fimd5x.o
obj-$(CONFIG_CPU_S5PC100)	+= s3cfb_fimd5x.o
obj-$(CONFIG_CPU_S5PC110)	+= s3cfb_fimd6x.o
obj-$(CONFIG_FB_S3C_LTE480WV)	+= s3cfb_lte480wv.o
obj-$(CONFIG_FB_S3C_TL2796)     += s3cfb_tl2796.o
obj-$(CONFIG_FB_S3C_LVDS)       += s3cfb_lvds.o
obj-$(CONFIG_FB_S3C_MDNIE)      += s3cfb_mdnie.o s3cfb_ielcd.o
endif

ifeq ($(CONFIG_FB_S3C_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

