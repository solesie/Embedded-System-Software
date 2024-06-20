#ifndef GAME1_SHADERS_LOADER_H
#define GAME1_SHADERS_LOADER_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>

// handle to shader program
extern GLuint g1_shader_program;
// indices of uniform variables
extern GLint g1_loc_mvp_matrix, g1_loc_primitive_color, g1_loc_position;

void game1_shaders_init(void);
void game1_shaders_del(void);

#endif // GAME1_SHADERS_LOADER_H