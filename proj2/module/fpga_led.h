#ifndef FPGA_LED_H
#define FPGA_LED_H

#include "fpga_fnd.h"

void led_init(char timer_init[FND_MAX + 1]);
void led_increase(void);
void led_del(void);

#endif
