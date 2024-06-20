#include <stdio.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "loader.h"
#include "../../logger.h"

static const char* vshader = 
	"attribute vec4 a_position;\n"
	"uniform mat4 u_ModelViewProjectionMatrix;\n"
	"uniform vec3 u_primitive_color;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"	v_color = vec4(u_primitive_color, 1.0);\n"
	"	gl_Position = u_ModelViewProjectionMatrix * a_position;\n"
	"}\n";
static const char* fshader = 
	"precision mediump float;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"	gl_FragColor = v_color;\n"
	"}\n";

struct shader_info {
	GLenum type;
	const char* source;
	GLuint shader;
};
static struct shader_info shaders[3] = {
	{ GL_VERTEX_SHADER, vshader, 0 },
	{ GL_FRAGMENT_SHADER, fshader, 0 },
	{ GL_NONE, NULL, 0 }
};

static GLuint load_shaders() {
	if (shaders == NULL) return 0;

	GLuint program = glCreateProgram();

	struct shader_info* entry = shaders;
	while (entry->type != GL_NONE) {
		GLuint shader = glCreateShader(entry->type);
		entry->shader = shader;

		glShaderSource(shader, 1, &entry->source, NULL);

		glCompileShader(shader);

		GLint compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLsizei len;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

			GLchar* log = (GLchar*)malloc((len + 1) * sizeof(GLchar));
			glGetShaderInfoLog(shader, len, &len, log);
			LOG_INFO("Shader compilation failed");
			free(log);

			return 0;
		}

		glAttachShader(program, shader);
		++entry;
	}

	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLsizei len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = (GLchar*)malloc((len + 1) * sizeof(GLchar));
		glGetProgramInfoLog(program, len, &len, log);
		LOG_INFO("Shader linking fail");
		free(log);

		for (entry = shaders; entry->type != GL_NONE; ++entry) {
			glDeleteShader(entry->shader);
			entry->shader = 0;
		}
		return 0;
	}
	return program;
}

GLuint g1_shader_program;
GLint g1_loc_position, g1_loc_mvp_matrix, g1_loc_primitive_color;
void shaders_init(void) {
	g1_shader_program = load_shaders();

	glUseProgram(g1_shader_program);

	g1_loc_mvp_matrix = glGetUniformLocation(g1_shader_program, "u_ModelViewProjectionMatrix");
	g1_loc_primitive_color = glGetUniformLocation(g1_shader_program, "u_primitive_color");
	g1_loc_position = glGetAttribLocation(g1_shader_program, "a_position");

	glUseProgram(g1_shader_program);
	LOG_INFO("Init shader finish %d %d %d %d", g1_shader_program, g1_loc_mvp_matrix, g1_loc_primitive_color, g1_loc_position);
}

void shaders_del(void){
	if (g1_shader_program != 0) {
		glDeleteProgram(g1_shader_program);
		g1_shader_program = 0;
	}
	struct shader_info* entry = shaders;
	while(entry->type != GL_NONE){
		glDeleteShader(entry->shader);
		entry->shader = 0;
		++entry;
	}
}