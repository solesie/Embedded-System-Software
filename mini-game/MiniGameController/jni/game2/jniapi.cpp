#include <jni.h>
#include <android/native_window.h> 
#include <android/native_window_jni.h> 
#include "jniapi.h"
#include "../logger.h"
#include "renderer.h"

static ANativeWindow* window;

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnCreate(JNIEnv* jenv, jobject obj){
	LOG_INFO("nativeOnCreate");
	game2_create();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnDestroy(JNIEnv* jenv, jobject obj){
	LOG_INFO("nativeOnDestroy");
	game2_destroy();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnResume(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnResume");
	game2_resume();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnPause(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnPause");
	game2_pause();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeSetSurface(JNIEnv* env, jobject obj, jobject surface){
	if (surface != 0) {
		window = ANativeWindow_fromSurface(env, surface);
		LOG_INFO("Got window %p", window);
		game2_set_surface(window);
	} 
	else {
		game2_del_surface();
		LOG_INFO("Releasing window");
	}
}

// JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeRestartGame(JNIEnv* env, jobject obj){
// 	LOG_INFO("nativeRestartGame");
// 	game2_restart();
// }

// JNIEXPORT jboolean JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeWaitBackInterrupt(JNIEnv* env, jobject obj){
// 	LOG_INFO("nativeWaitBackInterrupt");
// 	return game2_wait_back_interrupt();
// }