#include "linmath/linmath.h"

#include <iso646.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>



/* Convert an HSVA colour to an RGBA colour. 
 * Implementation of an algorithm available at: https://en.wikipedia.org/wiki/HSV_color_space#From_HSV
 */
void HSVA_to_RGBA(float h, float s, float v, float a, vec4* rgba_out)
{
	float* O = (float*)*rgba_out;
	static const unsigned int R = 0;
	static const unsigned int G = 1;
	static const unsigned int B = 2;
	static const unsigned int A = 3;
	O[A] = a;
	
	// The chroma of an RGB colour is c = max(r, g, b) - min(r, g, b), and the chroma of an HSV colour is c = sv.
	// We can ensure we generate a colour with the correct colour by ensuring the minimum component is 0 and the
	// maximum component is sv.
	float              c = s * v;
	static const float z = 0.0;
	if (c < 1e-4) { O[R] = z; O[G] = z; O[B] = z; return; } // A chroma of 0 always corresponds to pure black.
	
	// The RGB colour cube has six edges corresponding to hue spectra between a primary and a secondary colour.
	// The intermediate component has to be chosen so that we are the correct distance between them, i.e.
	// it should be 0 again for a pure primary hue, and sv again for a pure secondary hue. Hue varies like this:
	// red ----- yellow -- green --- cyan ---- blue ---- magenta - (red)
	// 0.00      0.17      0.33      0.50      0.67      0.83      1.00
	// So the following formula gets the distance along:
	float x = c * (1.0 - fabs(fmod(6.0 * h, 2.0) - 1.0));
	// This is because 1.0 - abs(mod(6h, 2) - 1) goes like this:
	// 1.0|        ..''..              ..''..              ..''..
	//    |    ..''      ''..      ..''      ''..      ..''      ''..
	//    |..''              ''..''              ''..''              ''..
	// 0.0x--------------------------------------------------------------
	// red^      yellow^   green^    cyan^     blue^     magenta^  (red)^
	// i.e. it is 0.0 for primary colours and 1.0 for secondary colours and linearly intermediate in-between.
	
	// We order the components differently based on which segment of the above spectrum we are in, with the
	// largest component corresponding to the primary edge of the segment.
	// TODO. We could do this without branching, based on permutating a template, outlined here: 
	// http://stackoverflow.com/questions/3018313/
	// algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both/19400360#19400360
	if      (h >= 0.00 and h < 0.17) { O[R] = c; O[G] = x; O[B] = z; } // S.1: c x 0
	else if (h >= 0.33 and h < 0.50) { O[R] = z; O[G] = c; O[B] = x; } // S.3: 0 c x
	else if (h >= 0.67 and h < 0.83) { O[R] = x; O[G] = z; O[B] = c; } // S.5: x 0 c
	else if (h >= 0.17 and h < 0.33) { O[R] = x; O[G] = c; O[B] = z; } // S.2: x c 0
	else if (h >= 0.50 and h < 0.67) { O[R] = z; O[G] = x; O[B] = c; } // S.4: 0 x c
	else if (h >= 0.83 and h < 1.00) { O[R] = c; O[G] = z; O[B] = x; } // S.6: c 0 x
	
	// In RGB space, the value is simply the largest component. Currently our largest component has the numeric
	// value c, so we need to add v - c to all components to match value while maintaining hue and chroma.
	float d = v - c;
	O[R] += d;
	O[G] += d;
	O[B] += d;
	
	//printf("[DEBUG] Hue, saturation, value, alpha: %.1f, %.1f, %.1f, %.1f.\n", h, s, v, a);
	//printf("[DEBUG] Chroma, intermediate, zero, delta: %.1f, %.1f, %.1f, %.1f.\n", c, x, z, d);
	//printf("[DEBUG] Red, green, blue, alpha: %.1f, %.1f, %.1f, %.1f.\n", O[R], O[G], O[B], O[A]);
}



/* Generates a set of size n of visually distinct pastel colours.
 */
void generate_pastel_colors(vec4* colors, unsigned int n, float a)
{
	static const float PASTEL_SATURATION = 0.4; // Pastel colours are defined, in part, by being quite grey.
	
	// Cycle through secondary, then primary, then tertiary colours. Pure red is omitted as a reserved colour.
	float pattern[11] = \
	{ \
		0.17, 0.50, 0.83, \
		      0.33, 0.67, /* 0.00 omitted */ \
		0.08, 0.42, 0.75, \
		0.25, 0.58, 0.92 \
	};
	
	// Generate n colours. We cycle through pattern, and each time we reach the end we change the value parameter
	// of the colours generated.
	vec4         color;
	unsigned int i      = 0;
	unsigned int j      = 0;
	float        v_light = 0.70; // Pastel colours have high value...
	float        v_dark  = 0.60; //
	float        s_light = 0.30; // ...and low saturation.
	float        s_dark  = 0.90; //
	float        delta_v = (v_light - v_dark) / (float)ceil((float)n / 11.0);
	float        delta_s = (s_light - s_dark) / (float)ceil((float)n / 11.0);
	float        v       = v_light;
	float        s       = s_light;
	while (i < n)
	{
		// If j is 11, we have cycled through the pattern, so start again but with darker (less-value) colours.
		if (j == 11)
		{
			v -= delta_v;
			s -= delta_s;
			j  = 0;
		}
		
		// Generate the colour.
		HSVA_to_RGBA(pattern[j], s, v, a, &color);
		memcpy(colors[i], color, sizeof(vec4));
		++i, ++j;
	}
}



/* TODO.
 */
void repeat_color(const vec4 color, unsigned int n, \
                  vec4** colors_out)
{
	// Set the residue colours to the specified colour.
	*colors_out = (vec4*)malloc(n * sizeof(vec4)); // malloc colors
	vec4* colors = *colors_out;
	for (unsigned int i = 0; i < n; ++i)
		memcpy(colors[i], color, sizeof(vec4));
}
