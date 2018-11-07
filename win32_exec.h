#pragma once

#include <windows.h>
#include <shellapi.h>

#include "log.h"

#pragma warning( push )
#pragma warning( disable : 4302 )
#pragma warning( disable : 4311 )

void exec_and_quit( const char * path, char * command_line = NULL ) {
	int ok = int( ShellExecute( NULL, "runas", path, command_line, NULL, SW_SHOWDEFAULT ) );
	if( ok == SE_ERR_ACCESSDENIED )
		return;
	if( ok <= 32 )
		FATAL( "ShellExecute" );

	exit( 0 );
}

#pragma warning( pop )
