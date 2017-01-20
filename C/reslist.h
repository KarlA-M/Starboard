


#define STARBOARD_RESLIST_MAX_RESIDUES_PER_FILE 9999


static inline void _scrape_file(FILE* f, char* type, unsigned int* out)
{
	unsigned int line_len   = 1024;
	char         line[line_len];
	while (fgets(line, line_len, fp) != NULL)
	{
		char* token = strtok(line, " ");
		while (token != NULL)
		{
			token = strtok(NULL, " ");
		}
	}
}



/* Take a file f in "type res_start res_finish\n" format, and a list of types t of length n, and then parse f
 * such that the array A has A[0] = [x, y, z, ...], A[1] = [u, v, w, ...], ... where A[0], A[1], ... correspond
 * to the first, second, ... types and x, y, z, ... correspond to the residues listed as being of the first type
 * and u, v, w, ... are the residues of the second type and so forth. 
 */
int file2reslist(FILE* f, char** types, unsigned int num_types, unsigned int*** arrays_out, unsigned int** lens_out)
{
	/* Expect parameters in the form:
	 *     FILE* f = fopen(...);
	 *     char* types[...] = {'EXAMPLE', ...};
	 *     unsigned int num_types = ...;
	 *     unsigned int* A[num_types];
	 *     unsigned int L[num_types];
	 *     file2reslist(f, types, num_types, &A, &L);
	 */
	
	unsigned int temp[STARBOARD_MAX_RESIDUES_PER_FILE];
	for (unsigned int i = 0; i < num_types; ++i)
	{
		_scrape_file()
	}
	
	
	*arrays_out      = (unsigned int**)malloc(num_types * sizeof(unsigned int*)); // malloc *arrays_out
	unsigned int** A = *arrays_out;
	*lens_out        = (unsigned int* )malloc(num_types * sizeof(unsigned int )); // malloc *lens_out
	unsigned int*  L = *lens_out;
	for (unsigned int i = 0; i < num_types; ++i)
		A[i] = (unsigned int*)malloc(STARBOARD_RESLIST_MAX_RESIDUES_PER_FILE * sizeof(unsigned int));
	             // malloc A[i]
		
	
}
