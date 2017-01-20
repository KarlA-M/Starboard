#include "input.h"
#include "commands.h"
#include "pdb.h"
#include "curve.h"
#include "ribbon.h"
#include "colorwheel.h"
#include "engine.h"
#include "shader.h"
#include "render.h"
#include "monoview.h"
#include "objects.h"

#include "linmath/linmath.h"

#include <iso646.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tgmath.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>


// Rendering.
//

const char*  TITLE   = "Starboard";
const GLuint WIDTH   = 1280;
const GLuint HEIGHT  = 720;
const float  VERSION = 3.3;
const GLuint SAMPLES = 8;
GLuint       RealWidth;
GLuint       RealHeight;

GLFWwindow* create_window(const char*, const GLuint, const GLuint, const float);


// User interaction via the terminal.
//

int do_load_command2(params_t*);
int do_status_command(params_t*);


// User interaction in 3D.
//

// Define a camera.
static const vec3 UP    = {0.0,  0.0, 1.0};
static const vec3 RIGHT = {0.0, -1.0, 0.0};
vec3 CameraPosition  = {70.0,   100.0,   15.0  }; // TODO. Rather than be hard-coded, these values should be
vec3 CameraDirection = { 0.455,   0.827,  0.331}; // calculated based on the input protein(s).
vec3 CameraUp;
vec3 CameraRight;

// Keyboard input, which controls the camera.
bool Keys[1024];

void callback_keyboard(GLFWwindow*, int, int, int, int);
void engage_keyboard();


/* Begin main program flow.
 */
int main(int argc, char** argv)
{
	GLFWwindow* window = create_window(TITLE, WIDTH, HEIGHT, VERSION);
	if (window == NULL)
	{
		printf("[FATAL] %s\n", "Failed to create window.");
		return -1;
	}
	
	printf("[NOTICE] %s\n", "Welcome to Starboard.");
	printf("[NOTICE] %s\n", "Enterting interactive mode:");
	printf("\n> ");
	fflush(stdout);
	
	
	// Create variables that monitor that current state of Starboard.
	//
	
	// Store & handle the user's text commands in the console.
	command_t cmd       = COMMAND_NULL;
	params_t  NO_PARAMS = {.argc = 0, .argv = NULL}; 
	params_t  args;
	memcpy(&args, &NO_PARAMS, sizeof(params_t));
	
	
	// Rendering & event handling.
	//
	
	int e;
	
	// Create the shading programs we need.
	GLuint main_shader;
	e = shader_program_create("GL/main.vert", "GL/main.frag", &main_shader); 
	if (e < 0)
	{
		printf("[FATAL] %s: %i.\n", "Call to shader_program_create() failed with code", main_shader);
		return -2;
	}
	engine_use_shader(main_shader);
	
	// Define the camera matrix, but it will be updated later.
	vec3_mul_cross(CameraUp, CameraDirection, RIGHT);
	vec3_mul_cross(CameraRight, CameraDirection, CameraUp);
	mat4x4  cameramat;
	vec3    camera_towards;
	GLfloat cameracmp[16];
	
	// Create the perspective projection transformation.
	mat4x4 perspectivemat; 
	mat4x4_perspective(perspectivemat, M_PI / 3.0, (float)WIDTH / (float)HEIGHT, 1.0, 150.0);
	GLfloat perspectivecmp[16];
	mat4x4_to_GLfloat16(perspectivemat, perspectivecmp);
	
	// Create the framebuffer that we will render to.
	engine_initialize();
	framebuffer_t framebuffer;
	e = engine_create_framebuffer(RealWidth, RealHeight, SAMPLES, &framebuffer);
	if (e != 0)
	{
		printf("[FATAL] %s: %i.\n", "Engine initialisation failed with code", e);
		return -3;
	}
	
	// Create the list of objects to render.
	initialize_objects();
	
	
	
	// Set callbacks for event handling in the 3D window.
	glfwSetKeyCallback(window, callback_keyboard);
	
	// Loop until the end of the program. 
	while (!glfwWindowShouldClose(window))
	{
		// Begin the current frame.
		engine_use_framebuffer(&framebuffer);
		static const vec4 canvas_color = {0.2, 0.3, 0.3, 0.0};
		engine_begin_frame(canvas_color);
		
		
		// Actual rendering code.
		//
		
		// We need to send the perspective and camera matrices to the shader. 
		GLint perspectiveloc = glGetUniformLocation(Shader, "projection");
		glUniformMatrix4fv(perspectiveloc, 1, GL_FALSE, perspectivecmp);
		GLint cameraloc = glGetUniformLocation(Shader, "view");
		glUniformMatrix4fv(cameraloc, 1, GL_FALSE, cameracmp);
		
		// Draw all of the renderable objects in the object list.
		draw_all_objects();
		
		
		// Command input.
		//
		
		// Get commands from the terminal (non-blocking).
		if (line_available())
			cmd = get_command(&args);
		
		switch (cmd)
		{
			// Parse the command to load a monomer structure as a ribbon.
			case COMMAND_LOAD: do_load_command2(&args);
			break;
			
			// Parse the command to print status information about which models are currently loaded.
			case COMMAND_STATUS: do_status_command(&args);
			break;
			
			// Handle unknown commands, empty commands, etc.
			default:
			case COMMAND_NULL:
				if (args.argc > 0)
					printf("[ERROR] %s: %s.\n", "Could not understand command", args.argv[0]);
			break;
		}
		
		// Allow GLFW to handle GUI-related events. 
		glfwPollEvents();
		engage_keyboard();
		
		// Update the camera.
		vec3_add(camera_towards, CameraPosition, (float*)CameraDirection);
		mat4x4_look_at(cameramat, CameraPosition, camera_towards, (float*)CameraUp);
		mat4x4_to_GLfloat16(cameramat, cameracmp);
		
		
		// Clean up this iteration.
		//
		
		// Free any memory malloc'd this cycle that is no longer needed.
		if (args.argc > 0)
		{
			destroy_params_t(&args);
			cmd = COMMAND_NULL;
			memcpy(&args, &NO_PARAMS, sizeof(params_t));
			printf("\n> ");
			fflush(stdout);
		}
		
		// End the current frame.
		engine_end_frame(1);
	}
	
	engine_destroy_framebuffer(&framebuffer);
	glfwTerminate();
	return 0;
}



/* Create an OpenGL window with GLFW.
*/
GLFWwindow* create_window(const char* title, const GLuint width, const GLuint height, const float version)
{
	// Get the desired OpenGL version.
	int version_major = floor(version);
	int version_minor = round((version - (float)version_major) * 10.0);
	//printf("[DEBUG] %s: %i.%i.\n", "OpenGL version", version_major, version_minor);
	
	// Describe to GLFW the kind of window we wish to create. 
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version_major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version_minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	#ifdef DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	#endif
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	
	// Create the window.
	// REMARK. NULL, NULL means 1. not fullscreen and 2. only use one window. 
	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL)
	{
		printf("[ERROR] %s\n", "Failed to initialise GLFW.");
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);
	
	// Use GLEW to ensure this program will use the correct OpenGL version.
	glewExperimental = GL_TRUE;
	int e = glewInit();
	glGetError(); // Discard errors from glewInit().
	//printf("[DEBUG] at glewInit(), glGetError() = %i.\n", glGetError());
	if (e != GLEW_OK)
	{
		printf("[ERROR] %s\n", "Failed to initialise GLEW:");
		printf("....... #%i: %s.\n", e, glewGetErrorString(e));
		return NULL;
	}
	
	// Inform OpenGL of the width and height of the context created.
	// REMARK. We use glfwGetFramebufferSize in case GLFW had to alter our request (i.e. Retina).
	glfwGetFramebufferSize(window, &RealWidth, &RealHeight);
	glViewport(0, 0, RealWidth, RealHeight);
	printf("%i, %i", RealWidth, RealHeight);
	
	return window;
}




/* TODO.
 */
int do_load_command2(params_t* args)
{
	//
	//
	
	if (args->argc != 2)
	{
		printf("[ERROR] %s\n", "Usage: load filename");
		return -2;
	}
	
	
	//
	//
	
	chain_t   chn_in;
	curve_t   cur_in;
	ribbon2_t rib_in;
	
	chain_t*   chn = &chn_in;
	curve_t*   cur = &cur_in;
	ribbon2_t* rib = &rib_in;
	
	int e;
	
	
	//
	//
	
	char filename[strlen(args->argv[1]) + 9]; // It's strlen + 9 because 'v' 'a' 'r' '/' '.' 'p' 'd' 'b' '\0'.
	filename[0] = '\0';
	strcat(filename, "var/");
	strcat(filename, args->argv[1]);
	strcat(filename, ".pdb");
	e = parse_pdb(chn, filename); // malloc chn->atoms
	if (e <= 0) // malloc chn->atoms
	{
		printf("[ERROR] %s: %s. Error code: %i.\n", "Could not open", filename, e);
		return -1;
	}
	printf("[NOTICE] %s: %i.\n", "Total atom count", chn->atoms_len);
	
	//
	//
	
	cur->alphas = (atom_t*)malloc(chn->atoms_len * sizeof(atom_t)); // malloc cur->alphas
	cur->alphas_len = filter_atoms(cur->alphas, chn->atoms, chn->atoms_len, 0, 0, "", "CA");
	atoms_to_vec4s(&cur->alpha_coords, cur->alphas, cur->alphas_len); // malloc cur->alpha_coords
	cur->residues_len = curve_extract_residues(cur->alphas, cur->alphas_len, &cur->residues); // malloc cur->residues
	cur->points_len = interpolate_arc_curve(cur->alpha_coords, cur->alphas_len, \
	                                        &cur->points, &cur->arc_centres, &cur->arc_radii, &cur->z_normals);
	                                        // malloc cur->points, cur->arc_centres, cur->arc_radii, cur->z_normals
	
	
	//
	//
	
	rib->num_vertices = curve_to_ribbon(cur->points, cur->points_len, cur->z_normals, NULL, \
	                                    &rib->vertex_components, &rib->num_vertex_components, \
	                                    &rib->element_components, &rib->num_element_components);
	                                    // malloc rib->vertex_components, rib->element_components
	
	vec4 default_color[1];
	generate_pastel_colors(default_color, 1, 1.00);
	repeat_color(default_color[0], cur->residues_len, &rib->residue_colors); // malloc rib->residue_colors
	residue_colors_to_vertex_colors(rib->residue_colors, cur->residues_len, \
	                                &rib->vertex_color_components, &rib->num_vertex_color_components);
	                                // malloc rib->vertex_color_components
	ribbon_to_outline(rib->num_vertices, &rib->outline_element_components, &rib->num_outline_element_components);
	                  // malloc rib->outline_element_components
	
	static const vec4 OUTLINE_COLOR = {1.0, 1.0, 1.0, 1.0};
	repeat_color(OUTLINE_COLOR, cur->residues_len, &rib->outline_colors);
	residue_colors_to_vertex_colors(rib->outline_colors, cur->residues_len, \
	                                &rib->outline_color_components, &rib->num_outline_color_components);
	                                // malloc rib->outline_color_components
	
	
	//
	//
	
	monoview_t* monoview = (monoview_t*)malloc(sizeof(monoview_t));
	monoview->name = malloc((strlen(args->argv[1]) + 1) * sizeof(char));
	memcpy(monoview->name, args->argv[1], (strlen(args->argv[1]) + 1) * sizeof(char));
	memcpy(&monoview->chain,  chn, sizeof(chain_t));
	memcpy(&monoview->curve,  cur, sizeof(curve_t));
	memcpy(&monoview->ribbon, rib, sizeof(ribbon2_t));
	drawable_t* drawable = (drawable_t*)malloc(sizeof(drawable_t));
	
	allocate_drawable_buffers(2, drawable); 
	monoview_to_drawable(monoview, drawable);
	add_object((void*)monoview, MONOVIEW, drawable);
	
	return 0;
}



/* TODO.
 */
int do_structure_command(params_t* args)
{
	if (args->argc != 2)
	{
		printf("[ERROR] %s\n", "Usage: uniprot #i");
		return -1;
	}
	
	
	// Convert the string args->argv[1] in the form #i to a model number i of type unsigned int.
	//
	
	char hash = args->argv[1][0];
	if (hash != '#')
	{
		printf("[ERROR] %s: %c.\n", "Parameter not of model number form #i", hash);
		return -2;
	}
	
	unsigned int len = strlen(args->argv[1]); // strlen + 1 for '\0', - 1 because no '#'.
	if (len < 2)
	{
		printf("[ERROR] %s\n", "No model number i specified.");
		return -3;
	}
	char num[len];
	memcpy(num, &args->argv[1][1], len);
	char* e;
	unsigned int model = (unsigned int)strtoul(num, &e, 10);
	if (e == num and model == 0)
	{
		printf("[ERROR] %s: %s.\n", "Could not convert to integer", num);
		return -4;
	}
	
	
	// Check that the specified model is present and a monomer structure.
	//
	
	if (model >= RenderObjsLen or RenderObjClasses[model] != MONOMER)
	{
		printf("[ERROR] %s\n", "Please choose a model listed as MONOMER under `status`.");
		return -5;
	}
	
	
	// Check that the .kbstr file is present, or create it.
	//
	
	char* uniprot = *(char**)RenderObjs[model];
	
	char filename[strlen(uniprot) + 11]; // + 11 because 'v' 'a' 'r' '/' '.' 'k' 'b' 's' 't' 'r' '\0'.
	filename[0] = '\0';
	strcat(filename, "var/");
	strcat(filename, uniprot);
	strcat(filename, ".kbstr");
	
	FILE* f = fopen(filename, "r");
	if (f == NULL)
	{
		char* command_template = "./Py/uniprot.py structure %s > var/%s.kbstr";
		char  command[strlen(command_template) + 2 * strlen(uniprot) - 3]; // - 4 for 2x %s, + 1 for '\0'.
		sprintf(command, command_template, uniprot, uniprot);
		system(command);
		
		f = fopen(filename, "r");
		if (f == NULL)
		{
			printf("[ERROR] %s: %s.\n", "Could not use uniprot.py to create file", filename);
			return -6;
		}
	}
	
}



/* TODO.
 */
int do_status_command(params_t* args)
{
	if (args->argc != 1)
	{
		printf("[ERROR] %s\n", "Usage: status");
		return -1;
	}
	
	printf("[STATUS] %s: %i.\n", "Number of models loaded", RenderObjsLen);
	char buffer[1024];
	for (unsigned int i = 0; i < RenderObjsLen; ++i)
	{
		objclass2string(RenderObjClasses[i], buffer);
		printf("........ %i %s %s\n", i, buffer, *(char**)RenderObjs[i]);
	}
}



/* TODO.
 */
static inline void _rotate_camera(vec3 axis, float angle)
{
	static quat rot;
	quat_rotate(rot, angle, axis);
	quat_mul_vec3(CameraDirection, rot, CameraDirection);
	quat_mul_vec3(CameraUp,        rot, CameraUp);
	quat_mul_vec3(CameraRight,     rot, CameraRight);
}



/* TODO.
 */
static inline void _translate_camera(vec3 axis, float distance)
{
	static vec3 del;
	vec3_scale(del, axis, distance);
	vec3_add(CameraPosition, del, CameraPosition);
}



/* TODO.
 */
void callback_keyboard(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (action == GLFW_PRESS)
		Keys[key] = true;
	else if (action == GLFW_RELEASE)
		Keys[key] = false;
}
void engage_keyboard()
{
	// TODO. Rather than moving by fixed angles/speeds, we should use a time delta.
	
	// Look around.
	//
	
	static const float angle = M_PI / 60.0;
	if (Keys[GLFW_KEY_I]) _rotate_camera(CameraRight,      angle);
	if (Keys[GLFW_KEY_K]) _rotate_camera(CameraRight,     -angle);
	if (Keys[GLFW_KEY_J]) _rotate_camera(CameraUp,         angle);
	if (Keys[GLFW_KEY_L]) _rotate_camera(CameraUp,        -angle);
	if (Keys[GLFW_KEY_U]) _rotate_camera(CameraDirection,  angle);
	if (Keys[GLFW_KEY_O]) _rotate_camera(CameraDirection, -angle);
	
	
	// Move.
	// 
	
	static const float speed = 1.2;
	if (Keys[GLFW_KEY_W]) _translate_camera(CameraDirection,  speed);
	if (Keys[GLFW_KEY_S]) _translate_camera(CameraDirection, -speed);
	if (Keys[GLFW_KEY_A]) _translate_camera(CameraRight,     -speed);
	if (Keys[GLFW_KEY_D]) _translate_camera(CameraRight,      speed);	
	if (Keys[GLFW_KEY_Q]) _translate_camera(CameraUp,         speed);
	if (Keys[GLFW_KEY_E]) _translate_camera(CameraUp,        -speed);	
}
