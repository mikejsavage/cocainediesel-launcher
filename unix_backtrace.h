#pragma once

#include <stdio.h>
#include <errno.h>

inline void print_backtrace_and_abort() {
	int error = errno;
	printf( "errno = %d: %s\n", error, strerror( error ) );

	// void * stack[ 128 ];
	// const int stack_size = backtrace( stack, 128 );
	// backtrace_symbols_fd( stack, stack_size, STDERR_FILENO );
	__builtin_trap();
}
