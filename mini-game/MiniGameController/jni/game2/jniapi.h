#ifndef GAME2_JNIAPI_H
#define GAME2_JNIAPI_H

extern "C" {
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnCreate(JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnDestroy(JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnResume(JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeOnPause(JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeSetSurface(JNIEnv* jenv, jobject obj, jobject surface);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeRestartGame(JNIEnv* env, jobject obj);
	JNIEXPORT jboolean JNICALL Java_com_example_minigamecontroller_Game2Activity_nativeWaitBackInterrupt(JNIEnv* env, jobject obj);
};

#endif // GAME2_JNIAPI_H