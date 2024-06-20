#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h> 
#include <android/native_window_jni.h> 
#include <jni.h>

#include "shaders/loader.h"

/*
 * game driver module arributes(modles/driver.c)
 */
#define MINIGAME_MAJOR 242
#define IOCTL_RUN_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 0)
#define IOCTL_STOP_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 1)
#define IOCTL_OFF_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 2)
#define IOCTL_RESET_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 3)
#define IOCTL_SET_TEXT_LCD_NONBLOCK _IOW(MINIGAME_MAJOR, 5, char*)
#define IOCTL_WAIT_BACK_INTERRUPT _IOR(MINIGAME_MAJOR, 6, int)
enum direction{
	NONE = 0,
	UP,
	LEFT,
	RIGHT,
	DOWN
};

#define COMPLETE_STR "COMPLETE"
#define EMPTY_STR ""

/*
 * esl attributes
 */
const static EGLint attribs[] = {
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_BLUE_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_RED_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_DEPTH_SIZE, 16,
	EGL_NONE
};
const static EGLint context_attribs[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};
struct egl_status{
	ANativeWindow* window;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLConfig config;
	EGLint numConfigs;
	EGLint format;
	EGLint width;
	EGLint height;

	bool exists_window;
};
static struct egl_status egl;

struct game_state{
	// When the activity resumes, a thread is created, 
	// and when the activity pauses, the thread is destroyed.
	// These variables are used to store informations related to the thread.
	pthread_t tid;
	pthread_mutex_t mutex;
	bool is_paused;
};
static struct game_state gstate;

struct game_pad{
	int fd;
	enum direction prev_dir;
};
static struct game_pad gpad;

static void reset_ingame_attrib(void){
	gstate.is_paused = true;
	
}

void game2_create(){
	gpad.fd = open("/dev/minigame", O_RDWR);
	if(gpad.fd == -1) LOG_ERROR("open error");
	else LOG_INFO("open success");

	egl.exists_window = false;
	
	pthread_mutex_init(&gstate.mutex, 0);
	reset_ingame_attrib();

}

void game2_destroy(){
	pthread_mutex_destroy(&gstate.mutex);
	reset_ingame_attrib();

	close(gpad.fd);
}