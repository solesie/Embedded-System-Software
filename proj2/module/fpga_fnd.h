#ifndef FPGA_FND_H
#define FPGA_FND_H

#define FND_MAX 4

void fnd_init(char timer_init[FND_MAX + 1]);
void fnd_increase(void);
void fnd_del(void);

#endif
