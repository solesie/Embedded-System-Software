#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <EGL/egl.h>
#include <GLES/gl.h>
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
#include "logger.h"

#define TO_RADIAN 0.01745329252f
/*
 * game1 driver module arributes
 */
#define GAME1_MAJOR 242
#define IOCTL_RUN_NONBLOCK _IO(GAME1_MAJOR, 0)
#define IOCTL_STOP_NONBLOCK _IO(GAME1_MAJOR, 1)
enum direction{
    NONE = 0,
    UP,
    LEFT,
    RIGHT,
    DOWN
};

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
 * The context being current applies to a thread, so different threads can have different current contexts.
 */
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
        del_egl();
        return;
    }
    if (!eglGetConfigAttrib(egl.display, egl.config, EGL_NATIVE_VISUAL_ID, &egl.format)) {
        LOG_ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
        del_egl();
        return;
    }
    ANativeWindow_setBuffersGeometry(egl.window, 0, 0, egl.format);
    if (!(egl.surface = eglCreateWindowSurface(egl.display, egl.config, egl.window, 0))) {
        LOG_ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
        del_egl();
        return;
    }
    if (!(egl.context = eglCreateContext(egl.display, egl.config, NULL, context_attribs))) {
        LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
        del_egl();
        return;
    }
    if (!eglMakeCurrent(egl.display, egl.surface, egl.surface, egl.context)) {
        LOG_ERROR("eglMakeCurrent() returned error %d", eglGetError());
        del_egl();
        return;
    }
    if (!eglQuerySurface(egl.display, egl.surface, EGL_WIDTH, &egl.width) ||
        !eglQuerySurface(egl.display, egl.surface, EGL_HEIGHT, &egl.height)) {
        LOG_ERROR("eglQuerySurface() returned error %d", eglGetError());
        del_egl();
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
    init_shaders();

    // prepare models
    init_road();
    init_sword();
    init_car();
    init_house();
    init_androboy();
}
static void release_context(){
    del_road();
    del_sword();
    del_car();
    del_house();
    del_androboy();

    del_shaders();

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

/*
 * Read fpga push switch from game1 driver and
 * change gstate androboy coords.
 */
static void read_gpad(){
    enum direction input;
    if(read(gpad.fd, &input, 1) != 1) return;

    if(input == LEFT || input == RIGHT){

    }
}

static void draw_frame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // road
    m_matrix = glm::mat4(1.0f);
    mvp_matrix = vp_matrix * m_matrix;
    glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
    draw_road();

    // superman androboy
    int superman_androboy_x = (gstate.cur_time % 1201) - 600; // -600 <= superman_androboy_x <= 600
    if (superman_androboy_x <= 200) {
        //y=0.4x+250
        m_matrix = glm::translate(m_matrix, glm::vec3(superman_androboy_x, 0.4f * superman_androboy_x + 250, 0.0f));
        m_matrix = glm::rotate(m_matrix, -90 * TO_RADIAN + atanf(0.4), glm::vec3(0.0f, 0.0f, 1.0f));
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
    draw_androboy();

    // unsigned char input;
    // int len = read(gpad.fd, &input, 1);
    // LOG_INFO("%d %d", len, (int)input);

    if (gstate.cur_time % 50 == 0) 
        add_new_car(gstate.cur_time);
    remove_out_of_bound_cars(gstate.cur_time);

    if(!gstate.gameover){
        // house
        gstate.house_x = (gstate.cur_time % 3202) / 2 - 800; // -800 <= house_x <= 800
        m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f*gstate.house_x, -10.0f, 0.0f));
        m_matrix = glm::scale(m_matrix, glm::vec3(50.0f, 5.0f, 1.0f));
        mvp_matrix = vp_matrix * m_matrix;
        glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
        draw_house();

        // androboy
        m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(gstate.androboy_cur_x, -90 - (gstate.androboy_cur_line * 67.5), 0.0f));
        m_matrix = glm::scale(m_matrix, glm::vec3(0.9f, 0.9f, 1.0f));
        mvp_matrix = vp_matrix * m_matrix;
        glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
        draw_androboy_without_foots();
        int foot_clock = gstate.cur_time % 80;
        if (foot_clock < 40) {
            draw_androboy_right_foot();
            m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(gstate.androboy_cur_x, -90 - (gstate.androboy_cur_line * 67.5) + 0.25f * foot_clock, 0.0f));
            m_matrix = glm::scale(m_matrix, glm::vec3(0.9f, 0.9f, 1.0f));
            mvp_matrix = vp_matrix * m_matrix;
            glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
            draw_androboy_left_foot();
        }
        else {
            draw_androboy_left_foot();
            m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(gstate.androboy_cur_x, -90 - (gstate.androboy_cur_line * 67.5) + 0.25f * (foot_clock - 40), 0.0f));
            m_matrix = glm::scale(m_matrix, glm::vec3(0.9, 0.9, 1.0f));
            mvp_matrix = vp_matrix * m_matrix;
            glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
            draw_androboy_right_foot();
        }

        // swords
        std::vector<std::pair<float, float> > swords = get_cur_swords(gstate.cur_time);
        for (int i = 0; i < swords.size(); ++i) {
            m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(swords[i].first, swords[i].second, 0.0f));
            m_matrix = glm::scale(m_matrix, glm::vec3(2.2, 6.5, 1.0f));
            mvp_matrix = vp_matrix * m_matrix;
            glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
            draw_sword();
        }
        // cars
        std::vector<std::pair<float, float> > cars = get_cur_cars(gstate.cur_time);
        for (int i = 0; i < cars.size(); ++i) {
            m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(cars[i].first, cars[i].second, 0.0f));
            m_matrix = glm::scale(m_matrix, glm::vec3(2.0, 2.0, 1.0f));
            mvp_matrix = vp_matrix * m_matrix;
            glUniformMatrix4fv(g1_loc_mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
            draw_car();
        }
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
        draw_frame();
        if (!eglSwapBuffers(egl.display, egl.surface)) {
            LOG_ERROR("eglSwapBuffers() returned error %d", eglGetError());
        }
        ++gstate.cur_time;
        usleep(10000); // 10ms
        pthread_mutex_unlock(&gstate.mutex);
    }
}

void create_game1(){
    gpad.fd = open("/dev/game1", O_RDWR);
	if(gpad.fd == -1) LOG_ERROR("open error");
    else LOG_INFO("open success");

    egl.exists_window = false;
    
    pthread_mutex_init(&gstate.mutex, 0);
    gstate.is_paused = true;
    gstate.androboy_cur_line = 1;
    gstate.androboy_cur_x = -300;
    gstate.androboy_cur_rightest = gstate.androboy_cur_x + 34;
    gstate.androboy_cur_leftest = gstate.androboy_cur_x - 34;
    gstate.cur_time = 0;
    gstate.gameover = false;

    create_swords();
}

void destroy_game1(){
    pthread_mutex_destroy(&gstate.mutex);
    gstate.is_paused = true;
    gstate.androboy_cur_line = 1;
    gstate.androboy_cur_x = -500;
    gstate.androboy_cur_rightest = gstate.androboy_cur_x + 34;
    gstate.androboy_cur_leftest = gstate.androboy_cur_x - 34;
    gstate.cur_time = 0;
    gstate.gameover = false;

    destroy_swords();
    destroy_cars();

    close(gpad.fd);
}

void del_egl(void){
    ANativeWindow_release(egl.window);
    egl.window = NULL;
    egl.exists_window = false;
}

void init_egl(ANativeWindow* window){
    if(egl.exists_window) return;

    LOG_INFO("Init egl");

    egl.window = window;
    egl.exists_window = true;

    pthread_create(&gstate.tid, NULL, render_loop, NULL);
    return;
}

void on_resume(void){
    pthread_mutex_lock(&gstate.mutex);
    gstate.is_paused = false;
    pthread_mutex_unlock(&gstate.mutex);

    // At most onResume() -> surfaceCreated() -> onPause() -> surfaceDestroyed().
    if(egl.exists_window)
        pthread_create(&gstate.tid, NULL, render_loop, NULL);
    else
        LOG_INFO("First onResume()");

    ioctl(gpad.fd, IOCTL_RUN_NONBLOCK);
}

void on_pause(void){
    pthread_mutex_lock(&gstate.mutex);
    gstate.is_paused = true;
    pthread_mutex_unlock(&gstate.mutex);

    pthread_join(gstate.tid, 0);
    
    ioctl(gpad.fd, IOCTL_STOP_NONBLOCK);
}