#ifndef STARBOARD_PDB
#define STARBOARD_PDB

#include <iso646.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "linmath/linmath.h"



/* Describe an atom within a protein, which has a residue, an atom type, and a position. 
 */
typedef struct atom
{
	unsigned int id;
	unsigned int res_id;
	char         res_type[4], type[5];
	float        x, y, z;
} atom_t;



/* Describe a collection of atoms, which form a chain.
 */
typedef struct chain
{
	atom_t*      atoms;
	unsigned int atoms_len;
} chain_t;



extern int parse_pdb(chain_t*, const char*);

extern int filter_atoms(atom_t*, const atom_t*, const unsigned int, const unsigned int, const unsigned int, \
                        const char*, const char*);

extern int atoms_to_vec4s(vec4**, const atom_t*, const unsigned int);

#endif
