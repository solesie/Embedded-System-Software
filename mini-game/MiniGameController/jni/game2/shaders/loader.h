#ifndef SHADERS_LOADER_H
#define SHADERS_LOADER_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>

// handle to shader program
extern GLuint g1_shader_program;
// indices of uniform variables
extern GLint g1_loc_mvp_matrix, g1_loc_primitive_color, g1_loc_position;

void shaders_init(void);
void shaders_del(void);

#endif // SHADERS_LOADER_H