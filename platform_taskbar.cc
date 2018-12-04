#include "intrinsics.h"

#if PLATFORM_WINDOWS
#include "win32_taskbar.cc"
#else
void taskbar_init() { }
void taskbar_term() { }

void taskbar_progress( u64 completed, u64 total ) { }
#endif
