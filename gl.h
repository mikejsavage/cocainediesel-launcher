#pragma once

#include "intrinsics.h"

struct v2u32 {
	u32 x, y;

	v2u32() { }

	explicit v2u32( u32 a, u32 b ) {
		x = a;
		y = b;
	}
};

struct GLFWwindow;

GLFWwindow * gl_init();
void gl_term();

v2u32 get_window_size();
float get_aspect_ratio();
