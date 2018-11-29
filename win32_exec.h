#pragma once

#include <windows.h>
#include <shellapi.h>

#include "log.h"
#include "str.h"

#pragma warning( push )
#pragma warning( disable : 4302 )
#pragma warning( disable : 4311 )

inline void exec_and_quit( const char * path, char * command_line = NULL ) {
	int ok = int( ShellExecute( NULL, "runas", path, command_line, NULL, SW_SHOWDEFAULT ) );
	if( ok == SE_ERR_ACCESSDENIED )
		return;
	if( ok <= 32 )
		FATAL( "ShellExecute" );

	exit( 0 );
}

inline void run_game( const char * path, const char * discord_id ) {
	if( discord_id == NULL )
		exec_and_quit( path );
	str< 256 > arg( "+setau cl_discord {}", discord_id );
	exec_and_quit( path, arg.c_str() );
}

inline bool open_in_browser( const char * url ) {
	int ok = int( ShellExecute( NULL, "runas", path, command_line, NULL, SW_SHOWDEFAULT ) );
	return ok > 32;
}

#pragma warning( pop )
