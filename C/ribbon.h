#include "linmath/linmath.h"

#include <iso646.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tgmath.h>
#include <GL/glew.h>
#include <GL/gl.h>

typedef struct ribbon2
{
	unsigned int num_vertices;
	GLfloat*     vertex_components;
	unsigned int num_vertex_components;
	
	GLuint*      element_components;
	unsigned int num_element_components;
	
	vec4*        residue_colors;
	GLfloat*     vertex_color_components;
	unsigned int num_vertex_color_components;
	
	GLuint*      outline_element_components;
	unsigned int num_outline_element_components;
	
	vec4*        outline_colors;
	GLfloat*     outline_color_components;
	unsigned int num_outline_color_components;
} ribbon2_t;



//
// Take a smooth arc-based curve (with an arc radius and normal at each point) and convert it to a ribbon
// representation that is thicker when the curve's curvature is higher.
//

extern int curve_to_ribbon(vec4*, unsigned int, vec4*, float*, \
                           GLfloat**, unsigned int*, GLuint**, unsigned int*);

extern void ribbon_to_outline(unsigned int num_vertices, \
                              GLuint** poly_out, unsigned int* num_indices);

extern void residue_colors_to_vertex_colors(vec4*, unsigned int, \
                                            GLfloat**, unsigned int*);
