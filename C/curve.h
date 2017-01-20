#include "pdb.h"

#include "linmath/linmath.h"

#include <iso646.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/gl.h>



/* Describe a curve comprised of alpha carbon atoms which have had their positions interpolated
 * by finding circles that pass through successive triplets of points. 
 */
typedef struct curve
{
	atom_t*      alphas;
	unsigned int alphas_len;
	vec4*        alpha_coords;
	
	unsigned int* residues;
	unsigned int  residues_len;
	
	vec4*        points;
	unsigned int points_len;
	vec4*        arc_centres;
	float*       arc_radii;
	vec4*        z_normals;
} curve_t;



//
// The goal of this module is a _very_ simple smooth arc representation of the protein backbone.
//

extern int three_points_arc(vec4, vec4, vec4, \
                            vec4*, float*, vec4*); 

extern int bisect_arc(vec4, vec4, vec4, \
                      vec4*);

extern int interpolate_arc_curve(vec4*, unsigned int, \
                                 vec4**, vec4**, float**, vec4**);

extern int curve_extract_residues(atom_t*, unsigned int, unsigned int**);

