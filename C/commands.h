#ifndef STARBOARD_COMMANDS
#define STARBOARD_COMMANDS

#include <iso646.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO.
 */
typedef enum command
{
	COMMAND_NULL,
	COMMAND_LOAD,
	COMMAND_STATUS
} command_t;



/* TODO.
 */
typedef struct params 
{
	unsigned int argc;
	char**       argv;
} params_t;



extern command_t get_command(params_t*);

extern void destroy_params_t(params_t*);

#endif
