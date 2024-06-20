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
#include "models/androman.h"
#include "models/tile.h"
#include "renderer.h"
#include "../logger.h"

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

	long long cur_time;
};
static struct game_state gstate;

struct game_pad{
	int fd;
	enum direction prev_dir;
};
static struct game_pad gpad;

static glm::mat4 mvp_matrix, vp_matrix, m_matrix;

/* 
 * OpenGL calls operate on a current context. 
 * Before you can make any OpenGL calls, you need to create a context, and make it current. 
 * The context being current applies to a thread, so different threads should have different current contexts.
 */
static void release_context(){
	androman_release();
	tile_release();

	game2_shaders_del();

	eglMakeCurrent(egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(egl.display, egl.context);
	eglDestroySurface(egl.display, egl.surface);
	eglTerminate(egl.display);

	egl.display = EGL_NO_DISPLAY;
	egl.surface = EGL_NO_SURFACE;
	egl.context = EGL_NO_CONTEXT;
	egl.config = NULL;
	egl.numConfigs = egl.format = egl.width = egl.height = 0;
}
static void acquire_context(){
	LOG_INFO("Acquire new context");

	if ((egl.display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
		LOG_ERROR("eglGetDisplay() returned error %d", eglGetError());
		return;
	}
	if (!eglInitialize(egl.display, 0, 0)) {
		LOG_ERROR("eglInitialize() returned error %d", eglGetError());
		return;
	}
	if (!eglChooseConfig(egl.display, attribs, &egl.config, 1, &egl.numConfigs)) {
		LOG_ERROR("eglChooseConfig() returned error %d", eglGetError());
		release_context();
		game2_del_surface();
		return;
	}
	if (!eglGetConfigAttrib(egl.display, egl.config, EGL_NATIVE_VISUAL_ID, &egl.format)) {
		LOG_ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
		release_context();
		game2_del_surface();
		return;
	}
	ANativeWindow_setBuffersGeometry(egl.window, 0, 0, egl.format);
	if (!(egl.surface = eglCreateWindowSurface(egl.display, egl.config, egl.window, 0))) {
		LOG_ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
		release_context();
		game2_del_surface();
		return;
	}
	if (!(egl.context = eglCreateContext(egl.display, egl.config, NULL, context_attribs))) {
		LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
		release_context();
		game2_del_surface();
		return;
	}
	if (!eglMakeCurrent(egl.display, egl.surface, egl.surface, egl.context)) {
		LOG_ERROR("eglMakeCurrent() returned error %d", eglGetError());
		release_context();
		game2_del_surface();
		return;
	}
	if (!eglQuerySurface(egl.display, egl.surface, EGL_WIDTH, &egl.width) ||
		!eglQuerySurface(egl.display, egl.surface, EGL_HEIGHT, &egl.height)) {
		LOG_ERROR("eglQuerySurface() returned error %d", eglGetError());
		release_context();
		game2_del_surface();
		return;
	}

	// for performance
	glDisable(GL_DITHER);
	glEnable(GL_CULL_FACE);

	// gray
	glClearColor(128 / 255.0f, 128 / 255.0f, 128 / 255.0f, 1.0f);

	// world coordinate
	glViewport(0, 0, egl.width, egl.height);

	// should be inited at current context
	game2_shaders_init();

	// prepare models
	androman_prepare();
	tile_prepare();
}

static void reset_ingame_attrib(void){
	gstate.is_paused = true;
	
}

static void* render_loop(void* nouse){
	acquire_context();

	// change screeen vertical to horizontal and normalize coords.
	vp_matrix = glm::ortho(-egl.width / 2.0, egl.width / 2.0, 
		-egl.height / 1.35, egl.height / 1.35, -1000.0, 1000.0);
	
	while(true){
		// consider onPause()
		pthread_mutex_lock(&gstate.mutex);
		if(gstate.is_paused){
			release_context();
			pthread_mutex_unlock(&gstate.mutex);
			pthread_exit(0);
		}
		// read_gpad();
		// draw_frame();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_matrix = glm::mat4(1.0f);
		mvp_matrix = vp_matrix * m_matrix;
		glUniformMatrix4fv(g2_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
		tile_draw();
		if (!eglSwapBuffers(egl.display, egl.surface)) {
			LOG_ERROR("eglSwapBuffers() returned error %d", eglGetError());
		}
		++gstate.cur_time;
		pthread_mutex_unlock(&gstate.mutex);
		usleep(10000); // 10ms
	}
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

void game2_del_surface(void){
	ANativeWindow_release(egl.window);
	egl.window = NULL;
	egl.exists_window = false;
}

void game2_set_surface(ANativeWindow* window){
	if(egl.exists_window) return;

	LOG_INFO("Init egl");

	egl.window = window;
	egl.exists_window = true;

	pthread_create(&gstate.tid, NULL, render_loop, NULL);
	return;
}

void game2_resume(void){
	pthread_mutex_lock(&gstate.mutex);
	gstate.is_paused = false;
	pthread_mutex_unlock(&gstate.mutex);

	// At most onResume() -> surfaceChanged() -> onPause() -> surfaceDestroyed().
	// However, if the created surface is not destroyed, from the second onResume onwards, 
	// surfaceCreated and surfaceChanged will be ignored.
	// In other words, the rendering thread is initially started from surfaceChanged 
	// and subsequently from onResume.
	if(egl.exists_window)
		pthread_create(&gstate.tid, NULL, render_loop, NULL);
	else
		LOG_INFO("First onResume()");

	ioctl(gpad.fd, IOCTL_RUN_TIMER_NONBLOCK);
}

void game2_pause(void){
	pthread_mutex_lock(&gstate.mutex);
	gstate.is_paused = true;
	pthread_mutex_unlock(&gstate.mutex);

	pthread_join(gstate.tid, 0);
	
	ioctl(gpad.fd, IOCTL_STOP_TIMER_NONBLOCK);
}