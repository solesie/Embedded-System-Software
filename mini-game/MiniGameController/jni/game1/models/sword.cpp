#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <vector>
#include <math.h>
#include "../shaders/loader.h"
#include "../../logger.h"

#define BUFFER_OFFSET(offset) ((GLvoid*)(offset))

#define SWORD_BODY 0
#define SWORD_BODY2 1
#define SWORD_HEAD 2
#define SWORD_HEAD2 3
#define SWORD_IN 4
#define SWORD_DOWN 5
#define SWORD_BODY_IN 6

const static GLfloat BODY[4][2] = { { -6.0, 0.0 },{ -6.0, -4.0 },{ 6.0, -4.0 },{ 6.0, 0.0 } };
const static GLfloat BODY2[4][2] = { { -2.0, -4.0 },{ -2.0, -6.0 } ,{ 2.0, -6.0 },{ 2.0, -4.0 } };
const static GLfloat HEAD[4][2] = { { -2.0, 16.0 },{ -2.0, 0.0 } ,{ 2.0, 0.0 },{ 2.0, 16.0 } };
const static GLfloat HEAD2[3][2] = { { 0.0, 19.46 },{ -2.0, 16.0 },{ 2.0, 16.0 } };
const static GLfloat IN[4][2] = { { -0.3, 15.3 },{ -0.3, 0.7 } ,{ 0.3, 0.7 },{ 0.3, 15.3 } };
const static GLfloat DOWN[4][2] = { { -2.0, -6.0 },{ -4.0, -8.0 },{ 4.0, -8.0 },{ 2.0, -6.0 } };
const static GLfloat BODY_IN[4][2] = { { 0.0, -1.0 },{ -1.0, -2.732 },{ 0.0, -4.464 },{ 1.0, -2.732 } };

const static GLfloat COLORS[7][3] = {
    { 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
    { 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
    { 155 / 255.0f, 155 / 255.0f, 155 / 255.0f },
    { 155 / 255.0f, 155 / 255.0f, 155 / 255.0f },
    { 0 / 255.0f, 0 / 255.0f, 0 / 255.0f },
    { 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
    { 255 / 255.0f, 0 / 255.0f, 0 / 255.0f }
};

// eclipse figure
const static int A = 305;
const static int B = 50;
const static int SPEED = 3;
const static int Y_CENTER = 180;

struct sword{
    int initail_x;
	bool is_initially_front;
};

static GLuint sword_vb;
static std::vector<struct sword> swords;

/*
 * Sword moves along an elliptical path. 
 * Define functions to calculate the path on the ellipse.
 */
inline static float calc_eclipse_normed_fabs_y(const float cur_x){
    return sqrtf((-(float)B * B) / ((float)A * A) * (cur_x * cur_x) + (float)B * B);
}
static float calc_cur_x(const struct sword* s, const long long cur_time){
    const long long moved_dist = SPEED * cur_time;
    // bottom of eclipse
    if(!s->is_initially_front){
        const int dist_to_minus_a = A + s->initail_x;
        if(moved_dist - dist_to_minus_a < 0)
            return s->initail_x - moved_dist;
        const int remainder = (moved_dist - dist_to_minus_a) % (4*A);
        if(remainder >= 2*A)
            return A - remainder % (2*A);
        return -A + remainder % (2*A);
    }
    // top of eclipse
    const int dist_to_plus_a = A - s->initail_x;
    if(moved_dist - dist_to_plus_a < 0)
        return s->initail_x + moved_dist;
    const int remainder = (moved_dist - dist_to_plus_a) % (4*A);
    if(remainder >= 2*A)
        return -A + remainder % (2*A);
    return A - remainder % (2*A);
}
static float calc_cur_y(const struct sword* s, const long long cur_time) {
    const long long moved_dist = SPEED * cur_time;
    const float cur_x = calc_cur_x(s, cur_time);
    const float normed_fabs_y = calc_eclipse_normed_fabs_y(cur_x);
    // bottom of eclipse
    if (!s->is_initially_front) {
        const int dist_to_minus_a = A + s->initail_x;
        if(moved_dist - dist_to_minus_a < 0)
            return Y_CENTER + normed_fabs_y;
        const int remainder = (moved_dist - dist_to_minus_a) % (4*A);
        if(remainder >= 2*A)
            return Y_CENTER + normed_fabs_y;
        return Y_CENTER - normed_fabs_y;
    }
    // top of eclipse
    const int dist_to_plus_a = A - s->initail_x;
    if(moved_dist - dist_to_plus_a < 0)
        return Y_CENTER - normed_fabs_y;
    const int remainder = (moved_dist - dist_to_plus_a) % (4*A);
    if(remainder >= 2*A)
        return Y_CENTER - normed_fabs_y;
    return Y_CENTER + normed_fabs_y;
}

void create_swords(void){
	for (int i = 0; i <= 8; ++i) {
		swords.push_back({-304 + i * 76, true});
		swords.push_back({-304 + i * 76, false});
	}
}

void destroy_swords(void){
	swords.clear();
}

std::vector<std::pair<float, float> > get_cur_swords(const long long cur_time){
    std::vector<std::pair<float, float> > ret;
    for (int i = 0; i < swords.size(); ++i) 
        ret.push_back(std::make_pair(calc_cur_x(&swords[i], cur_time), calc_cur_y(&swords[i], cur_time)));
    return ret;
}

void init_swords(void) {
	GLsizeiptr buffer_size = sizeof(BODY) + sizeof(BODY2) + sizeof(HEAD) + sizeof(HEAD2) + sizeof(IN) + sizeof(DOWN) + sizeof(BODY_IN);

	glGenBuffers(1, &sword_vb);
	glBindBuffer(GL_ARRAY_BUFFER, sword_vb);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(BODY), BODY);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY), sizeof(BODY2), BODY2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(BODY2), sizeof(HEAD), HEAD);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(BODY2) + sizeof(HEAD), sizeof(HEAD2), HEAD2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(BODY2) + sizeof(HEAD) + sizeof(HEAD2), sizeof(IN), IN);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(BODY2) + sizeof(HEAD) + sizeof(HEAD2) + sizeof(IN), sizeof(DOWN), DOWN);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(BODY) + sizeof(BODY2) + sizeof(HEAD) + sizeof(HEAD2) + sizeof(IN) + sizeof(DOWN), sizeof(BODY_IN), BODY_IN);
}

void draw_swords(void) {
	glBindBuffer(GL_ARRAY_BUFFER, sword_vb);
	glEnableVertexAttribArray(g1_loc_position);
	glVertexAttribPointer(g1_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glUniform3fv(g1_loc_primitive_color, 1, COLORS[SWORD_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[SWORD_BODY2]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[SWORD_HEAD]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[SWORD_HEAD2]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 3);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[SWORD_IN]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 4);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[SWORD_DOWN]);
	glDrawArrays(GL_TRIANGLE_FAN, 19, 4);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[SWORD_BODY_IN]);
	glDrawArrays(GL_TRIANGLE_FAN, 23, 4);

	glDisableVertexAttribArray(g1_loc_position);
}

void del_swords(void){
	glDeleteBuffers(1, &sword_vb);
}