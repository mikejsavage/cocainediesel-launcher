#pragma once

#include <windows.h>
#include <shellapi.h>

#include "log.h"
#include "str.h"

#pragma warning( push )
#pragma warning( disable : 4302 )
#pragma warning( disable : 4311 )

inline void exec_and_quit( const char * path, const char * command_line = NULL ) {
	wchar_t * wide_path = UTF8ToWide( path );
	wchar_t * wide_command_line = command_line == NULL ? NULL : UTF8ToWide( command_line );
	defer { free( wide_path ); };
	defer { free( wide_command_line ); };

	int ok = int( ShellExecuteW( NULL, L"runas", wide_path, wide_command_line, NULL, SW_SHOWDEFAULT ) );
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
	int ok = int( ShellExecuteA( NULL, "open", url, NULL, NULL, SW_SHOWDEFAULT ) );
	return ok > 32;
}

#pragma warning( pop )
