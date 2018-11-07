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
	const u8 * manifest = file_get_contents( argv[ 1 ], &manifest_len );

	u8 signature[ 64 ];
	crypto_sign( signature, secret_key, NULL, manifest, manifest_len );

	for( size_t i = 0; i < sizeof( signature ); i++ ) {
		printf( "%02x", signature[ i ] );
	}

	printf( "\n" );

	return 0;
}
