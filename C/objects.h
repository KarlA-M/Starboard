#include "engine.h"
#include "render.h"


// This defines a hard limit on the maximum renderable objects displayed at once in Starboard.
#define STARBOARD_OBJS_MAX 1024



/* TODO.
 */
typedef enum objclass
{ \
	MONOVIEW \
} objclass_t;
void objclass2string(objclass_t c, char* s)
{
	switch (c)
	{
		case MONOVIEW: memcpy(s, "MONOMER", 8 * sizeof(char)); break;
		default:       memcpy(s, "UNKNOWN", 8 * sizeof(char)); break;
	}
}



//
// TODO.
//

void*        RenderObjs[STARBOARD_OBJS_MAX];
objclass_t   RenderObjClasses[STARBOARD_OBJS_MAX];
drawable_t*  RenderObjDrawables[STARBOARD_OBJS_MAX];
unsigned int RenderObjsLen;



/* TODO.
 */
void initialize_objects()
{
	static bool RenderObjsInitialised = false;
	if (not RenderObjsInitialised)
	{
		RenderObjsInitialised = true;
		RenderObjsLen = 0;
	}
}




/* TODO.
 */
int add_object(void* object, objclass_t object_class, drawable_t* object_drawable)
{
	// Test for segfaults on RenderObjs before they happen.
	if (RenderObjsLen == STARBOARD_OBJS_MAX)
		return -1;
	
	// Add the object to the RenderObjs array.
	RenderObjs[RenderObjsLen] = object;
	RenderObjClasses[RenderObjsLen] = object_class;
	RenderObjDrawables[RenderObjsLen] = object_drawable;
	++RenderObjsLen;
	return RenderObjsLen - 1;
}



/* TODO.
 */
int del_object(unsigned int index)
{
	// We can't delete an object if it isn't within the bounds of the list.
	if (index >= RenderObjsLen)
		return -1;
	
	// Delete the object and shift the array down one index.
	for (unsigned int i = index + 1; i < RenderObjsLen; ++i)
	{
		RenderObjs[i - 1]         = RenderObjs[i];
		RenderObjClasses[i - 1]   = RenderObjClasses[i];
		RenderObjDrawables[i - 1] = RenderObjDrawables[i];
	}
	--RenderObjsLen;
	return 0;
}



/* TODO.
 */
int draw_object(unsigned int index)
{
	if (index >= RenderObjsLen)
		return -1;
	
	drawable_t* obj_draw  = RenderObjDrawables[index];
	objclass_t  obj_class = RenderObjClasses[index]; 
	
	// Once per drawable, we need to load the model matrix into the shader.
	GLint model_variable = glGetUniformLocation(Shader, "model");
	glUniformMatrix4fv(model_variable, 1, GL_FALSE, obj_draw->model_matrix_components);
	
	// Set certain OpenGL flags that depend on object type.
	switch (obj_class)
	{
		case MONOVIEW:
			glDisable(GL_CULL_FACE);
			glLineWidth(1.5);
		break;
	}
	
	// Iterate over the drawable's buffers.
	for (unsigned int i = 0; i < obj_draw->n; ++i)
	{
		glBindVertexArray(obj_draw->vao[i]);
		glDrawElements(obj_draw->element_class[i], obj_draw->ebo_len[i], GL_UNSIGNED_INT, (GLvoid*)0);
		glBindVertexArray(0);
	}
}



/* TODO.
 */
void draw_all_objects(void)
{
	for (unsigned int i = 0; i < RenderObjsLen; ++i)
		draw_object(i);
}
