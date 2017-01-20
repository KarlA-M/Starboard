#include <GL/glew.h>
#include <GL/gl.h>



/* TODO.
 */
static inline int _get_all(char* filename, char** out)
{
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
	{
		printf("[ERROR] %s: %s.\n", "Could not open file", filename);
		return -1;
	}
	
	// Query the file object for the file's length in characters.
	fseek(f, 0, SEEK_END);
	unsigned int length = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	// Create a memory location large enough to hold the file.
	*out         = (char*)malloc((length + 1) * sizeof(char));
	char* buffer = *out;
	if (buffer == NULL)
	{
		printf("[ERROR] %s\n", "Could not allocate memory for file contents in _get_all().");
		return -2;
	}
	
	// Read the file to the buffer.
	if (fread(buffer, length * sizeof(char), 1, f) == 0)
	{
		printf("[ERROR] %s\n", "Call to fread() returned 0, i.e. nothing read.");
		return -3;
	}
	buffer[length] = '\0';
	fclose(f);
	
	return length;
}



/* TODO.
 */
int shader_program_create(char* vertex_shader_filename, char* fragment_shader_filename, GLuint* shader_out)
{
	GLint  e;
	GLchar s[1024];
	
	
	// Vertex shader.
	// 
	
	// Load the vertex shader code.
	char* vertex_shader_code;
	e = (GLint)_get_all(vertex_shader_filename, &vertex_shader_code); // malloc vertex_shader_code
	if (e < 0)
	{
		printf("[ERROR] %s: %i.\n", "Call to _get_all(...) for vertex shader failed with code", e);
		return -1;
	}
	
	
	// Compile the vertex shader.
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, (const GLchar**)&vertex_shader_code, (GLvoid*)NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &e);
	if (e == GL_FALSE)
	{
		glGetShaderInfoLog(vertex_shader, 1024, (GLvoid*)NULL, s);
		printf("[ERROR] %s: %i:\n", "Failed to compile vertex shader with code", e);
		printf("....... %s\n", s);
		return -2;
	}
	
	
	// Fragment shader.
	//
	
	// Load the fragment shader code.
	char* fragment_shader_code;
	e = (GLint)_get_all(fragment_shader_filename, &fragment_shader_code); // malloc fragment_shader_code
	if (e < 0)
	{
		printf("[ERROR] %s: %i.\n", "Call to _get_all(...) for fragment shader failed with code", e);
		return -3;
	}
	
	// Compile the fragment shader.
	GLint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, (const GLchar**)&fragment_shader_code, (GLvoid*)NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &e);
	if (e == GL_FALSE)
	{
		glGetShaderInfoLog(fragment_shader, 1024, (GLvoid*)NULL, s);
		printf("[ERROR] %s: %i:\n", "Failed to compile fragment shader with code", e);
		printf("....... %s\n", s);
		glDeleteShader(vertex_shader);
		return -4;
	}
	
	
	// Linking.
	//
	
	// Attach the vertex and fragment shaders to a shader program.
	*shader_out   = glCreateProgram();
	GLuint shader = *shader_out;
	glAttachShader(shader, vertex_shader);
	glAttachShader(shader, fragment_shader);
	
	// Link the shaders into a shader program.
	glLinkProgram(shader);
	glGetProgramiv(shader, GL_LINK_STATUS, &e);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);  
	if (e == GL_FALSE)
	{
		glGetProgramInfoLog(shader, 1024, (GLvoid*)NULL, s);
		printf("[ERROR] %s: %i:\n", "Failed to link shader program with code", e);
		printf("....... %s\n", s);
		return -5;
	}
	return shader;
	
	// Clean up.
	free(vertex_shader_code); // free vertex_shader_code, fragment_shader_code
	free(fragment_shader_code);
}
