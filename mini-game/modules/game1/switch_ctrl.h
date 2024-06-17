#ifndef GAME1_SWITCH_CTRL_H
#define GAME1_SWITCH_CTRL_H

enum direction{
    NONE = 0,
    UP,
    LEFT,
    RIGHT,
    DOWN
};
void push_switch_init(void);
enum direction push_switch_read(void);
void push_switch_del(void);

#endif // GAME1_SWITCH_CTRL_H