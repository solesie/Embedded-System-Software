#include <linux/io.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "fpga_fnd.h"
#include "logging.h"

#define LED_MAX 8
#define IOM_LED_ADDRESS 0x08000016 // pysical address

static unsigned char *iom_fpga_led_addr;
static int data;

static void print_data(void){
	const unsigned short value = 1 << (LED_MAX- data);
    outw(value, (unsigned int) iom_fpga_led_addr);
}

void led_init(char timer_init[FND_MAX + 1]){
	int i;
	iom_fpga_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
	for(i = 0; i < FND_MAX; ++i) if(timer_init[i] != '0') data = timer_init[i] - '0';
	print_data();
}

void led_increase(void){
	++data;
	if(data > LED_MAX) data = 1;
	print_data();
}

void led_del(void){
	outw(0, (unsigned int) iom_fpga_led_addr);
	iounmap(iom_fpga_led_addr);
}
