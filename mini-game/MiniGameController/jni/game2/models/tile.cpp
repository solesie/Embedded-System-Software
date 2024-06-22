#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include "tile.h"
#include "../shaders/loader.h"
#include "../../logger.h"

#define BUFFER_OFFSET(offset) ((GLvoid*)(offset))

#define TILE_BLACK 0
#define TILE_RED 1

const static GLfloat TILE[4][2] = { {-TILE_WIDTH/2, TILE_HEIGHT/2},{-TILE_WIDTH/2, -TILE_HEIGHT/2},
	{TILE_WIDTH/2, -TILE_HEIGHT/2},{TILE_WIDTH/2, TILE_HEIGHT/2} };
const static GLfloat COLORS[2][3] = {
	{0.0f, 0.0f, 0.0f},
	{220 / 255.0f, 20 / 255.0f, 60 / 255.0f}
};

static GLuint tile_vb;

void tile_prepare(void) {
	GLsizeiptr buffer_size = sizeof(TILE);

	glGenBuffers(1, &tile_vb);
	glBindBuffer(GL_ARRAY_BUFFER, tile_vb);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TILE), TILE);
}

void tile_draw_wall(void){
	glBindBuffer(GL_ARRAY_BUFFER, tile_vb);
	glEnableVertexAttribArray(g2_loc_position);
	glVertexAttribPointer(g2_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glUniform3fv(g2_loc_primitive_color, 1, COLORS[TILE_BLACK]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(g2_loc_position);
}

void tile_draw_goal(void){
	glBindBuffer(GL_ARRAY_BUFFER, tile_vb);
	glEnableVertexAttribArray(g2_loc_position);
	glVertexAttribPointer(g2_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glUniform3fv(g2_loc_primitive_color, 1, COLORS[TILE_RED]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(g2_loc_position);
}

void tile_release(void){
	glDeleteBuffers(1, &tile_vb);
}