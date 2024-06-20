#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include "../shaders/loader.h"
#include "../../logger.h"

#define BUFFER_OFFSET(offset) ((GLvoid*)(offset))

const static GLfloat TILE[4][2] = { {-12.0f, 6.0f},{-12.0f, -6.0f},{12.0f, -6.0f},{12.0f, 6.0f} };
const static GLfloat COLORS[3] = {0.0f, 0.0f, 0.0f};

static GLuint tile_vb;

void tile_prepare(void) {
	GLsizeiptr buffer_size = sizeof(TILE);

	glGenBuffers(1, &tile_vb);
	glBindBuffer(GL_ARRAY_BUFFER, tile_vb);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TILE), TILE);
}

void tile_draw(void){
	glBindBuffer(GL_ARRAY_BUFFER, tile_vb);
	glEnableVertexAttribArray(g2_loc_position);
	glVertexAttribPointer(g2_loc_position, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glUniform3fv(g2_loc_primitive_color, 1, COLORS);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(g2_loc_position);
}

void tile_release(void){
	glDeleteBuffers(1, &tile_vb);
}