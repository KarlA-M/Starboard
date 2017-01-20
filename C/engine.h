#ifndef STARBOARD_ENGINE
#define STARBOARD_ENGINE

#include <iso646.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glxew.h>
#include <GLFW/glfw3.h>

#include "linmath/linmath.h"


/* A data structure representing a framebuffer to render into. 
 */
typedef struct framebuffer
{
	GLuint       fbo, canvas, rbo;
	unsigned int multisamples;
	unsigned int canvas_width, canvas_height;
} framebuffer_t;



framebuffer_t* Framebuffer;
extern void    engine_use_framebuffer(const framebuffer_t* f);

GLuint      Shader;
extern void engine_use_shader(const GLuint);

extern void engine_initialize(void);
extern int  engine_create_framebuffer(const unsigned int, const unsigned int, const unsigned int, \
                                      framebuffer_t*);
extern void engine_destroy_framebuffer(framebuffer_t*);

extern void engine_begin_frame(const vec4);
extern void engine_end_frame(const unsigned int);

#endif
