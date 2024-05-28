obj-m := stopwatch.o
stopwatch-objs := stopwatch_driver.o stopwatch_ctrl.o

KDIR := /work/achroimx_kernel
PWD := $(shell pwd)

default: 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=arm

push:
	adb push stopwatch.ko /data/local/tmp

clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *.order
	rm -rf *.symvers
	rm -rf .*.cmd
	rm -rf .tmp_versions
