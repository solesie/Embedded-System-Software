#include <linux/kernel.h>
#include <linux/io.h>
#include "switch_ctrl.h"
#include "../logging.h"

#define MAX_BUTTON 9
#define IOM_FPGA_PUSH_SWITCH_ADDRESS 0x08000050

static unsigned char *iom_fpga_push_switch_addr;

void push_switch_init(void){
    // init MMIO
    iom_fpga_push_switch_addr = ioremap(IOM_FPGA_PUSH_SWITCH_ADDRESS, 0x18);
}

enum direction push_switch_read(void){
    int i;
    unsigned char push_sw_value[MAX_BUTTON];
    unsigned short int _s_value;
    enum direction ret = NONE;

    for(i = 0; i < MAX_BUTTON; ++i) {
        _s_value = inw((unsigned int)iom_fpga_push_switch_addr+i*2);
        push_sw_value[i] = _s_value &0xFF;
    }

    for(i = 0; i < MAX_BUTTON; ++i){
        // dip switch
        if(push_sw_value[i] != 1 && push_sw_value[i] != 0)
            return NONE;
        
        if(push_sw_value[i] == 1){
            if(ret != NONE) return NONE;
            if(i == 1) ret = UP;
            else if(i == 3) ret = LEFT;
            else if(i == 5) ret = RIGHT;
            else if(i == 7) ret = DOWN;
        }
    }
    return ret;
}

void push_switch_del(void){
    iounmap(iom_fpga_push_switch_addr);
}