#include <jni.h>
#include <android/native_window.h> 
#include <android/native_window_jni.h> 
#include "jniapi.h"
#include "../logger.h"
#include "renderer.h"

static ANativeWindow* window;

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnCreate(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnCreate");
	game1_create();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnDestroy(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnDestroy");
	game1_destroy();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnResume(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnResume");
	game1_resume();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnPause(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnPause");
	game1_pause();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeSetSurface(JNIEnv* env, jobject obj, jobject surface){
	if (surface != 0) {
		window = ANativeWindow_fromSurface(env, surface);
		LOG_INFO("Got window %p", window);
		game1_set_surface(window);
	} 
	else {
		game1_del_surface();
		LOG_INFO("Releasing window");
	}
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeRestartGame(JNIEnv* env, jobject obj){
	LOG_INFO("nativeRestartGame");
	game1_restart();
}

JNIEXPORT jboolean JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeWaitBackInterrupt(JNIEnv* env, jobject obj){
	LOG_INFO("nativeWaitBackInterrupt");
	return game1_wait_back_interrupt();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnStart(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnStart");
	game1_start();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnStop(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnStop");
	game1_stop();
}