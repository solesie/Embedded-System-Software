obj-m := minigame.o
minigame-objs := driver.o switch_ctrl.o timer_ctrl.o led_ctrl.o text_lcd_ctrl.o interrupt_ctrl.o

KDIR := /work/achroimx_kernel
PWD := $(shell pwd)

default: 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=arm

push:
	adb push minigame.ko /data/local/tmp

clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *.order
	rm -rf *.symvers
	rm -rf .*.cmd
	rm -rf .tmp_versions
