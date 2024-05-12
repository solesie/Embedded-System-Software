obj-m := dev_driver.o
dev_driver-objs := timer_driver.o ioctl_timer_ctrl.o fpga_ctrl.o

KDIR := /work/achroimx_kernel
PWD := $(shell pwd)

default: 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=arm

push:
	adb push dev_driver.ko /data/local/tmp

clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *.order
	rm -rf *.symvers
	rm -rf .*.cmd
	rm -rf .tmp_versions
