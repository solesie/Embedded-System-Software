#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>

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
const static int ndir[5][2] = {
	{0,0}, {-1,0}, {0,-1}, {0,1}, {1,0}
};

#define MOVE_CNT_STR "MOVE CNT: "
#define COMPLETE_STR "COMPLETE        BACK->RESTART"

#define WIDTH_RATIO 2.0f
#define HEIGHT_RATIO 1.35f

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
	std::vector<std::vector<int> > maze;
	std::pair<int, int> cur_androman; // (r, c)
	std::pair<int, int> goal_point; // (r, c)
	int move_cnt;
	bool gameover;
};
static struct game_state gstate;

struct game_pad{
	int fd;
	enum direction prev_dir;
};
static struct game_pad gpad;

static glm::mat4 mvp_matrix, vp_matrix, m_matrix;

/*
 * (r, c) to (x, y) of tile's center
 */
static inline std::pair<float, float> maze_to_coord(int i, int j){
	return std::make_pair(2.0f*egl.width/WIDTH_RATIO/gstate.maze[0].size()*j - egl.width/WIDTH_RATIO + egl.width/WIDTH_RATIO/gstate.maze[0].size(), 
		-2.0f*egl.height/HEIGHT_RATIO/gstate.maze.size()*i + egl.height/HEIGHT_RATIO - egl.height/HEIGHT_RATIO/gstate.maze.size());
}

/* 
 * OpenGL calls operate on a current context. 
 * Before you can make any OpenGL calls, you need to create a context, and make it current. 
 * The context being current applies to a thread, so different threads should have different current contexts.
 */
static void release_context(void){
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

static void reset_ingame_attrib(std::vector<std::vector<int> >& maze){
	gstate.is_paused = true;
	gstate.cur_time = 0;
	for(int i = 0; i < gstate.maze.size(); ++i) 
		gstate.maze[i].clear();
	gstate.maze.clear();
	gstate.maze.resize(maze.size());
	for(int i = 0; i < gstate.maze.size(); ++i)
		gstate.maze[i] = maze[i];
	
	gstate.cur_androman = std::make_pair(0, 0);
	gstate.goal_point = std::make_pair(0, 0);
	if(maze.size() >= 3 && maze[0].size() >= 3){
		int r = gstate.maze.size() - 2;
		for(int j = 0; j < gstate.maze[r].size(); ++j){
			if(gstate.maze[r][j] == 0){
				gstate.cur_androman = std::make_pair(r, j);
				break;
			}
		}
		for(int j = gstate.maze[1].size() - 1; j >= 0; --j){
			if(gstate.maze[1][j] == 0){
				gstate.goal_point = std::make_pair(1, j);
				break;
			}
		}
	}
	gstate.gameover = false;
	gstate.move_cnt = 0;
	gpad.prev_dir = NONE;
}

static void process_gameover(void){
	if(gstate.gameover) return;
	if(gstate.cur_androman == gstate.goal_point){
		std::string str = COMPLETE_STR;
		ioctl(gpad.fd, IOCTL_OFF_TIMER_NONBLOCK);
		ioctl(gpad.fd, IOCTL_SET_TEXT_LCD_NONBLOCK, str.c_str());
		gstate.gameover = true;
	}
}

/*
 * Read fpga push switch from minigame driver and
 * change gstate androman coords.
 */
static void read_gpad(void){
	enum direction input = NONE;
	if(read(gpad.fd, &input, 1) != 1) return;
	if(gstate.gameover) return;

	for(int nd = 0; nd < 5; ++nd){
		if(input == nd && input != gpad.prev_dir){
			int nr = gstate.cur_androman.first + ndir[nd][0];
			int nc = gstate.cur_androman.second + ndir[nd][1];
			if(0 <= nr && nr < gstate.maze.size() && 0 <= nc && nc < gstate.maze[0].size()
				&& gstate.maze[nr][nc] == 0){
				gstate.cur_androman = std::make_pair(nr, nc);
				if(gstate.move_cnt < INT_MAX && input != NONE) ++gstate.move_cnt;
			}
			gpad.prev_dir = (enum direction)input;
			std::string str = MOVE_CNT_STR;
			std::stringstream ss;
			ss << gstate.move_cnt;
			str += ss.str();
			ioctl(gpad.fd, IOCTL_SET_TEXT_LCD_NONBLOCK, str.c_str());
		}
	}
}

static void draw_frame(void){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	std::pair<float, float> pff;

	// tile
	for(int i = 0; i < gstate.maze.size(); ++i){
		for(int j = 0; j < gstate.maze[i].size(); ++j){
			if(gstate.maze[i][j] == 0) 
				continue;
			pff = maze_to_coord(i, j);
			m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(pff.first, pff.second, 0.0f));
			m_matrix = glm::scale(m_matrix, glm::vec3(2.0f*egl.width/WIDTH_RATIO/gstate.maze[0].size()/TILE_WIDTH, 
				2.0f*egl.height/HEIGHT_RATIO/gstate.maze.size()/TILE_HEIGHT, 1.0f));
			mvp_matrix = vp_matrix * m_matrix;
			glUniformMatrix4fv(g2_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
			tile_draw_wall();
		}
	}

	// goal point
	pff = maze_to_coord(gstate.goal_point.first, gstate.goal_point.second);
	m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(pff.first, pff.second, 0.0f));
	m_matrix = glm::scale(m_matrix, glm::vec3(2.0f*egl.width/WIDTH_RATIO/gstate.maze[0].size()/TILE_WIDTH, 
		2.0f*egl.height/HEIGHT_RATIO/gstate.maze.size()/TILE_HEIGHT, 1.0f));
	mvp_matrix = vp_matrix * m_matrix;
	glUniformMatrix4fv(g2_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
	tile_draw_goal();

	// androman
	pff = maze_to_coord(gstate.cur_androman.first, gstate.cur_androman.second);
	m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(pff.first, pff.second, 0.0f));
	m_matrix = glm::scale(m_matrix, glm::vec3(0.5f, 0.3f, 1.0f));
	mvp_matrix = vp_matrix * m_matrix;
	glUniformMatrix4fv(g2_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
	androman_draw();
}
static void* render_loop(void* nouse){
	acquire_context();

	// change screeen vertical to horizontal and normalize coords.
	vp_matrix = glm::ortho(-egl.width / WIDTH_RATIO, egl.width / WIDTH_RATIO, 
		-egl.height / HEIGHT_RATIO, egl.height / HEIGHT_RATIO, -1000.0f, 1000.0f);
	
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

void game2_create(std::vector<std::vector<int> > maze){
	gpad.fd = open("/dev/minigame", O_RDWR);
	if(gpad.fd == -1) LOG_ERROR("open error");
	else LOG_INFO("open success");
	std::string str = MOVE_CNT_STR;
	str += '0';
	ioctl(gpad.fd, IOCTL_RESET_TIMER_NONBLOCK);
	ioctl(gpad.fd, IOCTL_SET_TEXT_LCD_NONBLOCK, str.c_str());

	egl.exists_window = false;
	
	reset_ingame_attrib(maze);
}

void game2_destroy(void){
	std::vector<std::vector<int> > empty_maze;
	reset_ingame_attrib(empty_maze);
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

/*
 * The restart method is always called in the onPause state and before onResume.
 */
void game2_restart(std::vector<std::vector<int> > maze){
	reset_ingame_attrib(maze);
	
	std::string str = MOVE_CNT_STR;
	str += '0';
	ioctl(gpad.fd, IOCTL_RESET_TIMER_NONBLOCK);
	ioctl(gpad.fd, IOCTL_SET_TEXT_LCD_NONBLOCK, str.c_str());
}

/*
 * In a blocking manner, the back interrupt detector thread, which is passed in Java, 
 * detects the back button interrupt.
 */
bool game2_wait_back_interrupt(void){
	int waked_by_intr;
	ioctl(gpad.fd, IOCTL_WAIT_BACK_INTERRUPT, &waked_by_intr);
	if(waked_by_intr) {
		LOG_INFO("waked up by interrupt");
		return true;
	}
	LOG_INFO("waked up by pause");
	return false;
}

void game2_start(void){
	pthread_mutex_init(&gstate.mutex, 0);
}

void game2_stop(void){
	pthread_mutex_destroy(&gstate.mutex);
}