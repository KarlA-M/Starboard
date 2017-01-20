#include "commands.h"



/* If we have seen that a line is available, transform it into a command.
 */
command_t get_command(params_t* out)
{
	command_t cmd = COMMAND_NULL;

	// Populate a buffer with whatever is in the stream. 
	char*  buffer = NULL;
	size_t len    = 0;
	int    e      = getline(&buffer, &len, stdin);    // Calls malloc() on buffer.

	// Error handling on the getline() call.
	if (e < 1)
	{
		printf("[WARNING] %s: %i.\n", "Call to getline() failed with code", e);
		if (buffer)
			free(buffer);
		return cmd;
	}

	// Get the first word of the command by tokenisation.
	int newline = strlen(buffer);
	buffer[newline - 1] = '\0';
	out->argc = 0;
	out->argv = NULL;
	char* token = strtok(buffer, " ");
	while (token != NULL)
	{
		// HACK. Replace all '#' with ' ' as a way to allow users to type spaces.
		// Also replace all non-printable characters with ' '.
		for (unsigned int i = 0; token[i] != '\0'; ++i)
			token[i] = ((token[i] == '#' or token[i] < 32 or token[i] > 126) ? ' ' : token[i]);

		// Increment argc and add this string to the argv array.
		++out->argc;
		out->argv = (char**)realloc(out->argv, out->argc * sizeof(char*));
		if (not out->argv)
		{
			printf("[ERROR] %s\n", "Call to realloc() returned NULL.");
			return cmd;
		}
		out->argv[out->argc - 1] = strdup(token);    // Calls malloc().

		token = strtok(NULL, " ");
	}

	// Attempt to identify the command parsed.
	if (out->argc > 0)
	{
		if (strcasecmp(out->argv[0], "load") == 0)
			cmd = COMMAND_LOAD;
		else if (strcasecmp(out->argv[0], "status") == 0)
			cmd = COMMAND_STATUS;
	}

	free(buffer);    // Free the malloc()ed buffer.
	return cmd;
}



/* All params_t variables should be fed through this function when they are finished with.
 */
void destroy_params_t(params_t* to_destroy)
{
	for (int i = 0; i < to_destroy->argc; i++)
		free(to_destroy->argv[i]);
	free(to_destroy->argv);
}
