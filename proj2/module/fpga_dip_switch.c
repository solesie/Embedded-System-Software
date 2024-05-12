#include <linux/io.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "logging.h"

#define IOM_FPGA_DIP_SWITCH_ADDRESS 0x08000000			// pysical address

static unsigned char *iom_fpga_dip_switch_addr;

void dip_switch_init(void){
	iom_fpga_dip_switch_addr = ioremap(IOM_FPGA_DIP_SWITCH_ADDRESS, 0x1);
}

/* 1: pressed, 0: not pressed */
int dip_switch_read(void){
	unsigned char dip_sw_value;	
	unsigned short _s_dip_sw_value;

	_s_dip_sw_value = inw((unsigned int)iom_fpga_dip_switch_addr);
	dip_sw_value = _s_dip_sw_value & 0xFF;

	if(dip_sw_value == 0) return 1;
	return 0;
}

void dip_switch_del(void){
	iounmap(iom_fpga_dip_switch_addr);
}
