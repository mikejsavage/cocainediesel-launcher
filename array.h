#pragma once

#include <stdlib.h>

#include "intrinsics.h"
#include "ggformat.h"
#include "strlcpy.h"
#include "log.h"

template< typename T >
class array {
public:
	array() {
		n = 0;
	}

	array( T * memory, size_t count ) {
		ASSERT( count == 0 || memory != NULL );
		n = count;
		elems = memory;
	}

	T & operator[]( size_t i ) {
		ASSERT( i < n );
		return elems[ i ];
	}

	const T & operator[]( size_t i ) const {
		ASSERT( i < n );
		return elems[ i ];
	}

	array< T > operator+( size_t i ) {
		ASSERT( i <= n );
		return array< T >( elems + i, n - i );
	}

	const array< T > operator+( size_t i ) const {
		ASSERT( i <= n );
		return array< T >( elems + i, n - i );
	}

	bool in_range( size_t i ) const {
		return i < n;
	}

	T * ptr() {
		return elems;
	}

	const T * ptr() const {
		return elems;
	}

	size_t num_bytes() const {
		return sizeof( T ) * n;
	}

	T * begin() {
		return elems;
	}

	T * end() {
		return elems + n;
	}

	const T * begin() const {
		return elems;
	}

	const T * end() const {
		return elems + n;
	}

	array< T > slice( size_t start, size_t one_past_end ) {
		ASSERT( start <= one_past_end );
		ASSERT( one_past_end <= n );
		return array< T >( elems + start, one_past_end - start );
	}

	const array< T > slice( size_t start, size_t one_past_end ) const {
		ASSERT( start <= one_past_end );
		ASSERT( one_past_end <= n );
		return array< T >( elems + start, one_past_end - start );
	}

	template< typename S >
	array< S > cast() {
		ASSERT( num_bytes() % sizeof( S ) == 0 );
		return array< S >( ( S * ) ptr(), num_bytes() / sizeof( S ) );
	}

	template< typename S >
	const array< S > cast() const {
		ASSERT( num_bytes() % sizeof( S ) == 0 );
		return array< S >( ( S * ) ptr(), num_bytes() / sizeof( S ) );
	}

	size_t n;

protected:
	T * elems;
};

template< typename T >
class array2d {
public:
	array2d() {
		w = h = 0;
	}

	array2d( T * memory, size_t width, size_t height ) {
		ASSERT( width * height == 0 || memory != NULL );
		w = width;
		h = height;
		elems = memory;
	}

	array2d( array< T > a, size_t width, size_t height ) {
		ASSERT( width * height == a.n );
		elems = a.ptr();
		w = width;
		h = height;
	}

	T & operator()( size_t x, size_t y ) {
		ASSERT( in_range( x, y ) );
		return elems[ y * w + x ];
	}

	const T & operator()( size_t x, size_t y ) const {
		ASSERT( in_range( x, y ) );
		return elems[ y * w + x ];
	}

	bool in_range( size_t x, size_t y ) const {
		return x < w && y < h;
	}

	T try_get( size_t x, size_t y, T def ) const {
		if( !in_range( x, y ) )
			return def;
		return elems[ y * w + x ];
	}

	T * ptr() {
		return elems;
	}

	const T * ptr() const {
		return elems;
	}

	array< T > row( size_t r ) {
		return array< T >( elems + r * w, w );
	}

	const array< T > row( size_t r ) const {
		return array< T >( elems + r * w, w );
	}

	size_t num_bytes() const {
		return sizeof( T ) * w * h;
	}

	template< typename S >
	array2d< S > cast() {
		ASSERT( sizeof( S ) == sizeof( T ) );
		return array2d< S >( ( S * ) ptr(), w, h );
	}

	template< typename S >
	const array2d< S > cast() const {
		ASSERT( sizeof( S ) == sizeof( T ) );
		return array2d< S >( ( S * ) ptr(), w, h );
	}

	size_t w, h;

private:
	T * elems;
};

// TODO: sucky duplication
template< typename T, size_t N >
class StaticArray {
public:
	T & operator[]( size_t i ) {
		ASSERT( i < N );
		return elems[ i ];
	}

	const T & operator[]( size_t i ) const {
		ASSERT( i < N );
		return elems[ i ];
	}

	array< T > operator+( size_t i ) {
		ASSERT( i <= N );
		return array< T >( elems + i, N - i );
	}

	const array< T > operator+( size_t i ) const {
		ASSERT( i <= N );
		return array< T >( elems + i, N - i );
	}

	bool in_range( size_t i ) const {
		return i < N;
	}

	T try_get( size_t i, T def ) const {
		if( !in_range( i ) )
			return def;
		return elems[ i ];
	}

	T * ptr() {
		return elems;
	}

	const T * ptr() const {
		return elems;
	}

	constexpr size_t size() const {
		return N;
	}

	size_t num_bytes() const {
		return sizeof( T ) * N;
	}

	T * begin() {
		return elems;
	}

	T * end() {
		return elems + N;
	}

	const T * begin() const {
		return elems;
	}

	const T * end() const {
		return elems + N;
	}

	array< T > slice( size_t start, size_t one_past_end ) {
		ASSERT( start <= one_past_end );
		ASSERT( one_past_end <= N );
		return array< T >( elems + start, one_past_end - start );
	}

	const array< T > slice( size_t start, size_t one_past_end ) const {
		ASSERT( start <= one_past_end );
		ASSERT( one_past_end <= N );
		return array< T >( elems + start, one_past_end - start );
	}

	template< typename S >
	array< S > cast() {
		ASSERT( num_bytes() % sizeof( S ) == 0 );
		return array< S >( ( S * ) ptr(), num_bytes() / sizeof( S ) );
	}

	template< typename S >
	const array< S > cast() const {
		ASSERT( num_bytes() % sizeof( S ) == 0 );
		return array< S >( ( S * ) ptr(), num_bytes() / sizeof( S ) );
	}

	operator array< T >() {
		return array< T >( elems, N );
	}

private:
	T elems[ N ];
};

// TODO: pluggable allocators
template< typename T >
class DynamicArray {
public:
	NONCOPYABLE( DynamicArray );

	DynamicArray( size_t initial_capacity = 0 ) {
		n = 0;
		capacity = initial_capacity;
		elems = NULL;
		if( initial_capacity > 0 ) {
			elems = realloc_array( elems, capacity );
		}
	}

	~DynamicArray() {
		free( elems );
	}

	void add( const T & x ) {
		size_t idx = extend( 1 );
		elems[ idx ] = x;
	}

	void clear() {
		n = 0;
	}

	void resize( size_t new_size ) {
		if( new_size < n ) {
			n = new_size;
			return;
		}

		if( new_size <= capacity ) {
			n = new_size;
			return;
		}

		size_t new_capacity = max( size_t( 64 ), capacity );
		while( new_capacity < new_size ) {
			new_capacity *= 2;
		}

		T * new_elems = realloc_array( elems, new_capacity );
		if( new_elems == NULL ) {
			FATAL( "couldn't allocate for DynamicArray" );
		}
		elems = new_elems;
		capacity = new_capacity;
		n = new_size;
	}

	size_t extend( size_t by ) {
		size_t old_size = n;
		resize( n + by );
		return old_size;
	}

	T & operator[]( size_t i ) {
		ASSERT( i < n );
		return elems[ i ];
	}

	const T & operator[]( size_t i ) const {
		ASSERT( i < n );
		return elems[ i ];
	}

	array< T > operator+( size_t i ) {
		ASSERT( i <= n );
		return array< T >( elems + i, n - i );
	}

	const array< T > operator+( size_t i ) const {
		ASSERT( i <= n );
		return array< T >( elems + i, n - i );
	}

	bool in_range( size_t i ) const {
		return i < n;
	}

	T try_get( size_t i, T def ) const {
		if( !in_range( i ) )
			return def;
		return elems[ i ];
	}

	T * ptr() {
		return elems;
	}

	const T * ptr() const {
		return elems;
	}

	size_t size() const {
		return n;
	}

	size_t num_bytes() const {
		return sizeof( T ) * n;
	}

	T * begin() {
		return elems;
	}

	T * end() {
		return elems + n;
	}

	const T * begin() const {
		return elems;
	}

	const T * end() const {
		return elems + n;
	}

	array< T > slice( size_t start, size_t one_past_end ) {
		ASSERT( start <= one_past_end );
		ASSERT( one_past_end <= n );
		return array< T >( elems + start, one_past_end - start );
	}

	const array< T > slice( size_t start, size_t one_past_end ) const {
		ASSERT( start <= one_past_end );
		ASSERT( one_past_end <= n );
		return array< T >( elems + start, one_past_end - start );
	}

	operator const array< T >() const {
		return array< T >( elems, n );
	}

private:
	size_t n;
	size_t capacity;
	T * elems;
};

template< typename T, typename F >
inline void visit( array< T > & a, F f ) {
	f( a.n );
	for( T & x : a ) f( x );
}

template< typename T, typename F >
inline void visit( const array< T > & a, F f ) {
	f( a.n );
	for( const T & x : a ) f( x );
}

template< typename T >
inline array< T > file_get_array( const char * path ) {
	size_t len;
	u8 * mem = file_get_contents( path, &len );

	ASSERT( len % sizeof( T ) == 0 );
	return array< T >( ( T * ) mem, len / sizeof( T ) );
}

template< typename T >
static T bilerp( const array2d< T > arr, float x, float y ) {
	size_t xi = ( size_t ) x;
	size_t yi = ( size_t ) y;
	size_t xi1 = min( xi + 1, arr.w - 1 );
	size_t yi1 = min( yi + 1, arr.h - 1 );

	float xf = x - xi;
	float yf = y - yi;

	T a = arr( xi, yi );
	T b = arr( xi1, yi );
	T c = arr( xi, yi1 );
	T d = arr( xi1, yi1 );

	T ab = lerp( a, xf, b );
	T cd = lerp( c, xf, d );

	return lerp( ab, yf, cd );
}

template< typename T >
static T bilerp01( const array2d< T > arr, float x, float y ) {
	return bilerp( arr, x * arr.w, y * arr.h );
}

inline void format( FormatBuffer * fb, array< const char > arr, const FormatOpts & opts ) {
	if( fb->len < fb->capacity ) {
		size_t len = min( arr.n + 1, fb->capacity - fb->len );
		strlcpy( fb->buf + fb->len, arr.ptr(), len );
	}
	fb->len += arr.n;
}
