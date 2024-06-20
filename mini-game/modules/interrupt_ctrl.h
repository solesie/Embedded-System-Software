#ifndef INTERRUPT_CTRL_H
#define INTERRUPT_CTRL_H

int interrupt_wait_back_intr(void);
void interrupt_wake_back_waiting_thread(void);
void interrupt_init(void);
void interrupt_request(void);
void interrupt_free(void);

#endif // GAME1_INTERRUPT_CTRL_H