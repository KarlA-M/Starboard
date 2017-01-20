#ifndef STARBOARD_RENDER
#define STARBOARD_RENDER

#include <GL/glew.h>
#include <GL/gl.h>

#include "linmath/linmath.h"



/* TODO.
 */
typedef struct drawable
{
	GLuint*      shader;
	GLuint*      vao;
	GLuint*      ebo;
	GLuint*      vbo;
	GLuint*      cbo;
	GLuint*      ebo_len;
	GLint*       element_class;
	unsigned int n;
	
	vec4    model_matrix[4];
	GLfloat model_matrix_components[16];
} drawable_t;



/* TODO.
 */
void allocate_drawable_buffers(unsigned int n, drawable_t* draw)
{
	draw->shader        = (GLuint*)malloc(n * sizeof(GLuint));
	draw->vao           = (GLuint*)malloc(n * sizeof(GLuint));
	draw->ebo           = (GLuint*)malloc(n * sizeof(GLuint));
	draw->vbo           = (GLuint*)malloc(n * sizeof(GLuint));
	draw->cbo           = (GLuint*)malloc(n * sizeof(GLuint));
	draw->ebo_len       = (GLuint*)malloc(n * sizeof(GLuint)); 
	draw->element_class = (GLint* )malloc(n * sizeof(GLint ));
	draw->n             = n;
	
	static const GLfloat IDENTITY[16] = { \
		1.0, 0.0, 0.0, 0.0, \
		0.0, 1.0, 0.0, 0.0, \
		0.0, 0.0, 1.0, 0.0, \
		0.0, 0.0, 0.0, 1.0 \
	}; // Note that this will be 'transposed' when treated as a matrix, i.e. the first four elements are column 1.

	for (unsigned int i = 0; i < 4; ++i)
		for (unsigned int j = 0; j < 4; ++j)
			draw->model_matrix[i][j] = IDENTITY[4 * i + j];
	//memcpy(draw->model_matrix,            IDENTITY, 16 * sizeof(GLfloat));
	memcpy(draw->model_matrix_components, IDENTITY, 16 * sizeof(GLfloat));
}



/* TODO.
 */
void buffer_elements(GLuint* indices, unsigned int num_indices, \
                     GLuint* ebo_out)
{
	printf("[DEBUG] %s: (%i, %i, %i), (%i, %i, %i), (%i, %i, %i), (%i, %i, %i), (%i, %i, %i), ...\n", \
	       "Buffering indices", indices[0],  indices[1],  indices[2], indices[3], indices[4], indices[5], \
	                            indices[6],  indices[7],  indices[8], indices[9], indices[10], indices[11], \
	                            indices[12], indices[13], indices[14]);
	printf("[DEBUG] %s: %i.\n", "Triangles to render", num_indices / 3);
	printf("[DEBUG] %s: %i squares (%i segments).\n", "Shapes to render", num_indices / 6, num_indices / 12);
	
	// Generate the element buffer object (indices).
	glGenBuffers(1, ebo_out);
	GLuint ebo = *ebo_out;
	
	// Bind the element buffer and upload the data.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(GLuint), indices, GL_STATIC_DRAW);
}



/* Convert our data to vertex objects on the GPU.
 */
void buffer_vertices(GLfloat* vertices, unsigned int num_vertex_components, \
                     GLuint* vbo_out)
{
	printf("[DEBUG] %s: (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f), ...\n", \
	       "Buffering vertices", vertices[0], vertices[1], vertices[2], \
	                             vertices[3], vertices[4], vertices[5], \
	                             vertices[6], vertices[7], vertices[8]);
	printf("........%s. (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f), ...\n", \
	       "..................", vertices[9],  vertices[10], vertices[11], \
	                             vertices[12], vertices[13], vertices[14], \
	                             vertices[15], vertices[16], vertices[17]);
	printf("........%s. (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f), ...\n", \
	       "..................", vertices[18], vertices[19], vertices[20], \
	                             vertices[21], vertices[22], vertices[23], \
	                             vertices[24], vertices[25], vertices[26]);
	printf("[DEBUG] %s: %i vertex components (%i vertices).\n", \
	       "Vertices to render", num_vertex_components, num_vertex_components / 3);
	printf("[DEBUG] %s: %i.\n", "Triangles to render", 4 * num_vertex_components / 18);
	fflush(stdout);
	// Generate the vertex buffer object.
	glGenBuffers(1, vbo_out);
	GLuint vbo = *vbo_out;
	
	// Bind the vertex buffer and upload the data.
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, num_vertex_components * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
}



/* TODO.
 */
void buffer_colors(GLfloat* colors, unsigned int num_color_components, \
                   GLuint* cbo_out)
{
	// Generate a buffer to hold the colour of each vertex.
	glGenBuffers(1, cbo_out);
	GLuint cbo = *cbo_out;
	
	// Bind the colour buffer and upload the vertex colours.
	glBindBuffer(GL_ARRAY_BUFFER, cbo);
	glBufferData(GL_ARRAY_BUFFER, num_color_components * sizeof(GLfloat), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
}



/* TODO.
 */
void buffer_color_repeated(GLfloat* color, unsigned int n, \
                           GLuint* cbo_out)
{
	GLfloat colors_repeated[4 * n];
	unsigned int K = 0;
	for (unsigned int i = 0; i < n; ++i)
		for (unsigned int j = 0; j < 4; ++j)
			colors_repeated[K++] = color[j];
	buffer_colors(colors_repeated, 4 * n, cbo_out);
}



/* TODO.
 */
void mat4x4_to_GLfloat16(mat4x4 in, GLfloat out[16])
{
	for (unsigned int i = 0; i < 4; ++i)
		for (unsigned int j = 0; j < 4; ++j)
			out[4 * i + j] = (GLfloat)in[i][j];
}

#endif
