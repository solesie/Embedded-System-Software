#include <linux/io.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "fpga_fnd.h"
#include "logging.h"

#define IOM_FND_ADDRESS 0x08000004 	// pysical address

static unsigned char *iom_fpga_fnd_addr;
static char data[FND_MAX];
static char pivot;

static void print_data(void){
	unsigned short data_short = data[0] << 12 | data[1] << 8 |data[2] << 4 |data[3];
	outw(data_short,(unsigned int)iom_fpga_fnd_addr);
}

void fnd_init(char timer_init[FND_MAX + 1]){
	int i;
	iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);

	for(i = 0; i < FND_MAX; ++i) if(timer_init[i] != '0') pivot = timer_init[i];

	strcpy(data, timer_init);
	print_data();
}

void fnd_increase(void){
	int i;
	for(i = 0; i < FND_MAX; ++i){
		if(data[i] != '0'){
			++data[i];
			if(data[i] == pivot){
				memset(data, '0', sizeof(data));
				i = (i + 1) % FND_MAX;
				data[i] = pivot;
			}
			break;
		}
	}
	print_data();
}

void fnd_del(void){
	memset(data, '0', sizeof(data));
	print_data();
	iounmap(iom_fpga_fnd_addr);
}
