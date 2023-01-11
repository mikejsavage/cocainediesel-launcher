#include <stddef.h>
#include <string.h>

void SafeStrCpy( char * dst, const char * src, size_t dst_size ) {
	if( dst_size == 0 )
		return;
	size_t src_len = strlen( src );
	size_t to_copy = src_len < dst_size ? src_len : dst_size - 1;
	memcpy( dst, src, to_copy );
	dst[ to_copy ] = '\0';
}
