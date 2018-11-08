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

inline void format( FormatBuffer * fb, array< const char > arr, const FormatOpts & opts ) {
	if( fb->len < fb->capacity ) {
		size_t len = min( arr.n + 1, fb->capacity - fb->len );
		strlcpy( fb->buf + fb->len, arr.ptr(), len );
	}
	fb->len += arr.n;
}
