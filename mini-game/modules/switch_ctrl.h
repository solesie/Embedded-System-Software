#ifndef SWITCH_CTRL_H
#define SWITCH_CTRL_H

enum direction{
	NONE = 0,
	UP,
	LEFT,
	RIGHT,
	DOWN
};
void push_switch_init(void);
enum direction push_switch_read(void);
void push_switch_exit(void);

#endif // SWITCH_CTRL_H