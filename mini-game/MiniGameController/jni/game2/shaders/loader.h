#ifndef GAME2_SHADERS_LOADER_H
#define GAME2_SHADERS_LOADER_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>

// handle to shader program
extern GLuint g2_shader_program;
// indices of uniform variables
extern GLint g2_loc_mvp_matrix, g2_loc_primitive_color, g2_loc_position;

void game2_shaders_init(void);
void game2_shaders_del(void);

#endif // GAME2_SHADERS_LOADER_H