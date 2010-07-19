ifndef PRJROOT
    $(error Cannot locate project environment)
endif

MODULES_DPRAM=y
MODULES_WLAN=y
MODULES_MULTIMEDIA=y
MODULES_FMRADIO=y
MODULES_VIBETONZ=y

.PHONY: clean install

ifeq ($(MODULES_DPRAM),y)
subdir-y += dpram
subdir-y += multipdp
endif
ifeq ($(MODULES_CAMERA),y)
subdir-y += camera
endif
ifeq ($(MODULES_WLAN),y)
subdir-y += bcm4329
endif
ifeq ($(MODULES_MULTIMEDIA),y)
subdir-y += pvrsrvkm
subdir-y += s3clcd
subdir-y += s3cbc
endif
ifeq ($(MODULES_VIBETONZ),y)
subdir-y += vibetonz
endif
ifeq ($(MODULES_BTGPIO),y)
subdir-y += btgpio
endif
ifeq ($(MODULES_FMRADIO),y)
subdir-y += fm_si4709
endif

SUBDIRS := $(subdir-y)

DO_INSTALL_MODS = y

all: $(SUBDIRS)

$(SUBDIRS)::
	@$(MAKE) -C $@

install: $(SUBDIRS)
ifeq ($(strip $(DO_INSTALL_MODS)),y)
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) --no-print-directory -C $$i $@ > /dev/null 2>&1; \
	done
endif
	@$(MAKE) --no-print-directory -C $(KDIR) modules_install

clean:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@; \
	done
