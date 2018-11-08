#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"
#include "platform_backtrace.h"

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef intptr_t sptr;
typedef uintptr_t uptr;

#define S8_MAX s8( INT8_MAX )
#define S16_MAX s16( INT16_MAX )
#define S32_MAX s32( INT32_MAX )
#define S64_MAX s64( INT64_MAX )
#define S8_MIN s8( INT8_MIN )
#define S16_MIN s16( INT16_MIN )
#define S32_MIN s32( INT32_MIN )
#define S64_MIN s64( INT64_MIN )

#define U8_MAX u8( UINT8_MAX )
#define U16_MAX u16( UINT16_MAX )
#define U32_MAX u32( UINT32_MAX )
#define U64_MAX u64( UINT64_MAX )

#define S8 INT8_C
#define S16 INT16_C
#define S32 INT32_C
#define S64 INT64_C
#define U8 UINT8_C
#define U16 UINT16_C
#define U32 UINT32_C
#define U64 UINT64_C

#define PI 3.14159265359f

#define STRINGIFY_HELPER( x ) #x
#define STRINGIFY( x ) STRINGIFY_HELPER( x )

#define CONCAT_HELPER( a, b ) a##b
#define CONCAT( a, b ) CONCAT_HELPER( a, b )
#define COUNTER_NAME( x ) CONCAT( x, __COUNTER__ )
#define LINE_NAME( x ) CONCAT( x, __LINE__ )

inline void assert_impl( const bool predicate, const char * message ) {
	if( !predicate ) {
		puts( message );
		print_backtrace_and_abort();
	}
}

#define ASSERT( predicate ) assert_impl( predicate, "\x1b[1;31massertion failed at " __FILE__ " line " STRINGIFY( __LINE__ ) ": \x1b[0;1m" #predicate "\x1b[0m" )

#define STATIC_ASSERT( p ) static_assert( p, #p )

template< typename T, size_t N >
char ( &ArrayCountObj( const T ( & )[ N ] ) )[ N ];
#define ARRAY_COUNT( arr ) ( sizeof( ArrayCountObj( arr ) ) )

#define is_power_of_2( n ) ( ( ( n ) & ( ( n ) - 1 ) ) == 0 )

#define align_power_of_2( n, alignment ) ( ( ( n ) + ( alignment ) - 1 ) & ~( ( alignment ) - 1 ) )
#define align2( n ) align_power_of_2( n, 2 )
#define align4( n ) align_power_of_2( n, 4 )
#define align8( n ) align_power_of_2( n, 8 )
#define align16( n ) align_power_of_2( n, 16 )
#define align_arbitrary( n, alignment ) ( ( ( n ) + ( alignment ) - 1 ) / ( alignment ) * ( alignment ) )

#define kilobytes( kb ) ( size_t( kb ) * size_t( 1024 ) )
#define megabytes( mb ) ( kilobytes( mb ) * size_t( 1024 ) )
#define gigabytes( gb ) ( megabytes( gb ) * size_t( 1024 ) )

template< typename T >
constexpr T min( T a, T b ) {
	return a < b ? a : b;
}

#define NONCOPYABLE( T ) T( const T & ) = delete; void operator=( const T & ) = delete;

template< typename F >
struct ScopeExit {
	ScopeExit( F f_ ) : f( f_ ) { }
	~ScopeExit() { f(); }
	F f;
};

template< typename F >
inline ScopeExit< F > MakeScopeExit( F f ) {
	return ScopeExit< F >( f );
};
#define SCOPE_EXIT( code ) auto COUNTER_NAME( SCOPE_EXIT_ ) = MakeScopeExit( [&]() { code; } )

template< typename S, typename T >
struct SameType {
	enum { value = false };
};
template< typename T >
struct SameType< T, T > {
	enum { value = true };
};

#define SAME_TYPE( S, T ) SameType< S, T >::value

template< typename To, typename From >
inline To checked_cast( const From & from ) {
	To result = To( from );
	ASSERT( From( result ) == from );
	return result;
}

// TODO: this sucks
inline u8 * file_get_contents( const char * path, size_t * out_len = NULL ) {
	FILE * file = fopen( path, "rb" );
	ASSERT( file != NULL );

	fseek( file, 0, SEEK_END );
	size_t len = checked_cast< size_t >( ftell( file ) );
	ASSERT( len < SIZE_MAX );
	fseek( file, 0, SEEK_SET );

	u8 * contents = ( u8 * ) malloc( len + 1 );
	size_t bytes_read = fread( contents, 1, len, file );
	contents[ len ] = '\0';
	ASSERT( bytes_read == len );

	if( out_len ) *out_len = len;

	fclose( file );

	return contents;
}

template< typename T >
inline T * realloc_array( T * old, size_t count ) {
	ASSERT( SIZE_MAX / count >= sizeof( T ) );
	return ( T * ) realloc( old, count * sizeof( T ) );
}
