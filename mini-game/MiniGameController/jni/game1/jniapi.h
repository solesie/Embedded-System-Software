#ifndef GAME1_JNIAPI_H
#define GAME1_JNIAPI_H

extern "C" {
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnCreate(JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnDestroy(JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnResume(JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnPause(JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeSetSurface(JNIEnv* jenv, jobject obj, jobject surface);
	JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeRestartGame(JNIEnv* env, jobject obj);
	JNIEXPORT jboolean JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeWaitBackInterrupt(JNIEnv* env, jobject obj);
};

#endif // GAME1_JNIAPI_H