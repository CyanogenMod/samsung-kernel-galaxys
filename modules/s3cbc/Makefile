##############################################################################
# COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2009-2014 ALL RIGHTS RESERVED
# AUTHOR		: mk93.lee@samsung.com
##############################################################################
# VERSION&DATE	: Version 1.00	2009/10/21 (mk93.lee)
##############################################################################


EXTRA_CFLAGS += -I$(PRJROOT)/modules/include


obj-m := s3c_bc.o

s3c_bc-objs := s3cbc_module.o

all:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean:
	rm -f *.ko *.mod.c *.mod.o *~ .*.cmd .*.o.cmd Module.symvers modules.order $(obj-m)

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install

