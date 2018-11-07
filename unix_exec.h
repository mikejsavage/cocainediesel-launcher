#pragma once

#include <unistd.h>

#include "log.h"

void exec_and_quit( const char * path ) {
	execl( path, path, ( char * ) 0 );
	FATAL( "execl" );
}
