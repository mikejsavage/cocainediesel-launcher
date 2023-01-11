#pragma once

#include "intrinsics.h"

#define GLFW_INCLUDE_NONE
#include "libs/glfw3/GLFW/glfw3.h"

void taskbar_init();
void taskbar_term();

void taskbar_progress( GLFWwindow * window, u64 completed, u64 total );
void taskbar_clear( GLFWwindow * window );
