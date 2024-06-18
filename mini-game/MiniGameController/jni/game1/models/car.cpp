#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <list>
#include <random>
#include "../shaders/loader.h"
#include "../../logger.h"

#define BUFFER_OFFSET(offset) ((GLvoid*)(offset))

#define CAR_BODY 0
#define CAR_FRAME 1
#define CAR_WINDOW 2
#define CAR_LEFT_LIGHT 3
#define CAR_RIGHT_LIGHT 4
#define CAR_LEFT_WHEEL 5
#define CAR_RIGHT_WHEEL 6

const static int CAR_INITIAL_X = 1000;

const static GLfloat BODY[4][2] = { { -16.0, 0.0 },{ -16.0, -8.0 },{ 16.0, -8.0 },{ 16.0, 0.0 } };
const static GLfloat FRAME[4][2] = { { -10.0, 10.0 },{ -10.0, 0.0 },{ 10.0, 0.0 },{ 10.0, 10.0 } };
const static GLfloat WINDOW[4][2] = { { -8.0, 8.0 },{ -8.0, 0.0 },{ 8.0, 0.0 },{ 8.0, 8.0 } };
const static GLfloat LEFT_LIGHT[4][2] = { { -9.0, -4.0 },{ -10.0, -5.0 },{ -9.0, -6.0 },{ -8.0, -5.0 } };
const static GLfloat RIGHT_LIGHT[4][2] = { { 9.0, -4.0 },{ 8.0, -5.0 },{ 9.0, -6.0 },{ 10.0, -5.0 } };
const static GLfloat LEFT_WHEEL[4][2] = { { -10.0, -8.0 },{ -10.0, -12.0 },{ -6.0, -12.0 },{ -6.0, -8.0 } };
const static GLfloat RIGHT_WHEEL[4][2] = { { 6.0, -8.0 },{ 6.0, -12.0 },{ 10.0, -12.0 },{ 10.0, -8.0 } };

const static GLfloat COLORS[7][3] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 216 / 255.0f, 208 / 255.0f, 174 / 255.0f },
    { 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
    { 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
    { 21 / 255.0f, 30 / 255.0f, 26 / 255.0f },
    { 21 / 255.0f, 30 / 255.0f, 26 / 255.0f }
};

struct car{
    long long created_at;
    float velocity;
    int line;
};
static std::list<struct car> cars;
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> line_dist_4(0, 3); // 4 lines
static std::uniform_int_distribution<> vel_dist_5(0, 4); // 5 velocity
static GLuint car_vb;

void add_new_car(long long cur_time){
    cars.push_back({cur_time, 4.0f*(vel_dist_5(gen)+1), line_dist_4(gen)});
}

void destroy_cars(void){
	cars.clear();
}

void remove_out_of_bound_cars(long long cur_time){
    for (auto it = cars.begin(); it != cars.end();) {
        if (2*CAR_INITIAL_X <= it->velocity * (cur_time - it->created_at))
            it = cars.erase(it);
        else
            ++it;
    }
}

/*
 * If there is crushed car, remove the car from list and initialize (x,y) at crushed_car
 * and return true.
 */
bool remove_crushed_car(long long cur_time, int human_cur_line, int human_cur_rightest, int human_cur_leftest, 
        std::pair<float, float>* crushed_car) {
    int car_rightest, car_leftest;
    int moved_dist;
    for (auto it = cars.begin(); it != cars.end();) {
        moved_dist = it->velocity * (cur_time - it->created_at);
        car_rightest = CAR_INITIAL_X - moved_dist + 16;
        car_leftest = CAR_INITIAL_X - moved_dist - 16;
        if (it->line == human_cur_line && 
            ((human_cur_leftest <= car_rightest && human_cur_rightest > car_leftest) || 
                (human_cur_rightest >= car_leftest && human_cur_leftest < car_rightest))){
            crushed_car->first = CAR_INITIAL_X - moved_dist;
            crushed_car->second = -110 - (it->line * 67.5f);
            it = cars.erase(it);
            return true;
        }
        else{
            ++it;
        }
    }
    return false;
}

/*
 * (x, y)
 */
std::vector<std::pair<float, float> > get_cur_cars(long long cur_time){
    std::vector<std::pair<float, float> > ret;
    for(auto& c : cars)
        ret.push_back(std::make_pair(CAR_INITIAL_X - c.velocity * (cur_time - c.created_at), -110 - (c.line * 67.5)));
    return ret;
}

void init_car(void) {
    GLsizeiptr buffer_size = sizeof(BODY) + sizeof(FRAME) + sizeof(WINDOW) + sizeof(LEFT_LIGHT)
        + sizeof(RIGHT_LIGHT) + sizeof(LEFT_WHEEL) + sizeof(RIGHT_WHEEL);

    glGenBuffers(1, &car_vb);
    glBindBuffer(GL_ARRAY_BUFFER, car_vb);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(BODY), BODY);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY), sizeof(FRAME), FRAME);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(FRAME), sizeof(WINDOW), WINDOW);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(FRAME) + sizeof(WINDOW), sizeof(LEFT_LIGHT), LEFT_LIGHT);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(FRAME) + sizeof(WINDOW) + sizeof(LEFT_LIGHT),
        sizeof(RIGHT_LIGHT), RIGHT_LIGHT);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(FRAME) + sizeof(WINDOW) + sizeof(LEFT_LIGHT)
        + sizeof(RIGHT_LIGHT), sizeof(LEFT_WHEEL), LEFT_WHEEL);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(FRAME) + sizeof(WINDOW) + sizeof(LEFT_LIGHT)
        + sizeof(RIGHT_LIGHT) + sizeof(LEFT_WHEEL), sizeof(RIGHT_WHEEL), RIGHT_WHEEL);
}

void draw_car(void) {
    glBindBuffer(GL_ARRAY_BUFFER, car_vb);
    glEnableVertexAttribArray(g1_loc_position);
    glVertexAttribPointer(g1_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glUniform3fv(g1_loc_primitive_color, 1, COLORS[CAR_BODY]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[CAR_FRAME]);
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[CAR_WINDOW]);
    glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[CAR_LEFT_LIGHT]);
    glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[CAR_RIGHT_LIGHT]);
    glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[CAR_LEFT_WHEEL]);
    glDrawArrays(GL_TRIANGLE_FAN, 20, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[CAR_RIGHT_WHEEL]);
    glDrawArrays(GL_TRIANGLE_FAN, 24, 4);

    glDisableVertexAttribArray(g1_loc_position);
}

void del_car(void){
	glDeleteBuffers(1, &car_vb);
}