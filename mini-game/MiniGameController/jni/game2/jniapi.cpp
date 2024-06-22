#include <jni.h>
#include <android/native_window.h> 
#include <android/native_window_jni.h> 
#include "jniapi.h"
#include "../logger.h"
#include "renderer.h"

static ANativeWindow* window;

static std::vector<std::vector<int> > generate_maze(JNIEnv* env, jobject obj){
	jclass cls = env->GetObjectClass(obj);
	jmethodID mid = env->GetMethodID(cls, "generateMaze","()[[I");
	if(mid == NULL)
		LOG_ERROR("method id not found");
	
	std::vector<std::vector<int> > ret;
	jobjectArray res = (jobjectArray)env->CallObjectMethod(obj, mid);
	jsize row = env->GetArrayLength(res);
	ret.resize(row);
	for(int i = 0; i < row; ++i){
		jintArray jarr = (jintArray)env->GetObjectArrayElement(res, i);
		jsize col = env->GetArrayLength(jarr);
		ret[i].resize(col);
		jint* carr = env->GetIntArrayElements(jarr, NULL);
		for(int j = 0; j < col; ++j)
			ret[i][j] = carr[j];
		env->ReleaseIntArrayElements(jarr, carr, 0);
	}
	return ret;
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnCreate(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnCreate");
	game2_create(generate_maze(env, obj));
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnDestroy(JNIEnv* env, jobject obj){
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

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeRestartGame(JNIEnv* env, jobject obj){
	LOG_INFO("nativeRestartGame");
	game2_restart(generate_maze(env, obj));
}

JNIEXPORT jboolean JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeWaitBackInterrupt(JNIEnv* env, jobject obj){
	LOG_INFO("nativeWaitBackInterrupt");
	return game2_wait_back_interrupt();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnStart(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnStart");
	game2_start();
}

JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnStop(JNIEnv* env, jobject obj){
	LOG_INFO("nativeOnStop");
	game2_stop();
}