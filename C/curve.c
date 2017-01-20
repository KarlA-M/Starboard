#include "curve.h"



/* Take three 3D points, A, B, C, and output O, the centre of the circle that intersects them, and R, the radius.
 */
int three_points_arc(vec4 A, vec4 B, vec4 C, vec4* O, float* R, vec4* Z)
{
	static const vec4 UP = {0.0, 0.0, 1.0, 0.0}; // This is a default orientation for the ribbon on error.
	
	// We identify the basis of the plane of A, B, C, such that A_3 = B_3 = C_3 = 0.
	vec4 AB, BC; // The displacement vectors taking us around the arc.
	vec4_sub(AB, B, A);
	vec4_sub(BC, C, B);
	vec4 E_AB, E_ABxBC, E_ABxABxBC; // The basis vectors of the plane, i.e. in which the problem appears 2D.
	vec3_norm(E_AB, AB); // Basis vectors are normed.
	E_AB[3] = 0.0; // The fourth component should be zero, as the basis shouldn't 'point anywhere' in 4D.
	vec3_mul_cross(E_ABxBC, E_AB, BC);
	vec3_norm(E_ABxBC, E_ABxBC);
	E_ABxBC[3] = 0.0;
	if (vec3_len(E_ABxBC) < 1e-4) // Exit with an error code if AB, BC are parallel. 
	{
		printf("[NOTICE] %s\n", "AB, BC are parallel in call to three_point_arc(...).");
		memcpy(*O, B, sizeof(vec4));
		*R = 0.0;
		memcpy(*Z, UP, sizeof(vec4));
		return -1; 
	}
	vec3_mul_cross(E_ABxABxBC, E_AB, E_ABxBC);
	// vec3_norm(E_ABxABxBC, E_ABxABxBC); // No need, cross product of orthonormal vectors is normed.
	E_ABxABxBC[3] = 0.0; 
	
	// We now identify the transformation that takes us into and out of that plane.
	mat4x4 P, Q; // The basis matrix and transformation matrix (i.e. inverse of basis matrix).
	mat4x4_identity(P);
	for (unsigned int i = 0; i < 3; i++)
	{
		P[i][0] = E_AB[i];
		P[i][1] = E_ABxABxBC[i]; 
		P[i][2] = E_ABxBC[i]; // This is the component that will be perpendicular to the plane, i.e. 0 always.
	}
	mat4x4_invert(Q, P);
	vec4 aA, aB, aC, aAB, aBC; // Transform all of our points into the new coordinate system. 
	mat4x4_mul_vec4(aA, Q, A);
	mat4x4_mul_vec4(aB, Q, B);
	mat4x4_mul_vec4(aC, Q, C);
	mat4x4_mul_vec4(aAB, Q, AB);
	mat4x4_mul_vec4(aBC, Q, BC);
	
	// We're now in the plane where everything is 2D, so let's do some geometry to find the centre of the arc.
	vec4 half_aAB, bisect_aAB, half_aBC, bisect_aBC; // We bisect AB' and BC'.
	vec4_scale(half_aAB, aAB, 0.5);
	vec4_add(bisect_aAB, aA, half_aAB);
	vec4_scale(half_aBC, aBC, 0.5);
	vec4_add(bisect_aBC, aB, half_aBC);
	float m_aAB = aAB[1] / aAB[0];
	float m_aBC = aBC[1] / aBC[0];
	float m1 = -1.0 / m_aAB; // Gradient from bisect(AB') to centre of arc.
	float c1 = bisect_aAB[1] - m1 * bisect_aAB[0]; // The line y = m_1 x + c_1 goes through O, bisect(AB').
	float m2 = -1.0 / m_aBC; // Gradient from bisect(BC') to centre of arc.
	float c2 = bisect_aBC[1] - m2 * bisect_aBC[0]; // The line y = m_2 x + c_2 goes through O, bisect(BC').
	
	// We solve y = m_1 x + c_1, y = m_2 x + c_2 to get x, y:
	vec4 aO = \
	{ \
		- (c2 - c1) / (m2 - m1), \
		- m1 * (c2 - c1) / (m2 - m1) + c1, \
		0.0, \
		1.0 \
	}; // The fourth component is 1.0 because this is an absolute position, not just a direction.
	
	// Transform O' back to O using the basis matrix P.
	mat4x4_mul_vec4(*O, P, aO);
	vec4 AO;
	vec4_sub(AO, *O, A);
	*R = vec3_len(AO);
	mat4x4_mul_vec4(*Z, P, (float*)UP);
	if (vec3_len(*Z) < 0.9 or vec3_len(*Z) > 1.1)
		printf("[WARNING] Z vector not normed: %.2f, %.2f, %.2f.\n", *Z[0], *Z[1], *Z[2]);
	return 0;
}



/* Take two points, A, B, and the centre of a circle that intersects them, O, and bisect the arc between them.
 */
int bisect_arc(vec4 A, vec4 B, vec4 O, vec4* N)
{
	//printf("[DEBUG] bisect_arc(A = %.2f..., B = %.2f..., O = %.2f...);\n", A[0], B[0], O[0]);
	
	// Get the radius of the circle.
	vec4 AO, BO;
	vec4_sub(AO, O, A);
	float len_R = vec3_len(AO);
	// Check that AO = BO, and that AO x BO != 0 i.e. AO, BO not parallel.
	vec4_sub(BO, O, B);
	float len_R2 = vec3_len(AO);
	if (len_R2 - len_R > 1e-4)
		return -1;
	vec4 AOxBO;
	vec3_mul_cross(AOxBO, AO, BO);
	float len_AOxBO = vec3_len(AOxBO);
	if (len_AOxBO < 1e-4)
		return -2;
	
	// Bisect the line from A to B.
	vec4 AB, half_AB, bisect_AB;
	vec4_sub(AB, B, A);
	vec4_scale(half_AB, AB, 0.5);
	vec4_add(bisect_AB, A, half_AB);
	// Get the distance from the bisection to O.
	vec4 Y;
	vec4_sub(Y, O, bisect_AB);
	float len_Y = vec3_len(Y);
	// Get the difference, D = R - Y, and displace the bisection by that distance. 
	float len_D = len_R - len_Y;
	vec4 E_Y;
	vec3_norm(E_Y, Y); // Note that this points towards the centre of the circle.
	E_Y[3] = 0.0; // Shouldn't 'point anywhere' in 4D.
	vec4 D; // This will point from O to D.
	vec4_scale(D, E_Y, -len_D); 
	vec4_add(*N, bisect_AB, D);
	//printf("[DEBUG] len_Y = %.2f, len_R = %.2f, len_D = %.2f, N = %.2f...;\n", len_Y, len_R, len_D, *N[0]);
	return 0;
}



/* Bisect twice based on different radii and take the mean result.
 */
static inline void __average_bisect(vec4 P1, vec4 P2, vec4 O1, vec4 O2, vec4* B, vec4* O, float* R, vec4* Z)
{
	// Perform the arc bisection twice and output the average result.
	vec4 bisect1, bisect2, bisect_add;
	int e = bisect_arc(P1, P2, O1, &bisect1);
	int f = bisect_arc(P1, P2, O2, &bisect2);
	if (e < 0 or f < 0)
	{
		//printf("[NOTICE] %s %i, %i.\n", "Calls to bisect_arc failed with codes:", e, f);
		memcpy(bisect1, P1, sizeof(vec4));
		memcpy(bisect2, P2, sizeof(vec4));
	}
	vec4_add(bisect_add, bisect1, bisect2);
	vec4_scale(*B, bisect_add, 0.5);
	// Calculate the new arc centre, radius, and normal. 
	three_points_arc(P1, *B, P2, O, R, Z);
}



/* Take a set of n points with arc parameters and output 2n - 1 interpolated points.
 */
static inline int _interpolate_coords_by_arcs(vec4* coords, unsigned int count, \
                                              vec4* centres, float* radii, vec4* normals, \
                                              vec4** p_out, vec4** O_out, float** R_out, vec4** Z_out)
{
	// Create storage space for the new set of points, arc centres, etc.
	unsigned int count_new = 2 * count - 1;
	*p_out = (vec4*) malloc(count_new * sizeof(vec4));
	*O_out = (vec4*) malloc(count_new * sizeof(vec4));
	*R_out = (float*)malloc(count_new * sizeof(float));
	*Z_out = (vec4*) malloc(count_new * sizeof(vec4));
	if (*p_out == NULL or *O_out == NULL or *R_out == NULL or *Z_out == NULL)
	{
		printf("[WARNING] %s\n", "A call to malloc() returned NULL.");
		return -1;
	}
	vec4*  p = *p_out;
	vec4*  O = *O_out;
	float* R = *R_out;
	vec4*  Z = *Z_out;
	
	// Iterate over every point (bar the last). Take it and its next neighbour, and interpolate. 
	vec4 bisect;
	for (unsigned int i = 0, j = 0; i < count - 1; ++i, j += 2)
	{
		// Perform an arc bisection on the points i and i + 1.
		__average_bisect(coords[i], coords[i + 1], centres[i], centres[i + 1], \
		                 &p[j + 1], &O[j + 1],     &R[j + 1],  &Z[j + 1]);
		memcpy(p[j],  coords[i],  sizeof(vec4));
		memcpy(O[j],  centres[i], sizeof(vec4));
		memcpy(&R[j], &radii[i],  sizeof(float));
		memcpy(Z[j],  normals[i], sizeof(vec4));
	}
	// Append the last point, which we skipped (because last + 1 would be an overflow). 
	memcpy(p[count_new - 1],  coords[count - 1],  sizeof(vec4));
	memcpy(O[count_new - 1],  centres[count - 1], sizeof(vec4));
	memcpy(&R[count_new - 1], &radii[count - 1],  sizeof(float));
	memcpy(Z[count_new - 1],  normals[count - 1], sizeof(vec4));
	return count_new;
}



/* Map n points to 2n + 1 interpolated points.
 */
int interpolate_arc_curve(vec4* coords, unsigned int count, \
                          vec4** p_out, vec4** O_out, float** R_out, vec4** Z_out)
{
	static const vec4 UP = {0.0, 0.0, 1.0, 0.0}; // This is a default orientation for the ribbon on error.
	
	// Create arrays for the calculated centres, radii, and normals to the arc i.e. the ribbon. 
	vec4  centres[count];
	float radii[count];
	vec4  normals[count];
	// Iterate over all the points bar the first and the last, and find the arc parameters for those points
	// based on their previous and next neighbour.
	int e;
	for (unsigned int i = 1; i < count - 1; ++i)
	{
		e = three_points_arc(coords[i - 1], coords[i], coords[i + 1], &centres[i], &radii[i], &normals[i]);
		if (e < 0) // On error, assign sensible default parameters.
		{
			printf("[NOTICE] %s: %i.\n", "Call to three_points_arc(...) returned code", e);
			memcpy(centres[i], coords[i], sizeof(vec4));
			radii[i] = 0.0;
			memcpy(normals[i], UP, sizeof(vec4));
		}
	}
	// Fill in the information for the first and last point. 
	memcpy(centres[0],         coords[0],         sizeof(vec4));
	memcpy(centres[count - 1], coords[count - 1], sizeof(vec4));
	radii[0]         = 0.0;
	radii[count - 1] = 0.0;
	memcpy(normals[0],         UP, sizeof(vec4));
	memcpy(normals[count - 1], UP, sizeof(vec4));
	
	// Take the coordinates, and the arc parameters we just calculated, and interpolate all the points. 
	int f = _interpolate_coords_by_arcs(coords, count, centres, radii, normals, \
                                            p_out, O_out, R_out, Z_out);
	        // malloc p_out, O_out, R_out, Z_out
	// Parse the return value of _interpolate_coords_by_arcs(...).
	unsigned int count_new;
	if (f <= 0) // A return value of zero or less should never happen, but indicates error.
		return f - 1;
	else // A return value of one or more indicates the number of points returned.
		count_new = f + 2;
	// Map "A -|- B -|- C" to "A -|- B -|- C -|- " i.e. we need two extra half-widths at the end. 
	*p_out = (vec4*) realloc(*p_out, count_new * sizeof(vec4));
	*O_out = (vec4*) realloc(*O_out, count_new * sizeof(vec4));
	*R_out = (float*)realloc(*R_out, count_new * sizeof(float));
	*Z_out = (vec4*) realloc(*Z_out, count_new * sizeof(vec4));
	if (*p_out == NULL or *O_out == NULL or *R_out == NULL or *Z_out == NULL)
	{
		printf("[WARNING] %s\n", "A call to realloc() returned NULL.");
		return -3;
	}
	vec4*  p = *p_out;
	vec4*  O = *O_out;
	float* R = *R_out;
	vec4*  Z = *Z_out;
	// Calculate the position of the points in this last residue.
	vec4 DN;
	vec4_sub(DN, p[count_new - 3], p[count_new - 4]); // Goes from N-3 -> N-2.
	vec4_add(p[count_new - 2], p[count_new - 3], DN); // N-1 = N-2 + (N-3 -> N-2).
	vec4_add(p[count_new - 1], p[count_new - 2], DN); // N   = N-1 + (N-3 -> N-2).
	// Give those points radii, centres, etc.
	memcpy(O[count_new - 2], p[count_new - 2], sizeof(vec4));
	R[count_new - 2] = 0.0;
	memcpy(Z[count_new - 2], UP, sizeof(vec4));
	memcpy(O[count_new - 1], p[count_new - 1], sizeof(vec4));
	R[count_new - 1] = 0.0;
	memcpy(Z[count_new - 1], UP, sizeof(vec4));
	return count_new;
}



/* TODO.
 */
int curve_extract_residues(atom_t* alphas, unsigned int alphas_len, unsigned int** residues_out)
{
	*residues_out = (unsigned int*)malloc(alphas_len * sizeof(unsigned int));
	unsigned int* residues = *residues_out;
	
	for (unsigned int i = 0; i < alphas_len; ++i)
		residues[i] = alphas[i].res_id;
	return alphas_len;
}
