#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "../shaders/loader.h"
#include "../../logger.h"

#define BUFFER_OFFSET(offset) ((GLvoid*)(offset))

#define HOUSE_ROOF 0
#define HOUSE_BODY 1
#define HOUSE_CHIMNEY 2
#define HOUSE_DOOR 3
#define HOUSE_WINDOW 4

const static GLfloat ROOF[3][2] = { { 0.0, 12.0 },{ -12.0, 0.0 },{ 12.0, 0.0 } };
const static GLfloat BODY[4][2] = { { -12.0, 0.0 },{ -12.0, -14.0 },{ 12.0, -14.0 },{ 12.0, 0.0 } };
const static GLfloat CHIMNEY[4][2] = { { 6.0, 14.0 },{ 6.0, 6.0 },{ 10.0, 2.0 },{ 10.0, 14.0 } };
const static GLfloat DOOR[4][2] = { { -8.0, -8.0 },{ -8.0, -14.0 },{ -4.0, -14.0 },{ -4.0, -8.0 } };
const static GLfloat WINDOW[4][2] = { { 4.0, -2.0 },{ 4.0, -6.0 },{ 8.0, -6.0 },{ 8.0, -2.0 } };

const static GLfloat COLORS[5][3] = {
	{ 200 / 255.0f, 39 / 255.0f, 42 / 255.0f },
	{ 235 / 255.0f, 225 / 255.0f, 196 / 255.0f },
	{ 255 / 255.0f, 0 / 255.0f, 0 / 255.0f },
	{ 233 / 255.0f, 113 / 255.0f, 23 / 255.0f },
	{ 44 / 255.0f, 180 / 255.0f, 49 / 255.0f }
};

static GLuint house_vb;

void house_prepare(void ) {
	GLsizeiptr buffer_size = sizeof(ROOF) + sizeof(BODY) + sizeof(CHIMNEY) + sizeof(DOOR)
		+ sizeof(WINDOW);

	glGenBuffers(1, &house_vb);
	glBindBuffer(GL_ARRAY_BUFFER, house_vb);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ROOF), ROOF);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ROOF), sizeof(BODY), BODY);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ROOF) + sizeof(BODY), sizeof(CHIMNEY), CHIMNEY);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ROOF) + sizeof(BODY) + sizeof(CHIMNEY), sizeof(DOOR), DOOR);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ROOF) + sizeof(BODY) + sizeof(CHIMNEY) + sizeof(DOOR),
		sizeof(WINDOW), WINDOW);
}

void house_draw(void) {
	glBindBuffer(GL_ARRAY_BUFFER, house_vb);
	glEnableVertexAttribArray(g1_loc_position);
	glVertexAttribPointer(g1_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glUniform3fv(g1_loc_primitive_color, 1, COLORS[HOUSE_ROOF]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[HOUSE_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 3, 4);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[HOUSE_CHIMNEY]);
	glDrawArrays(GL_TRIANGLE_FAN, 7, 4);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[HOUSE_DOOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 11, 4);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[HOUSE_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 4);

	glDisableVertexAttribArray(g1_loc_position);
}

void house_release(void){
	glDeleteBuffers(1, &house_vb);
}