#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <vector>
#include <math.h>
#include "../shaders/loader.h"
#include "../../logger.h"

#define TO_RADIAN 0.01745329252f
#define BUFFER_OFFSET(offset) ((GLvoid*)(offset))

#define ANDROBOY_HEAD 0
#define ANDROBOY_LEFT_EYE 1
#define ANDROBOY_RIGHT_EYE 2
#define ANDROBOY_LEFT_ANTENNA 3
#define ANDROBOY_RIGHT_ANTENNA 4
#define ANDROBOY_BODY 5
#define ANDROBOY_LEFT_HAND 6
#define ANDROBOY_RIGHT_HAND 7
#define ANDROBOY_LEFT_FOOT 8
#define ANDROBOY_RIGHT_FOOT 9

/*
 * Androboy head and eyes are circle shape and have many vertices.
 * So these values are initailized at runtime but may consider as const variable.
 */
static GLfloat HEAD[180][2];
static GLfloat LEFT_EYE[360][2];
static GLfloat RIGHT_EYE[360][2];
const static GLfloat LEFT_ANTENNA[4][2] = { {-15.0845, 27.95838},{-16.84978, 27.13181},{-13.4, 21.1},{-12.0,21.95} };
const static GLfloat RIGHT_ANTENNA[4][2] = { {15.0845, 27.95838},{12.0,21.95},{13.4,21.1},{16.84978, 27.13181} };
const static GLfloat BODY[4][2] = { {-25.0,0},{-25.0,-45.0},{25.0,-45.0},{25.0,0.0} };
const static GLfloat LEFT_HAND[4][2] = { {-34.0,0.0},{-34.0,-31.0},{-28.0,-31.0},{-28.0,0.0} };
const static GLfloat RIGHT_HAND[4][2] = { {28.0,0.0},{28.0,-31.0},{34.0,-31.0},{34.0,0.0} };
const static GLfloat LEFT_FOOT[4][2] = { {-15.5,-45.0},{-15.5,-60.0},{-9.5,-60.0},{-9.5,-45.0} };
const static GLfloat RIGHT_FOOT[4][2] = { {9.5,-45.0},{9.5,-60.0},{15.5,-60.0},{15.5,-45.0} };
const static GLfloat COLORS[10][3] = {
    { 164 / 255.0f, 198 / 255.0f, 57 / 255.0f },
    { 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
    { 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
    { 164 / 255.0f, 198 / 255.0f, 57 / 255.0f },
    { 164 / 255.0f, 198 / 255.0f, 57 / 255.0f },
    { 164 / 255.0f, 198 / 255.0f, 57 / 255.0f },
    { 164 / 255.0f, 198 / 255.0f, 57 / 255.0f },
    { 164 / 255.0f, 198 / 255.0f, 57 / 255.0f },
    { 164 / 255.0f, 198 / 255.0f, 57 / 255.0f },
    { 164 / 255.0f, 198 / 255.0f, 57 / 255.0f }
};

static GLuint androboy_vb;

void init_androboy(void) {
    float head_rad = 25.0f, eye_rad = 3.0f;
    for(int i = 0; i < 360; ++i){
        if(i < 180){
            HEAD[i][0] = head_rad * cos(i * TO_RADIAN);
            HEAD[i][1] = head_rad * sin(i * TO_RADIAN) + 1.0f;
        }
        LEFT_EYE[i][0] = eye_rad * cos(i * TO_RADIAN) - 12.5f;
        LEFT_EYE[i][1] = eye_rad * sin(i * TO_RADIAN) + 12.5f;
        RIGHT_EYE[i][0] = eye_rad * cos(i * TO_RADIAN) + 12.5f;
        RIGHT_EYE[i][1] = eye_rad * sin(i * TO_RADIAN) + 12.5f;
    }

    GLsizeiptr buffer_size = sizeof(HEAD) + sizeof(LEFT_EYE) + sizeof(RIGHT_EYE) + sizeof(LEFT_ANTENNA) + sizeof(RIGHT_ANTENNA) 
        + sizeof(BODY) + sizeof(LEFT_HAND) + sizeof(RIGHT_HAND) + sizeof(LEFT_FOOT) + sizeof(RIGHT_FOOT);

    glGenBuffers(1, &androboy_vb);
    glBindBuffer(GL_ARRAY_BUFFER, androboy_vb);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(HEAD), HEAD);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD), sizeof(LEFT_EYE), LEFT_EYE);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD) + sizeof(LEFT_EYE), sizeof(RIGHT_EYE), RIGHT_EYE);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD) + sizeof(LEFT_EYE) + sizeof(RIGHT_EYE), sizeof(LEFT_ANTENNA), LEFT_ANTENNA);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD) + sizeof(LEFT_EYE) + sizeof(RIGHT_EYE) + sizeof(LEFT_ANTENNA),
        sizeof(RIGHT_ANTENNA), RIGHT_ANTENNA);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD) + sizeof(LEFT_EYE) + sizeof(RIGHT_EYE) + sizeof(LEFT_ANTENNA)
        + sizeof(RIGHT_ANTENNA), sizeof(BODY), BODY);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD) + sizeof(LEFT_EYE) + sizeof(RIGHT_EYE) + sizeof(LEFT_ANTENNA)
        + sizeof(RIGHT_ANTENNA) + sizeof(BODY), sizeof(LEFT_HAND), LEFT_HAND);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD) + sizeof(LEFT_EYE) + sizeof(RIGHT_EYE) + sizeof(LEFT_ANTENNA)
        + sizeof(RIGHT_ANTENNA) + sizeof(BODY) + sizeof(LEFT_HAND), sizeof(RIGHT_HAND), RIGHT_HAND);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD) + sizeof(LEFT_EYE) + sizeof(RIGHT_EYE) + sizeof(LEFT_ANTENNA)
        + sizeof(RIGHT_ANTENNA) + sizeof(BODY) + sizeof(LEFT_HAND) + sizeof(RIGHT_HAND), sizeof(LEFT_FOOT), LEFT_FOOT);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(HEAD) + sizeof(LEFT_EYE) + sizeof(RIGHT_EYE) + sizeof(LEFT_ANTENNA)
        + sizeof(RIGHT_ANTENNA) + sizeof(BODY) + sizeof(LEFT_HAND) + sizeof(RIGHT_HAND) + sizeof(LEFT_FOOT), 
        sizeof(RIGHT_FOOT), RIGHT_FOOT);
}

void draw_androboy_without_foots(void) {
    glBindBuffer(GL_ARRAY_BUFFER, androboy_vb);
    glEnableVertexAttribArray(g1_loc_position);
	glVertexAttribPointer(g1_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_HEAD]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 180);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_LEFT_EYE]);
    glDrawArrays(GL_TRIANGLE_FAN, 180, 360);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_RIGHT_EYE]);
    glDrawArrays(GL_TRIANGLE_FAN, 540, 360);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_LEFT_ANTENNA]);
    glDrawArrays(GL_TRIANGLE_FAN, 900, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_RIGHT_ANTENNA]);
    glDrawArrays(GL_TRIANGLE_FAN, 904, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_BODY]);
    glDrawArrays(GL_TRIANGLE_FAN, 908, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_LEFT_HAND]);
    glDrawArrays(GL_TRIANGLE_FAN, 912, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_RIGHT_HAND]);
    glDrawArrays(GL_TRIANGLE_FAN, 916, 4);

    glDisableVertexAttribArray(g1_loc_position);
}

void draw_androboy_left_foot(void) { 
    glBindBuffer(GL_ARRAY_BUFFER, androboy_vb);
    glEnableVertexAttribArray(g1_loc_position);
	glVertexAttribPointer(g1_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_LEFT_FOOT]);
    glDrawArrays(GL_TRIANGLE_FAN, 920, 4);

    glDisableVertexAttribArray(g1_loc_position);
}

void draw_androboy_right_foot(void) {
    glBindBuffer(GL_ARRAY_BUFFER, androboy_vb);
    glEnableVertexAttribArray(g1_loc_position);
	glVertexAttribPointer(g1_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_RIGHT_FOOT]);
    glDrawArrays(GL_TRIANGLE_FAN, 924, 4);

    glDisableVertexAttribArray(g1_loc_position);
}

void draw_androboy(void){
    glBindBuffer(GL_ARRAY_BUFFER, androboy_vb);
    glEnableVertexAttribArray(g1_loc_position);
	glVertexAttribPointer(g1_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_HEAD]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 180);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_LEFT_EYE]);
    glDrawArrays(GL_TRIANGLE_FAN, 180, 360);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_RIGHT_EYE]);
    glDrawArrays(GL_TRIANGLE_FAN, 540, 360);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_LEFT_ANTENNA]);
    glDrawArrays(GL_TRIANGLE_FAN, 900, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_RIGHT_ANTENNA]);
    glDrawArrays(GL_TRIANGLE_FAN, 904, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_BODY]);
    glDrawArrays(GL_TRIANGLE_FAN, 908, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_LEFT_HAND]);
    glDrawArrays(GL_TRIANGLE_FAN, 912, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_RIGHT_HAND]);
    glDrawArrays(GL_TRIANGLE_FAN, 916, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_LEFT_FOOT]);
    glDrawArrays(GL_TRIANGLE_FAN, 920, 4);
    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ANDROBOY_RIGHT_FOOT]);
    glDrawArrays(GL_TRIANGLE_FAN, 924, 4);

    glDisableVertexAttribArray(g1_loc_position);
}

void del_androboy(void){
    glDeleteBuffers(1, &androboy_vb);
}