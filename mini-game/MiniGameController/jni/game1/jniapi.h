#ifndef GAME1_JNIAPI_H
#define GAME1_JNIAPI_H

extern "C" {
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnCreate(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnDestroy(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnResume(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnPause(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeSetSurface(JNIEnv* env, jobject obj, jobject surface);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeRestartGame(JNIEnv* env, jobject obj);
	JNIEXPORT jboolean JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeWaitBackInterrupt(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnStart(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnStop(JNIEnv* env, jobject obj);
};

#endif // GAME1_JNIAPI_H