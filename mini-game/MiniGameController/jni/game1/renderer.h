#ifndef GAME1_RENDERER_H
#define GAME1_RENDERER_H

#include <android/native_window.h> 
#include <android/native_window_jni.h>

void create_game1(void);
void destroy_game1(void);
void del_egl(void);
void init_egl(ANativeWindow*);
void on_resume(void);
void on_pause(void);

#endif // GAME1_RENDERER_H