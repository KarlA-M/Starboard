#include "ribbon.h"

static const unsigned int COUNT_PER_RESIDUE = 3; // Fixed by curve generation.



/* Convert an interpolated curve to an OpenGL representation of a ribbon.
 */
int curve_to_ribbon(vec4* p, unsigned int count, vec4* Z, float* T, \
                    GLfloat** vertices_out, unsigned int* num_components_out, \
                    GLuint** polygons_out, unsigned int* num_indices_out)
{
	// We could equally well work with GLfloat* -> vec4, but we prefer GLfloat* -> vec3 as it more efficiently
	// transfers data to the GPU (i.e. cuts data transfer by a quarter).
	#define SHIPYARD_RIBBON_COMPONENTS_PER_VERTEX 3
	
	printf("[DEBUG] %s: (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f), ...\n", \
	       "Curve positions", p[0][0], p[0][1], p[0][2], p[1][0], p[1][1], p[1][2]);
	
	// We transform points A, B, C, and . with half-way points - into points x..., y..., z... as follows.
	// x x xyy yzz z
	// A - B - C - .
	// x x xyy yzz z
	// First, calculate the total number of vertices and vertex components (vec3) needed.
	unsigned int num_residues = (count - 1) / (COUNT_PER_RESIDUE - 1);
	unsigned int num_vertices = 2 * count + 2 * (num_residues - 1); 
	*num_components_out = num_vertices * SHIPYARD_RIBBON_COMPONENTS_PER_VERTEX;
	unsigned int num_components = *num_components_out;
	// Calculate the number of indices needed. Each residue has 2 * COUNT_PER_RESIDUE vertices and therefore
	// has 2 * COUNT_PER_RESIDUE - 2 triangles, which need three indices each, so 3 * (2 * COUNT_PER_RESIDUE - 2).
	*num_indices_out = num_residues * (3 * (2 * COUNT_PER_RESIDUE - 2));
	unsigned int num_indices = *num_indices_out;
	// Create space in memory to hold the new vertices and polygon indices. 
	*vertices_out = (GLfloat*)malloc(num_components * sizeof(GLfloat));
	*polygons_out = (GLuint*)malloc(num_indices * sizeof(GLuint));
	GLfloat* vert = *vertices_out;
	GLuint*  poly = *polygons_out;
	
	// The thickness array might be null, indicating uniform thickness.
	float* U;
	if (T == NULL)
		U = (float*)calloc(num_vertices, sizeof(float)); // malloc U
	else
		U = T;
	
	// Iterate over every point in the input curve, grouped by residue. 
	vec4 E_Z, A, B;
	unsigned int K = 0; // Working value of current vertex component in vert[].
	float pitch;
	for (unsigned int i = 0; i < count - 1; i += COUNT_PER_RESIDUE - 1) // It's count - 1 to skip the very last
	                                                                    // residue start (it's just the end),
	                                                                    // and COUNT_PER_RESIDUE - 1 so that
	                                                                    // the shared end-of/start-of point
	                                                                    // is included twice correctly.
		for (unsigned int j = 0; j < COUNT_PER_RESIDUE; ++j)
		{
			//printf("[DEBUG] (i = %i) + (j = %i) = %i.\n", i, j, i + j);
			//printf("[DEBUG] p[i + j] = %.1f, %.1f, %.1f.\n", p[i + j][0], p[i + j][1], p[i + j][2]);
			//printf("[DEBUG] Z[i + j] = %.1f, %.1f, %.1f.\n", Z[i + j][0], Z[i + j][1], Z[i + j][2]);
			
			// Into A and B, put the point plus & minus the arc normal, to give the ribbon thickness.
			pitch = 0.75 + U[i + j];
			//printf("[DEBUG] pitch = %.2f.\n", pitch);
			//printf("[DEBUG] Z = %.1f, %.1f, %.1f.\n", Z[i + j][0], Z[i + j][1], Z[i + j][2]);
			vec4_scale(E_Z, Z[i + j], pitch);
			vec4_add(A, p[i + j], E_Z);
			vec4_sub(B, p[i + j], E_Z);
			if (K > 0)
			{
				vec4 previous;
				previous[0] = vert[K - SHIPYARD_RIBBON_COMPONENTS_PER_VERTEX];
				previous[1] = vert[K - SHIPYARD_RIBBON_COMPONENTS_PER_VERTEX + 1];
				previous[2] = vert[K - SHIPYARD_RIBBON_COMPONENTS_PER_VERTEX + 2];
				previous[3] = 0.0;
				vec4 previous_to_A, previous_to_B;
				vec4_sub(previous_to_A, A, previous);
				vec4_sub(previous_to_B, B, previous);
				if (vec3_len(previous_to_B) > vec3_len(previous_to_A))
				{
					vec4 tmp;
					memcpy(tmp, A,   sizeof(vec4));
					memcpy(A,   B,   sizeof(vec4));
					memcpy(B,   tmp, sizeof(vec4));
				}
			}
			// Put point A into the vertex list, then point B. 
			for (unsigned int k = 0; k < 3; ++k)
				vert[K++] = A[k];
			// HACK. We use preprocessor directives to avoid branching in this foreach i, j loop. 
			#if SHIPYARD_VERTEX_COMPONENTS_PER_VERTEX == 4
				vert[K++] = 1.0;
			#endif
			for (unsigned int k = 0; k < 3; ++k)
				vert[K++] = B[k];
			#if SHIPYARD_VERTEX_COMPONENTS_PER_VERTEX == 4
				vert[K++] = 1.0;
			#endif
		}
	printf("[DEBUG] Wrote %i vertex components, i.e. %i vertices.\n", \
	       K, K / SHIPYARD_RIBBON_COMPONENTS_PER_VERTEX);
	
	// If we calloc'd a thickness array, free it.
	if (T == NULL)
		free(U);
	
	// Basically, the offset at each residue is half of the length of the edge pattern * i.
	// We could potentially calculate this, but it's *much* easier and more efficient to just hard-code it.
	#define STARBOARD_RIBBON_EDGE_PATTERN_LENGTH 12
	static const GLuint EDGE_PATTERN[STARBOARD_RIBBON_EDGE_PATTERN_LENGTH] = \
	                    {0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4};
	K = 0;
	unsigned int offset;
	for (unsigned int i = 0; i < num_residues; ++i)
	{
		offset = STARBOARD_RIBBON_EDGE_PATTERN_LENGTH * i / 2;
		for (unsigned int k = 0; k < STARBOARD_RIBBON_EDGE_PATTERN_LENGTH; k++)
			poly[K++] = offset + EDGE_PATTERN[k];
	}
	printf("[DEBUG] Wrote %i (out of %i) indices, i.e. %i polygons, i.e. %i squares, i.e. %i residues.\n", \
	       K, num_indices, K / 3, K / 6, K / (6 * (COUNT_PER_RESIDUE - 1)));
	return num_vertices;
}



/* TODO.
 */
void ribbon_to_outline(unsigned int num_vertices, \
                       GLuint** poly_out, unsigned int* num_indices)
{
	*num_indices = num_vertices + 1;
	*poly_out = (GLuint*)malloc(*num_indices * sizeof(GLuint));
	GLuint* poly = *poly_out;
	// Essentially, we count up the even vertices (one edge), then down in odd vertices, then join the ends.
	unsigned int K = 0;
	for (unsigned int i = 0; i < num_vertices; i += 2)
		poly[K++] = i;
	for (unsigned int i = num_vertices; i > 0; i -= 2)
		poly[K++] = i - 1;
	poly[K++] = 0;
}



/* TODO.
 */
void residue_colors_to_vertex_colors(vec4* colors, unsigned int num_residues, \
                                     GLfloat** color_components_out, unsigned int* num_color_components_out)
{
	// Calculate the total number of RGBA components needed to store colour information about every vertex
	// in the ribbon.
	*num_color_components_out = num_residues * COUNT_PER_RESIDUE * 2 * 4;
	unsigned int num_color_components = *num_color_components_out;
	
	// Allocate memory for the components.
	*color_components_out = (GLfloat*)malloc(num_color_components * sizeof(GLfloat));
	GLfloat* color_components = *color_components_out;
	
	// Set all of the colour components.
	unsigned int K = 0;
	for (unsigned int i = 0; i < num_residues; ++i)
		for (unsigned int j = 0; j < COUNT_PER_RESIDUE * 2; ++j)
			for (unsigned int k = 0; k < 4; ++k)
				color_components[K++] = colors[i][k];
}
