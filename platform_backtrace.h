#pragma once

#include "platform.h"

#if PLATFORM_WINDOWS
#include "win32_backtrace.h"
#elif PLATFORM_UNIX
#include "unix_backtrace.h"
#else
#error new platform
#endif
