#pragma once

#include "platform.h"

#if PLATFORM_WINDOWS

#include <windows.h>
#include <io.h>

#if !COMPILER_MINGW
typedef int ssize_t;
#endif

#define snprintf _snprintf
#define mkdir( path, mode ) ( CreateDirectoryA( path, NULL ) != 0 ? 0 : -1 )
#define rmdir( path ) ( RemoveDirectory( path ) != 0 ? 0 : -1 )

#elif PLATFORM_UNIX
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#else
#error new platform
#endif
