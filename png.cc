#include "intrinsics.h"
#include "glad.h"

#include "libs/stb/stb_image.h"

STATIC_ASSERT( SAME_TYPE( u32, GLuint ) );

static GLuint upload( const void * data, int w, int h ) {
	GLuint texture;
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

	return texture;
}

u32 load_png( const char * path ) {
	int w, h;
	u8 * img = stbi_load( path, &w, &h, NULL, 4 );
	ASSERT( img != NULL );

	GLuint texture = upload( img, w, h );

	stbi_image_free( img );

	return texture;
}

u32 load_png_memory( const u8 * data, size_t len ) {
	int w, h;
	u8 * img = stbi_load_from_memory( data, checked_cast< int >( len ), &w, &h, NULL, 4 );
	ASSERT( img != NULL );

	GLuint texture = upload( img, w, h );

	stbi_image_free( img );

	return texture;
}
