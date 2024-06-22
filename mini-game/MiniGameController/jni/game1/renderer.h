#ifndef GAME1_RENDERER_H
#define GAME1_RENDERER_H

#include <android/native_window.h> 
#include <android/native_window_jni.h>

void game1_create(void);
void game1_destroy(void);
void game1_del_surface(void);
void game1_set_surface(ANativeWindow*);
void game1_resume(void);
void game1_pause(void);
void game1_restart(void);
bool game1_wait_back_interrupt(void);
void game1_start(void);
void game1_stop(void);

#endif // GAME1_RENDERER_H