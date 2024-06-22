#ifndef GAME2_RENDERER_H
#define GAME2_RENDERER_H

#include <vector>
#include <android/native_window.h> 
#include <android/native_window_jni.h>

void game2_create(std::vector<std::vector<int> > maze);
void game2_destroy(void);
void game2_del_surface(void);
void game2_set_surface(ANativeWindow*);
void game2_resume(void);
void game2_pause(void);
void game2_start(void);
void game2_stop(void);
void game2_restart(std::vector<std::vector<int> > maze);
bool game2_wait_back_interrupt(void);

#endif // GAME2_RENDERER_H