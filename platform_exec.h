#pragma once

#include "platform.h"

#if PLATFORM_WINDOWS
#include "win32_exec.h"
#elif PLATFORM_UNIX
#include "unix_exec.h"
#else
#error new platform
#endif
