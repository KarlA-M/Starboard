#include "render.h"



/* This data type defines all the elements needed to construct a visual representation of a 
 * monomeric protein structure. 
 */
typedef struct monoview
{
	char*     name;
	chain_t   chain;
	curve_t   curve;
	ribbon2_t ribbon;
} monoview_t;



/* TODO.
 */
int monoview_to_drawable(monoview_t* view, drawable_t* draw)
{
	if (draw->n != 2) return -1;
	
	// Create a new VAO, then bind and upload the VBO, CBO, and EBO. 
	glGenVertexArrays(1, &draw->vao[0]);
	glBindVertexArray(draw->vao[0]);
	buffer_elements(view->ribbon.element_components, view->ribbon.num_element_components, \
	                &draw->ebo[0]);
	draw->ebo_len[0] = view->ribbon.num_element_components;
	buffer_vertices(view->ribbon.vertex_components, view->ribbon.num_vertex_components, \
	                &draw->vbo[0]);
	buffer_colors(view->ribbon.vertex_color_components, view->ribbon.num_vertex_color_components, \
	              &draw->cbo[0]);
	draw->element_class[0] = GL_TRIANGLES;
	glBindVertexArray(0);
	
	// Create a VAO for the outline too.
	glGenVertexArrays(1, &draw->vao[1]);
	glBindVertexArray(draw->vao[1]);
	buffer_elements(view->ribbon.outline_element_components, view->ribbon.num_outline_element_components, \
	                &draw->ebo[1]);
	draw->ebo_len[1] = view->ribbon.num_outline_element_components;
	draw->vbo[1] = draw->vbo[0];
	glBindBuffer(GL_ARRAY_BUFFER, draw->vbo[1]);
	buffer_colors(view->ribbon.outline_color_components, view->ribbon.num_outline_color_components, \
	              &draw->cbo[1]);
	draw->element_class[1] = GL_LINE_STRIP;
	glBindVertexArray(0);
	
	return 0;
}
