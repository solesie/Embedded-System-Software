#ifndef FPGA_CTRL_H
#define FPGA_CTRL_H

#define FND_MAX 4

void fpga_ioremap(void);
void fpga_iounmap(void);
void fpga_on(int _timer_cnt, int _countdown, char _timer_init[FND_MAX + 1]);
void fpga_off(void);
void fpga_set_countdown(void);
void fpga_countdown(void);
void fpga_increase(void);
void fpga_dip_switch_read_sync(void);

#endif