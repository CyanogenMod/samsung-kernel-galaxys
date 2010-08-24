.PHONY: clean install

obj-$(CONFIG_MODULES_DROP_CACHES) += drop_caches/
obj-$(CONFIG_MODULES_DPRAM) += dpram/
obj-$(CONFIG_MODULES_CAMERA) += camera/
obj-$(CONFIG_MODULES_WLAN) += bcm4329/

ifeq ($(CONFIG_MODULES_MULTIMEDIA),y)
obj-y += pvrsrvkm/
obj-y += s3clcd/
obj-y += s3cbc/
endif

ifeq ($(CONFIG_FLASH_ONENAND),y)
obj-y += dpram/fsr/
#obj-y += rfs/
#obj-y += j4fs/
endif

obj-$(CONFIG_MODULES_VIBETONZ) += vibetonz
obj-$(CONFIG_MODULES_ELED) += eled
obj-$(CONFIG_MODULES_BTGPIO) += btgpio
obj-$(CONFIG_MODULES_FMRADIO) += fm_si4709
obj-$(CONFIG_MODULES_PARAM) += param