#pragma once

#include "platform.h"

#if PLATFORM_WINDOWS
#include "win32_fs.h"
#elif PLATFORM_UNIX
#include "unix_fs.h"
#else
#error new platform
#endif
