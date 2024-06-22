#ifndef GAME2_JNIAPI_H
#define GAME2_JNIAPI_H

extern "C" {
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnCreate(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnDestroy(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnResume(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnPause(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeSetSurface(JNIEnv* env, jobject obj, jobject surface);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeRestartGame(JNIEnv* env, jobject obj);
	JNIEXPORT jboolean JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeWaitBackInterrupt(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnStart(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnStop(JNIEnv* env, jobject obj);
};

#endif // GAME2_JNIAPI_H