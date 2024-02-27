ifneq ($(KERNELRELEASE),)
# kbuild part of makefil


obj-m:= adxl345.o

else
# normal makefile

KDIR ?= /lib/modules/`uname -r`/build
default:
	@echo $(KDIR)
	@echo "PWD=$(PWD)"
	$(MAKE) -C $(KDIR) M=$(PWD)
endif
