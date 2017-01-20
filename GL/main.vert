#version 330 

layout(location = 0) in vec3 position; // This vertex shader assumes input is vec3 and adds own w-coordinate.
layout(location = 1) in vec4 vertex_color; 

uniform mat4 model; 
uniform mat4 view; 
uniform mat4 projection; 

out vec4 fragment_color; 

void main(void) 
{ 
	gl_Position = projection * view * model * vec4(position, 1.0); 
	fragment_color = vertex_color; 
} 
