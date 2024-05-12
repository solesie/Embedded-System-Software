obj-m := dev_driver.o
dev_driver-objs := timer_driver.o timer.o fpga_led.o fpga_text_lcd.o fpga_dot.o fpga_fnd.o fpga_dip_switch.o

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
