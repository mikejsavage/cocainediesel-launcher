#pragma once

#include <unistd.h>

#include "log.h"

inline void run_game( const char * path ) {
	execl( path, path, ( char * ) 0 );
	FATAL( "execl" );
}

inline bool open_in_browser( const char * url ) {
	int child = fork();
	if( child == -1 )
		return false;

	if( child != 0 )
		return true;

	execlp( "xdg-open", "xdg-open", url, ( char * ) 0 );
	FATAL( "execlp" );

	return false;
}
