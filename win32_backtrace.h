#pragma once

#include <windows.h>
#include <errno.h>

#pragma warning( push )
#pragma warning( disable : 4091 )
#include <dbghelp.h>
#pragma warning( pop )

inline void print_backtrace_and_abort() {
	{
		int error = errno;
		printf( "errno = %d: %s\n", error, strerror( error ) );
	}
	{
		DWORD error = GetLastError();
		char buf[ 1024 ];
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
			MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buf, sizeof( buf ), NULL );
		printf( "GetLastError = %d: %s", error, buf );
	}

	const int max_symbol_len = 1024;

	HANDLE process = GetCurrentProcess();
	SymInitialize( process, NULL, TRUE );

	void * stack[ 128 ];
	WORD num_frames = CaptureStackBackTrace( 0, 128, stack, NULL );

	for( WORD i = 0; i < num_frames; i++ ) {
		char symbol_mem[ sizeof( SYMBOL_INFO ) + max_symbol_len * sizeof( TCHAR ) ];
		SYMBOL_INFO * symbol = ( SYMBOL_INFO * ) symbol_mem;
		symbol->MaxNameLen = max_symbol_len;
		symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

		DWORD64 addr64 = DWORD64( stack[ i ] );
		SymFromAddr( process, addr64, NULL, symbol );

		IMAGEHLP_LINE64 line;
		DWORD col;
		line.SizeOfStruct = sizeof( line );
		BOOL has_file_info = SymGetLineFromAddr64( process, addr64, &col, &line );

		if( has_file_info ) {
			printf( "%s (%s:%d - 0x%08I64x)\n", symbol->Name, line.FileName, line.LineNumber, symbol->Address );
		}
		else {
			printf( "%s (0x%08I64x)\n", symbol->Name, symbol->Address );
		}
	}

	__debugbreak();
}
