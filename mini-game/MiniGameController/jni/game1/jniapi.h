#ifndef GAME1_JNIAPI_H
#define GAME1_JNIAPI_H

extern "C" {
    JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnStart(JNIEnv* jenv, jobject obj);
    JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnStop(JNIEnv* jenv, jobject obj);
    JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnResume(JNIEnv* jenv, jobject obj);
    JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeOnPause(JNIEnv* jenv, jobject obj);
    JNIEXPORT void JNICALL Java_com_example_minigamecontroller_Game1Activity_nativeSetSurface(JNIEnv* jenv, jobject obj, jobject surface);
};

#endif // GAME1_JNIAPI_H