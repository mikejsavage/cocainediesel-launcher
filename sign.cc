#include <stdio.h>

#include "intrinsics.h"
#include "secret_key.h"

#include "libs/monocypher/monocypher.h"

int main( int argc, char ** argv ) {
	if( argc != 2 ) {
		fprintf( stderr, "usage: %s <path/to/manifest.txt>\n", argv[ 0 ] );
		return 1;
	}

	size_t manifest_len;
	const char * manifest = file_get_contents_or_empty( argv[ 1 ], &manifest_len );
	if( strlen( manifest ) == 0 ) {
		fprintf( stderr, "empty manifest\n" );
		return 1;
	}

	u8 signature[ 64 ];
	crypto_sign( signature, secret_key, NULL, manifest, manifest_len );

	for( size_t i = 0; i < sizeof( signature ); i++ ) {
		printf( "%02x", signature[ i ] );
	}

	printf( "\n" );

	return 0;
}
