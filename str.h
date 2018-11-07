#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intrinsics.h"
#include "strlcpy.h"
#include "ggformat.h"

template< size_t N >
class str {
public:
	STATIC_ASSERT( N > 0 );

	str() {
		clear();
	}

	template< typename... Rest >
	str( const char * fmt, const Rest & ... rest ) {
		sprintf( fmt, rest... );
	}

	void clear() {
		buf[ 0 ] = '\0';
		length = 0;
	}

	template< typename T >
	void operator+=( const T & x ) {
		appendf( "{}", x );
	}

	template< typename... Rest >
	void sprintf( const char * fmt, const Rest & ... rest ) {
		size_t copied = ggformat( buf, N, fmt, rest... );
		length = min( copied, N - 1 );
	}

	template< typename... Rest >
	void appendf( const char * fmt, const Rest & ... rest ) {
		size_t copied = ggformat( buf + length, N - length, fmt, rest... );
		length += min( copied, N - length - 1 );
	}

	void truncate( size_t n ) {
		if( n >= length ) {
			return;
		}
		buf[ n ] = '\0';
		length = n;
	}

	char & operator[]( size_t i ) {
		ASSERT( i < N );
		return buf[ i ];
	}

	const char & operator[]( size_t i ) const {
		ASSERT( i < N );
		return buf[ i ];
	}

	const char * c_str() const {
		return buf;
	}

	size_t len() const {
		return length;
	}

	bool operator==( const char * rhs ) const {
		return strcmp( buf, rhs ) == 0;
	}

	bool operator!=( const char * rhs ) const {
		return !( *this == rhs );
	}

private:
	char buf[ N ];
	size_t length;
};

template< size_t N >
void format( FormatBuffer * fb, const str< N > & buf, const FormatOpts & opts ) {
	format( fb, buf.c_str(), opts );
}
