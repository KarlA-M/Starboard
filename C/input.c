#include "input.h"

// Parameters for select(). _TO_READ is initialised automatically in _initialise_nonblocking_stdin(). 
static fd_set               _TO_READ;
static const struct timeval _IMMEDIATE = {0, 0};



/* Performs first-use initialisation of _TO_READ.
 */
static bool _INITIALISED = false;
static inline void _initialise_nonblocking_stdin(void)
{
	if (not _INITIALISED)
	{
		FD_ZERO(&_TO_READ);
		FD_SET(STDIN_FILENO, &_TO_READ);
		_INITIALISED = true;
	}
}



/* Returns true if there is a line available to read from stdin, else returns false. 
 */
bool line_available(void)
{
	_initialise_nonblocking_stdin();

	// We copy the parameters before running select() because it overrides them.
	fd_set         to_read = _TO_READ;
	struct timeval timeout = _IMMEDIATE;
	return (select(1, &to_read, NULL, NULL, &timeout) > 0);
}

