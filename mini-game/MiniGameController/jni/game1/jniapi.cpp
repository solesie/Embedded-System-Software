#include <jni.h>
#include <android/native_window.h> 
#include <android/native_window_jni.h> 
#include "jniapi.h"
#include "../logger.h"
#include "renderer.h"

static ANativeWindow* window;

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnStart(JNIEnv* jenv, jobject obj){
    LOG_INFO("nativeOnStart");
    create_game1();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnStop(JNIEnv* jenv, jobject obj){
    LOG_INFO("nativeOnStop");
    destroy_game1();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnResume(JNIEnv* env, jobject obj){
    LOG_INFO("nativeOnResume");
    on_resume();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnPause(JNIEnv* env, jobject obj){
    LOG_INFO("nativeOnPause");
    on_pause();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeSetSurface(JNIEnv* env, jobject obj, jobject surface){
    if (surface != 0) {
        window = ANativeWindow_fromSurface(env, surface);
        LOG_INFO("Got window %p", window);
        init_egl(window);
    } 
    else {
        del_egl();
        LOG_INFO("Releasing window");
    }
}
