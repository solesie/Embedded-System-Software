#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "../shaders/loader.h"
#include "../../logger.h"

#define BUFFER_OFFSET(offset) ((GLvoid*)(offset))

#define ROAD_ROAD 0
#define ROAD_LINE1 1
#define ROAD_LINE2 2
#define ROAD_LINE3 3

static const GLfloat ROAD[4][2] = { {-1000.0, -80.0}, {-1000.0, -350.0}, {1000.0, -350.0}, {1000.0, -80.0} };
static const GLfloat LINE1[2][2] = { {-1000.0, -147.5}, {1000.0, -147.5} };
static const GLfloat LINE2[2][2] = { {-1000.0, -215.0}, {1000.0, -215.0} };
static const GLfloat LINE3[2][2] = { {-1000.0, -282.5}, {1000.0, -282.5} };

static const GLfloat COLORS[4][3] = {
	{ 104 / 255.0f, 108 / 255.0f,  94 / 255.0f }, //ROAD
	{ 235 / 255.0f, 236 / 255.0f, 240 / 255.0f }, //LINE1
	{ 235 / 255.0f, 236 / 255.0f, 240 / 255.0f }, //LINE2
	{ 235 / 255.0f, 236 / 255.0f, 240 / 255.0f }  //LINE3
};

static GLuint road_vb;

void init_road(void) {
	GLsizeiptr buffer_size = sizeof(ROAD) + sizeof(LINE1) + sizeof(LINE2) + sizeof(LINE3);

	glGenBuffers(1, &road_vb);
	glBindBuffer(GL_ARRAY_BUFFER, road_vb);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ROAD), ROAD);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ROAD), sizeof(LINE1), LINE1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ROAD) + sizeof(LINE1), sizeof(LINE2), LINE2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ROAD) + sizeof(LINE1) + sizeof(LINE2), sizeof(LINE3), LINE3);
}

void draw_road(void) {
	glBindBuffer(GL_ARRAY_BUFFER, road_vb);
	glEnableVertexAttribArray(g1_loc_position);
    glVertexAttribPointer(g1_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glUniform3fv(g1_loc_primitive_color, 1, COLORS[ROAD_ROAD]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glLineWidth(6.0);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[ROAD_LINE1]);
	glDrawArrays(GL_LINES, 4, 2);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[ROAD_LINE2]);
	glDrawArrays(GL_LINES, 6, 2);
	glUniform3fv(g1_loc_primitive_color, 1, COLORS[ROAD_LINE3]);
	glDrawArrays(GL_LINES, 8, 2);
	glLineWidth(1.0);

	glDisableVertexAttribArray(g1_loc_position);
}

void del_road(void){
	glDeleteBuffers(1, &road_vb);
}