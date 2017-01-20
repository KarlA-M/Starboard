#include "engine.h"



/* Use the framebuffer in the framebuffer_t object specified to render into.
 */
void engine_use_framebuffer(const framebuffer_t* f)
{
	// global Framebuffer
	Framebuffer = (framebuffer_t*)f;
}



/* Use the shader with the specified shader ID to render with.
 */
void engine_use_shader(const GLuint s)
{
	// global Shader
	Shader = s;
	glUseProgram(Shader);
}



/* Perform first-time use set-up for the rendering engine: set OpenGL options, etc.
 */
static GLFWmonitor** Monitors;           
static GLFWvidmode*  VideoMode;          
static double        RefreshesPerSecond; 
static double        SecondsPerRefresh;  
void engine_initialize(void)
{
	glEnable(GL_BLEND); // Allow alpha transparency in our geometry.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST); // Occlude fragments based on a depth test.
	
	// Get some information about the monitor which is helpful later if we have to manually v-sync.
	static unsigned int count;
	Monitors           = (GLFWmonitor**)glfwGetMonitors(&count);
	VideoMode          = (GLFWvidmode*)glfwGetVideoMode((GLFWmonitor*)Monitors[0]);
	RefreshesPerSecond = (double)VideoMode->refreshRate;
	SecondsPerRefresh  = 1.0 / RefreshesPerSecond;
}



/* Create a framebuffer with the specified properties.
 * Returns 0 on success, otherwise error.
 */
int engine_create_framebuffer(const unsigned int canvas_width, const unsigned int canvas_height, \
                              const unsigned int multisamples, \
                              framebuffer_t* out)
{
	// Create the objets we need for the framebuffer.
	out->multisamples  = multisamples;
	out->canvas_width  = canvas_width;
	out->canvas_height = canvas_height;
	if (out->multisamples > 1)
		glEnable(GL_MULTISAMPLE); // We wish to use a multisampled framebuffer, so we enable that option.
	glGenFramebuffers(1, &out->fbo);
	glGenTextures(1, &out->canvas);
	glGenRenderbuffers(1, &out->rbo);
	
	// Set up the framebuffer to work with it.
	glBindFramebuffer(GL_FRAMEBUFFER, out->fbo);
	
	// Create a texture-based canvas to draw on.
	if (out->multisamples > 1) // It needs to be multisampling-aware if requested.
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, out->canvas); 
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, multisamples, GL_RGBA, \
		                        out->canvas_width, out->canvas_height, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	}
	else // Otherwise, a normal texture is fine.
	{
		glBindTexture(GL_TEXTURE_2D, out->canvas); 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, out->canvas_width, out->canvas_height, 0, GL_RGBA, \
		             GL_UNSIGNED_BYTE, (GLvoid*)NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	// Similarly, we create a rendering pipeline for this frame buffer that is multisampling aware.
	glBindRenderbuffer(GL_RENDERBUFFER, out->rbo); 
	if (out->multisamples > 1)
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, out->multisamples, GL_DEPTH24_STENCIL8, \
		                                 out->canvas_width, out->canvas_height);
	else
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, \
		                      out->canvas_width, out->canvas_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	
	// Add the canvas and rendering pipeline buffers to the framebuffer.
	if (out->multisamples > 1)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, \
		                       GL_TEXTURE_2D_MULTISAMPLE, out->canvas, 0);
	else
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, \
		                       GL_TEXTURE_2D, out->canvas, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, out->rbo);
	
	// We check that the framebuffer was created correctly.
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("[ERROR] %s\n", "Could not complete framebuffer.");
		return -1;
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return 0;
}



/* Destroy a framebuffer object by deleting its OpenGL objects.
 */
void engine_destroy_framebuffer(framebuffer_t* in)
{
	glDeleteRenderbuffers(1, &in->rbo   );
	glDeleteTextures(     1, &in->canvas);
	glDeleteFramebuffers( 1, &in->fbo   );  
}



/* Begin the rendering of a frame by binding the framebuffer and clearing it.
 */
static double TimeFrameStarted; 
void engine_begin_frame(const vec4 canvas_color)
{
	TimeFrameStarted = glfwGetTime();
	
	// Tell OpenGL to render to our framebuffer rather than the screen.
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer->fbo);
	
	// Clear the canvas before drawing geometry.
	glClearColor(canvas_color[0], canvas_color[1], canvas_color[2], canvas_color[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}



/* Perform a benchmarking test.
 */
static unsigned int NumFramesRendered     = 0;
static double       TimeBenchmarkStarted  = 0;
static double       TimeBenchmarkFinished = 0;
static inline void _engine_benchmark(void)
{
	if (TimeBenchmarkStarted == 0)
		TimeBenchmarkStarted = glfwGetTime();
	++NumFramesRendered;
	if (NumFramesRendered == 1000)
	{
		TimeBenchmarkFinished = glfwGetTime();
		printf("\n[BENCHMARK] 1000 frames rendered in %.3f seconds, i.e. %.3f frames per second.\n", \
		       TimeBenchmarkFinished - TimeBenchmarkStarted, \
		       1000.0 / (TimeBenchmarkFinished - TimeBenchmarkStarted));
	}
}



/* End the frame by blitting the framebuffer to the screen and waiting for vsync. 
 */
static double TimeFrameFinished;
void engine_end_frame(const unsigned int render_every_x_refreshes)
{
	// Display the frame that we have rendered.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, Framebuffer->fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, Framebuffer->canvas_width, Framebuffer->canvas_height, \
	                  0, 0, Framebuffer->canvas_width, Framebuffer->canvas_height, \
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glFlush();
	
	//TimeFrameFinished = glfwGetTime();
	//double interval = TimeFrameFinished - TimeFrameStarted;
	//printf("[DEBUG] %s: %.9f.\n", "Time to render", interval);
	
	// V-sync.
	#ifdef VSYNC
	if (GLX_SGI_video_sync)
	{
		// Wait for v-blank, i.e. this call implements v-sync.
		GLuint count;
		glXWaitVideoSyncSGI(render_every_x_refreshes, 0, &count);
	}
	else
	{
		// Ensure that the frame timings work out to the desired frames per second. 
		while (glfwGetTime() - TimeFrameStarted < SecondsPerRefresh * render_every_x_refreshes); // Busy-waiting.
	}
	#endif
	
	#ifdef DEBUG
	_engine_benchmark();
	#endif
}

