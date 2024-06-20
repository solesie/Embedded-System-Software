#ifndef GAME2_RENDERER_H
#define GAME2_RENDERER_H

#include <android/native_window.h> 
#include <android/native_window_jni.h>

void game2_create(void);
void game2_destroy(void);
void game2_del_surface(void);
void game2_set_surface(ANativeWindow*);
void game2_resume(void);
void game2_pause(void);
void game2_restart(void);
bool game2_wait_back_interrupt(void);

#endif // GAME2_RENDERER_H