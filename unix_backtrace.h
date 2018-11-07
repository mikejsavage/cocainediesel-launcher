#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <execinfo.h>

inline void print_backtrace_and_abort() {
	int error = errno;
	printf( "errno = %d: %s\n", error, strerror( error ) );

	void * stack[ 128 ];
	const int stack_size = backtrace( stack, 128 );
	backtrace_symbols_fd( stack, stack_size, STDERR_FILENO );
	abort();
}
