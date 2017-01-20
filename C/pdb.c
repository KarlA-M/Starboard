#include "pdb.h"



/* Take a line and extract a string field at character start_index, length len.
 */
static inline int __get_field_s(char* out, const char* line, const unsigned int start_index, const size_t len)
{
	// Get the substring from the line.
	char substr[len + 1];
	strncpy(substr, line + start_index, len);
	substr[len] = '\0';

	// Strip leading and trailing whitespace.
	int real_start = 0, real_end = 0;
	for (unsigned int i = 0, c = (unsigned int)substr[0]; c != '\0'; c = (unsigned int)substr[++i])
	{
		if (c != ' ' and c != '\n')    // real_end should be the last non-whitespace non-null character.  
			real_end = i;
		else if (real_start == i)    // If whitespace, and still at start, then increment real_start.
			++real_start;
	}
	if (real_end < real_start)    // Can happen if string all whitespace.
		real_end = real_start;
	const size_t real_len = 1 + real_end - real_start;
	char real[real_len + 1];
	strncpy(real, substr + real_start, real_len);
	real[real_len] = '\0';

	// Copy the string into the output variable.
	strcpy(out, real);
	return 0;
}



/* Take a line and extract an unsigned integer field at character start_index, length len.
 */
static inline int __get_field_ui(unsigned int* out, const char* line, const unsigned int start_index, const size_t len)
{
	// Get the substring from the line.
	char substr[len + 1];
	strncpy(substr, line + start_index, len);
	substr[len] = '\0';
	
	// Convert to an integer.
	errno = 0;
	long s = strtol(substr, NULL, 0);
	if (errno)    // Return an error code if conversion failed.
		return -1;
	if (s < 0 or s > INT_MAX)    // Return an error code if the number given is out of range. 
		return -2;

	// Copy the int into the output variable. 
	*out = (unsigned int)s;
	return 0;
}



/* Take a line and extract a float field at character start_index, length len.
 */
static inline int __get_field_f(float* out, const char* line, const unsigned int start_index, const size_t len)
{
	// Get the substring from the line.
	char substr[len + 1];
	strncpy(substr, line + start_index, len);
	substr[len] = '\0';
	
	// Convert to an integer.
	errno = 0;
	float s = strtof(substr, NULL);
	if (errno)    // Return an error code if conversion failed.
		return -1;

	// Copy the int into the output variable. 
	*out = s;
	return 0;
}



/* Generic interface for __get_field_<type>. 
 */
#define __get_field(o, l, s, n) \
	_Generic((o), \
		char*: __get_field_s, \
		unsigned int*: __get_field_ui, \
		float*: __get_field_f, \
		default: NULL \
	)(o, l, s, n)



/* Populate an atom structure with the relevant fields from a PDB ATOM record line.
 * Returns: +1 not ATOM record; 0 success; less than 0 error.
 */
static inline int _parse_atom_record_line(atom_t* out, const char* line)
{
	// Check that this actually is an ATOM record.
	char record[7];
	if (__get_field((char*)record, line, 0, 6))
		return -1;
	if (strcmp(record, "ATOM"))
	{
		printf("[DEBUG] %s: %s.\n", "Ignoring record", (char*)record);
		return 1;    // "Not an ATOM record."
	}

	// Attempt to populate the given atom structure.
	if (__get_field(&out->id, line, 0, 6))
		return -2;
	if (__get_field(&out->res_id, line, 22, 4))
		return -3;
	if (__get_field((char*)out->res_type, line, 17, 3))
		return -4;
	if (__get_field((char*)out->type, line, 12, 4))
		return -5;
	if (__get_field(&out->x, line, 30, 8))
		return -6;
	if (__get_field(&out->y, line, 38, 8))
		return -7;
	if (__get_field(&out->z, line, 46, 8))
		return -8;
	return 0;
}



/* Parse a PDB file into an array of atom structures. 
 */
int parse_pdb(chain_t* chain, const char* filename)
{
	FILE* fp = fopen(filename, "r");
	if (fp == NULL)
		return -1;
	
	// Create the chain structure.
	//*chain_out = (chain_t*)malloc(sizeof(chain_t));
	//chain_t* chain = *chain_out;
	
	// Create an array of atom structures.
	unsigned int atoms_len  = 1024;
	unsigned int atom_count = 0;
	chain->atoms = (atom_t*)malloc(atoms_len * sizeof(atom_t));
	atom_t* atoms    = chain->atoms;    // REMARK. For readability (& historical reasons).
	
	// Iterate over each line adding ATOM records to the atoms array.
	unsigned int line_len   = 1024;
	char         line[line_len];
	int          e;
	atom_t       out;
	while (fgets(line, line_len, fp) != NULL)
	{
		// Attempt to parse line as an atom record, skipping on error. 
		e = _parse_atom_record_line(&out, line);
		if (e == 1)
			continue;
		else if (e < 0)
		{
			printf("[WARNING] %s: %i.\n", "Function _get_field failed with code", e);
			continue;
		}
		
		// If we get this far, we have a good atom record, so add it to the array.  
		atoms[atom_count] = out;
		++atom_count;
		
		// If the next added atom would buffer overflow, double the memory available. 	
		if (atom_count == atoms_len)
		{
			atoms_len *= 2;
			//printf("[DEBUG] %s: %i.\n", "Increasing size of atoms array to", atoms_len);
			chain->atoms = (atom_t*)realloc(atoms, atoms_len * sizeof(atom_t));
			atoms = chain->atoms;
			
			if (atoms == NULL)
				return -2;
		}
	}
	
	fclose(fp);
	chain->atoms_len = atom_count;
	return atom_count;
}



/*
 */
static inline bool __startswith(const char* a, const char* b)
{
	unsigned int len = strlen(b);
	return strncmp(a, b, len);
}



/*
 */
static inline bool _atom_match(const atom_t a, const unsigned int id, const unsigned int res_id, \
                               const char* res_type, const char* type)
{
	// Return false if any conditions fail; return true if they all pass.
	if (id > 0 and a.id != id)    // Make sure the atom ID matches.
		return false;
	if (res_id > 0 and a.res_id != res_id)    // Residue ID.
		return false;
	if (strlen(res_type) > 0 and __startswith(a.res_type, res_type))    // Residue type.
		return false;
	if (strlen(type) > 0 and __startswith(a.type, type))    // Atom type.
		return false;
	return true;
}



/*
 */
int filter_atoms(atom_t* out, const atom_t* in, const unsigned int len, const unsigned int id, \
                 const unsigned int res_id, const char* res_type, const char* type)
{
	int i = 0;
	for (int j = 0; j < len; j++)
		if (_atom_match(in[j], id, res_id, res_type, type))
			out[i++] = in[j];
	return i;
}



/*
 */
int atoms_to_vec4s(vec4** out, const atom_t* in, const unsigned int len)
{
	// Allocate memory for the out array.
	*out = (vec4*)malloc(len * sizeof(vec4));
	if (*out == NULL)
	{
		printf("[ERROR] %s\n", "Call to malloc() returned NULL.");
		return -1;
	}
	vec4* coords = *out;
	// Copy the coordinates x, y, z to the new array of vec4s.
	for (int i = 0; i < len; i++)
	{
		coords[i][0] = in[i].x;
		coords[i][1] = in[i].y;
		coords[i][2] = in[i].z;
		coords[i][3] = 0.0;
	}
	return 0;
}
