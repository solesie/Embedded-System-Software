#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/string.h>
#include "switch_ctrl.h"
#include "logging.h"

#define IOM_LED_ADDRESS 0x08000016
#define LED_MAX 8

static int led;

static unsigned char *iom_fpga_led_addr;

static void fpga_print_led(void){
	unsigned short _s_value = 0;
	int i;
	for(i = 1; i <= led; ++i)
		_s_value |= (1 << (LED_MAX - i));
	outw(_s_value, (unsigned int)iom_fpga_led_addr);
}

void led_init(void){
	// init MMIO
	iom_fpga_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
    led = 0;
}

void led_set(int _led){
    if(_led >= 0 && _led < LED_MAX)
    led = _led;
    fpga_print_led();
}

void led_exit(void){
	led = 0;
	fpga_print_led();
	iounmap(iom_fpga_led_addr);
}