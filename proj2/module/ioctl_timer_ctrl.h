#ifndef IOCTL_TIMER_CTRL_H
#define IOCTL_TIMER_CTRL_H

#include "fpga_ctrl.h"

void create_timer_ctrl(void);
void destroy_timer_ctrl(void);
int set_timer_ctrl(unsigned int timer_interval, unsigned int timer_cnt, char timer_init[FND_MAX + 1]);
int run_timer_ctrl(void);

#endif
