#include "platform_taskbar.h"

#if PLATFORM_WINDOWS
#include "win32_taskbar.cc"
#else
void taskbar_init() { }
void taskbar_term() { }

void taskbar_progress( GLFWwindow * window, u64 completed, u64 total ) { }
void taskbar_clear( GLFWwindow * window ) { }
#endif
