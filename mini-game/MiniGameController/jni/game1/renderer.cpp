#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <string>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h> 
#include <android/native_window_jni.h> 
#include <jni.h>

#include "shaders/loader.h"
#include "models/road.h"
#include "models/sword.h"
#include "models/car.h"
#include "models/house.h"
#include "models/androboy.h"
#include "renderer.h"
#include "../logger.h"

#define TO_RADIAN 0.01745329252f
#define TO_DEGREE 57.295779513f

/*
 * game driver module arributes(modles/driver.c)
 */
#define MINIGAME_MAJOR 242
#define IOCTL_RUN_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 0)
#define IOCTL_STOP_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 1)
#define IOCTL_OFF_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 2)
#define IOCTL_RESET_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 3)
#define IOCTL_SET_LED_NONBLOCK _IOW(MINIGAME_MAJOR, 4, int)
#define IOCTL_SET_TEXT_LCD_NONBLOCK _IOW(MINIGAME_MAJOR, 5, char*)
#define IOCTL_WAIT_BACK_INTERRUPT _IOR(MINIGAME_MAJOR, 6, int)
enum direction{
	NONE = 0,
	UP,
	LEFT,
	RIGHT,
	DOWN
};
#define INITIAL_LIFE 3
#define LIFECOUNT_STR "LIFECOUNT: "
#define GAMEOVER_STR "GAMEOVER        BACK->RESTART"

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
	int androboy_cur_line, androboy_cur_x;
	int androboy_cur_rightest, androboy_cur_leftest;
	int house_x;
	bool gameover;
	int lifecount;
};
static struct game_state gstate;

/*
 * Only used to render gameover animation motion.
 */
struct gameover_motion_attrib{
	long long gameover_time;

	std::pair<float, float> last_crushed_car; // (x, y)
	std::vector<std::pair<float, float> > gameover_swords;

	std::vector<float> rotating_swords_time_limit;
	std::vector<float> rotating_swords_cur_time;
	std::vector<float> killing_swords_delta;
};
static struct gameover_motion_attrib gover_attrib;

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
static void release_context(void){
	road_release();
	sword_release();
	car_release();
	house_release();
	androboy_release();

	game1_shaders_del();

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
static void acquire_context(void){
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
		game1_del_surface();
		return;
	}
	if (!eglGetConfigAttrib(egl.display, egl.config, EGL_NATIVE_VISUAL_ID, &egl.format)) {
		LOG_ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
		release_context();
		game1_del_surface();
		return;
	}
	ANativeWindow_setBuffersGeometry(egl.window, 0, 0, egl.format);
	if (!(egl.surface = eglCreateWindowSurface(egl.display, egl.config, egl.window, 0))) {
		LOG_ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
		release_context();
		game1_del_surface();
		return;
	}
	if (!(egl.context = eglCreateContext(egl.display, egl.config, NULL, context_attribs))) {
		LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
		release_context();
		game1_del_surface();
		return;
	}
	if (!eglMakeCurrent(egl.display, egl.surface, egl.surface, egl.context)) {
		LOG_ERROR("eglMakeCurrent() returned error %d", eglGetError());
		release_context();
		game1_del_surface();
		return;
	}
	if (!eglQuerySurface(egl.display, egl.surface, EGL_WIDTH, &egl.width) ||
		!eglQuerySurface(egl.display, egl.surface, EGL_HEIGHT, &egl.height)) {
		LOG_ERROR("eglQuerySurface() returned error %d", eglGetError());
		release_context();
		game1_del_surface();
		return;
	}

	// for performance
	glDisable(GL_DITHER);
	glEnable(GL_CULL_FACE);

	// red
	glClearColor(220 / 255.0f, 20 / 255.0f, 60 / 255.0f, 1.0f);

	// world coordinate
	glViewport(0, 0, egl.width, egl.height);

	// should be inited at current context
	game1_shaders_init();

	// prepare models
	road_prepare();
	sword_prepare();
	car_prepare();
	house_prepare();
	androboy_prepare();
}

static void reset_ingame_attrib(void){
	gstate.is_paused = true;
	gstate.androboy_cur_line = 1;
	gstate.androboy_cur_x = -400;
	gstate.androboy_cur_rightest = gstate.androboy_cur_x + 34;
	gstate.androboy_cur_leftest = gstate.androboy_cur_x - 34;
	gstate.cur_time = 0;
	gstate.gameover = false;
	gstate.lifecount = INITIAL_LIFE;

	gover_attrib.gameover_time = 0;
	gover_attrib.last_crushed_car = std::make_pair(0.0f, 0.0f);
	gover_attrib.gameover_swords.clear();
	gover_attrib.rotating_swords_cur_time.clear();
	gover_attrib.rotating_swords_time_limit.clear();
	gover_attrib.killing_swords_delta.clear();

	gpad.prev_dir = NONE;

	cars_free();
}

/*
 * Read fpga push switch from minigame driver and
 * change gstate androboy coords.
 */
static void read_gpad(void){
	enum direction input = NONE;
	if(read(gpad.fd, &input, 1) != 1) return;
	if(gstate.gameover) return;
	switch(input){
	case LEFT:
		if (-490 < gstate.androboy_cur_x) {
			gstate.androboy_cur_x -= 10;
			gstate.androboy_cur_leftest -= 10;
			gstate.androboy_cur_rightest -= 10;
		}
		gpad.prev_dir = LEFT;
		break;
	case RIGHT:
		if(gstate.androboy_cur_x < 490){
			gstate.androboy_cur_x += 10;
			gstate.androboy_cur_leftest += 10;
			gstate.androboy_cur_rightest += 10;
		}
		gpad.prev_dir = RIGHT;
		break;
	case DOWN:
		if(gstate.androboy_cur_line < 3 && gpad.prev_dir != DOWN)
			++gstate.androboy_cur_line;
		gpad.prev_dir = DOWN;
		break;
	case UP:
		if(gstate.androboy_cur_line > 0 && gpad.prev_dir != UP)
			--gstate.androboy_cur_line;
		gpad.prev_dir = UP;
		break;
	case NONE:
		gpad.prev_dir = NONE;
		break;
	default:
		LOG_ERROR("Undefined direction is detected");
		break;
	}
}

static void process_gameover(void){
	if(gstate.gameover) return;
	// check crushed car
	std::pair<float, float> crushed_car;
	if(car_remove_crushed(gstate.cur_time, gstate.androboy_cur_line, 
		gstate.androboy_cur_rightest, gstate.androboy_cur_leftest, &crushed_car)){
		--gstate.lifecount;
		// check gameover
		if(gstate.lifecount > 0){
			std::string str = LIFECOUNT_STR;
			str += gstate.lifecount + '0';
			ioctl(gpad.fd, IOCTL_SET_LED_NONBLOCK, &gstate.lifecount);
			ioctl(gpad.fd, IOCTL_SET_TEXT_LCD_NONBLOCK, str.c_str());
		}
		else{
			// set gameover motion rendering attributes
			std::string str = GAMEOVER_STR;
			std::vector<std::pair<float, float> > swords = swords_get(gstate.cur_time);
			ioctl(gpad.fd, IOCTL_OFF_TIMER_NONBLOCK);
			ioctl(gpad.fd, IOCTL_SET_LED_NONBLOCK, &gstate.lifecount);
			ioctl(gpad.fd, IOCTL_SET_TEXT_LCD_NONBLOCK, str.c_str());
			gstate.gameover = true;
			gover_attrib.last_crushed_car = crushed_car;
			gover_attrib.gameover_time = gstate.cur_time;
			gover_attrib.gameover_swords = swords;

			int ss = gover_attrib.gameover_swords.size();
			float androboy_x = gstate.androboy_cur_x;
			float androboy_y = -90 - (gstate.androboy_cur_line * 67.5);
			gover_attrib.rotating_swords_cur_time.resize(ss, 0.0f);
			gover_attrib.killing_swords_delta.resize(ss, 0.0f);
			gover_attrib.rotating_swords_time_limit.resize(ss);
			for (int i = 0; i < ss; ++i) {
				float sword_x = gover_attrib.gameover_swords[i].first;
				float sword_y = gover_attrib.gameover_swords[i].second;
				float sword_angle = atanf((sword_y - androboy_y) / (sword_x - androboy_x));
				sword_angle += sword_angle < 0 ? 270 * TO_RADIAN : 90 * TO_RADIAN;
				gover_attrib.rotating_swords_time_limit[i] = sword_angle * TO_DEGREE;
			}
		}
	}
}

static void draw_frame(void){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// road
	m_matrix = glm::mat4(1.0f);
	mvp_matrix = vp_matrix * m_matrix;
	glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
	road_draw();

	// superman androboy
	int superman_androboy_x = (gstate.cur_time % 1201) - 600; // -600 <= superman_androboy_x <= 600
	if (superman_androboy_x <= 200) {
		//y=0.2x+250
		m_matrix = glm::translate(m_matrix, glm::vec3(superman_androboy_x, 0.2f * superman_androboy_x + 250, 0.0f));
		m_matrix = glm::rotate(m_matrix, -90 * TO_RADIAN + atanf(0.2), glm::vec3(0.0f, 0.0f, 1.0f));
		m_matrix = glm::scale(m_matrix, glm::vec3(1.5f, 1.5f, 1.0f));
	}
	else {
		//y=-0.5x+368
		m_matrix = glm::translate(m_matrix, glm::vec3(superman_androboy_x, -0.5f * superman_androboy_x + 368, 0.0f));
		m_matrix = glm::rotate(m_matrix, (superman_androboy_x % 360) * 5 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		m_matrix = glm::scale(m_matrix, glm::vec3(300.0f / superman_androboy_x, 300.0f / superman_androboy_x, 1.0f));
	}
	mvp_matrix = vp_matrix * m_matrix;
	glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
	androboy_draw();

	// cars
	std::vector<std::pair<float, float> > cars = cars_get(gstate.cur_time);
	for (int i = 0; i < cars.size(); ++i) {
		m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(cars[i].first, cars[i].second, 0.0f));
		m_matrix = glm::scale(m_matrix, glm::vec3(2.0, 2.0, 1.0f));
		mvp_matrix = vp_matrix * m_matrix;
		glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
		car_draw();
	}

	if (gstate.cur_time % 50 == 0) 
		car_add(gstate.cur_time);
	cars_remove_out_of_bound(gstate.cur_time);

	if(!gstate.gameover){
		// house
		gstate.house_x = (gstate.cur_time % 3202) / 2 - 800; // -800 <= house_x <= 800
		m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f*gstate.house_x, -10.0f, 0.0f));
		m_matrix = glm::scale(m_matrix, glm::vec3(50.0f, 5.0f, 1.0f));
		mvp_matrix = vp_matrix * m_matrix;
		glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
		house_draw();

		// androboy
		m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(gstate.androboy_cur_x, -90 - (gstate.androboy_cur_line * 67.5), 0.0f));
		m_matrix = glm::scale(m_matrix, glm::vec3(0.9f, 0.9f, 1.0f));
		mvp_matrix = vp_matrix * m_matrix;
		glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
		androboy_draw_without_foots();
		int foot_clock = gstate.cur_time % 80;
		if (foot_clock < 40) {
			androboy_draw_right_foot();
			m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(gstate.androboy_cur_x, -90 - (gstate.androboy_cur_line * 67.5) + 0.25f * foot_clock, 0.0f));
			m_matrix = glm::scale(m_matrix, glm::vec3(0.9f, 0.9f, 1.0f));
			mvp_matrix = vp_matrix * m_matrix;
			glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
			androboy_draw_left_foot();
		}
		else {
			androboy_draw_left_foot();
			m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(gstate.androboy_cur_x, -90 - (gstate.androboy_cur_line * 67.5) + 0.25f * (foot_clock - 40), 0.0f));
			m_matrix = glm::scale(m_matrix, glm::vec3(0.9, 0.9, 1.0f));
			mvp_matrix = vp_matrix * m_matrix;
			glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
			androboy_draw_right_foot();
		}

		// swords
		std::vector<std::pair<float, float> > swords = swords_get(gstate.cur_time);
		for (int i = 0; i < swords.size(); ++i) {
			m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(swords[i].first, swords[i].second, 0.0f));
			m_matrix = glm::scale(m_matrix, glm::vec3(2.2, 6.5, 1.0f));
			mvp_matrix = vp_matrix * m_matrix;
			glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
			sword_draw();
		}
	}
	else{
		long long passed = gstate.cur_time - gover_attrib.gameover_time;
		long long crushed_car_cur_time = std::min(180LL, passed);
		long long gameover_sword_cur_time = std::min(1000LL, passed);
		long long gameover_androboy_cur_time = std::min(90LL, passed);
		int ss = gover_attrib.gameover_swords.size();
		float androboy_x = gstate.androboy_cur_x;
		float androboy_y = -90.0f - (gstate.androboy_cur_line * 67.5);

		//draw stopped house
		m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f * gstate.house_x, -10.0, 0.0f));
		m_matrix = glm::scale(m_matrix, glm::vec3(50.0f, 5.0f, 1.0f));
		mvp_matrix = vp_matrix * m_matrix;
		glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
		house_draw();

		//draw crushed car
		m_matrix = glm::translate(glm::mat4(1.0f), 
			glm::vec3(gover_attrib.last_crushed_car.first + crushed_car_cur_time, 
				gover_attrib.last_crushed_car.second + 100.0f * sinf(crushed_car_cur_time * TO_RADIAN), 0.0f));
		m_matrix = glm::rotate(m_matrix, crushed_car_cur_time * TO_RADIAN, glm::vec3(0.0, 0.0, 1.0f));
		m_matrix = glm::scale(m_matrix, glm::vec3(2.0f, 2.0f, 1.0f));
		mvp_matrix = vp_matrix * m_matrix;
		glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
		car_draw();

		//draw stopped sword
		if (gameover_sword_cur_time < 100) {
			for (int i = 0; i < ss; ++i) {
				m_matrix = glm::translate(glm::mat4(1.0f), 
					glm::vec3(gover_attrib.gameover_swords[i].first, gover_attrib.gameover_swords[i].second, 0.0f));
				m_matrix = glm::scale(m_matrix, glm::vec3(2.2f, 6.5f, 1.0f));
				mvp_matrix = vp_matrix * m_matrix;
				glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
				sword_draw();
			}
		}
		//draw rotating sword
		else if (gameover_sword_cur_time < 200) {
			for (int i = 0; i < ss; ++i) {
				float sword_x = gover_attrib.gameover_swords[i].first;
				float sword_y = gover_attrib.gameover_swords[i].second;
				gover_attrib.rotating_swords_cur_time[i] = std::min(gover_attrib.rotating_swords_time_limit[i], (float)passed);
				m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(sword_x, sword_y, 0.0f));
				m_matrix = glm::rotate(m_matrix, gover_attrib.rotating_swords_cur_time[i] * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
				m_matrix = glm::scale(m_matrix, glm::vec3(2.2, 6.5, 1.0f));
				mvp_matrix = vp_matrix * m_matrix;
				glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
				sword_draw();
			}
		}
		//draw moving sword
		else if (gameover_sword_cur_time <= 1000) {
			for (int i = 0; i < ss; ++i) {
				float sword_x = gover_attrib.gameover_swords[i].first;
				float sword_y = gover_attrib.gameover_swords[i].second;
				// retrace androboy
				if (gover_attrib.rotating_swords_time_limit[i] >= 180.0f && sword_x + gover_attrib.killing_swords_delta[i] < androboy_x) 
					gover_attrib.killing_swords_delta[i] += 4.0f;
				if (180.0f > gover_attrib.rotating_swords_time_limit[i] && gover_attrib.rotating_swords_time_limit[i] >= 90.0f 
					&& sword_x + gover_attrib.killing_swords_delta[i] > androboy_x)
					gover_attrib.killing_swords_delta[i] -= 4.0f;
				
				float new_x = sword_x + gover_attrib.killing_swords_delta[i];
				float new_y = (sword_y - androboy_y) / (sword_x - androboy_x) * (new_x - androboy_x) + androboy_y;
				m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(new_x, new_y, 0.0f));
				m_matrix = glm::rotate(m_matrix, gover_attrib.rotating_swords_time_limit[i] * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
				m_matrix = glm::scale(m_matrix, glm::vec3(2.2, 6.5, 1.0f));
				mvp_matrix = vp_matrix * m_matrix;
				glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
				sword_draw();
			}
		}

		// draw died androboy
		m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(androboy_x, androboy_y, 0.0f));
		m_matrix = glm::rotate(m_matrix, gameover_androboy_cur_time * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		m_matrix = glm::scale(m_matrix, glm::vec3(0.9f, 0.9f, 1.0f));
		mvp_matrix = vp_matrix * m_matrix;
		glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
		androboy_draw();
	}
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
		read_gpad();
		process_gameover();
		draw_frame();
		if (!eglSwapBuffers(egl.display, egl.surface)) {
			LOG_ERROR("eglSwapBuffers() returned error %d", eglGetError());
		}
		++gstate.cur_time;
		pthread_mutex_unlock(&gstate.mutex);
		usleep(10000); // 10ms
	}
}

void game1_create(void){
	gpad.fd = open("/dev/minigame", O_RDWR);
	if(gpad.fd == -1) LOG_ERROR("open error");
	else LOG_INFO("open success");
	int initial_life = INITIAL_LIFE;
	std::string str = LIFECOUNT_STR;
	str += initial_life + '0';
	ioctl(gpad.fd, IOCTL_RESET_TIMER_NONBLOCK);
	ioctl(gpad.fd, IOCTL_SET_LED_NONBLOCK, &initial_life);
	ioctl(gpad.fd, IOCTL_SET_TEXT_LCD_NONBLOCK, str.c_str());

	egl.exists_window = false;
	
	reset_ingame_attrib();

	swords_create();
}

void game1_destroy(void){
	reset_ingame_attrib();
	swords_destroy();
	close(gpad.fd);
}

void game1_start(void){
	pthread_mutex_init(&gstate.mutex, 0);
}

void game1_stop(void){
	pthread_mutex_destroy(&gstate.mutex);
}

void game1_del_surface(void){
	ANativeWindow_release(egl.window);
	egl.window = NULL;
	egl.exists_window = false;
}

void game1_set_surface(ANativeWindow* window){
	if(egl.exists_window) return;

	LOG_INFO("Init egl");

	egl.window = window;
	egl.exists_window = true;

	pthread_create(&gstate.tid, NULL, render_loop, NULL);
	return;
}

void game1_resume(void){
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

void game1_pause(void){
	pthread_mutex_lock(&gstate.mutex);
	gstate.is_paused = true;
	pthread_mutex_unlock(&gstate.mutex);

	pthread_join(gstate.tid, 0);
	
	ioctl(gpad.fd, IOCTL_STOP_TIMER_NONBLOCK);
}

/*
 * The restart method is always called in the onPause state and before onResume.
 */
void game1_restart(void){
	reset_ingame_attrib();
	
	int initial_life = INITIAL_LIFE;
	std::string str = LIFECOUNT_STR;
	str += initial_life + '0';
	ioctl(gpad.fd, IOCTL_RESET_TIMER_NONBLOCK);
	ioctl(gpad.fd, IOCTL_SET_LED_NONBLOCK, &initial_life);
	ioctl(gpad.fd, IOCTL_SET_TEXT_LCD_NONBLOCK, str.c_str());
}

/*
 * In a blocking manner, the back interrupt detector thread, which is passed in Java, 
 * detects the back button interrupt.
 */
bool game1_wait_back_interrupt(void){
	int waked_by_intr;
	ioctl(gpad.fd, IOCTL_WAIT_BACK_INTERRUPT, &waked_by_intr);
	if(waked_by_intr) {
		LOG_INFO("waked up by interrupt");
		return true;
	}
	LOG_INFO("waked up by pause");
	return false;
}