#version 330 

in vec4 fragment_color;

out vec4 pixel_color; 

/* Take a depth buffer value and normalise it to [0, 1] based on knowledge of the near and far plane. 
 */
float linearize_depth(float depth, float near, float far) 
{ 
	float z      = depth * 2.0 - 1.0;                                    // Take us back to normalised
	                                                                     // device coordinates.
	float linear = (2.0 * near * far) / (far + near - z * (far - near)); // Linearises the depth. 
	float normed = (linear - near) / (far - near);                       // Normalises the range to [0, 1]. 
	
	return normed; 
} 

void main(void) 
{
	// Fade effect based on distance from camera.
	//

	// Apply a brightness effect (make colour more white) when fragments are close to the camera.
	float s      = linearize_depth(gl_FragCoord.z, 1.0, 20.0); 
	vec4  shine  = (0.9 - s * s) * vec4(1.0, 1.0, 1.0, fragment_color.w) \
	             + (0.1 + s * s) * fragment_color; 
	
	// Apply a darkness effect (make colour more like background colour) when fragments are far.
	float z    = linearize_depth(gl_FragCoord.z, 1.0, 30.0); 
	vec4  fade = (1.0 - z * z) * shine \
	           + z * z         * vec4(0.2, 0.3, 0.3, fragment_color.w); 
	
	pixel_color = vec4(fade.x, fade.y, fade.z, fragment_color.w); 
} 
