#include <stdio.h>

#include "intrinsics.h"
#include "ggformat.h"

#include "libs/monocypher/monocypher.h"

int main() {
	crypto_blake2b_ctx blake;
	crypto_blake2b_general_init( &blake, 32, NULL, 0 );

	while( true ) {
		u8 buf[ 8192 ];
		ssize_t n = fread( buf, 1, sizeof( buf ), stdin );
		if( n == 0 )
			break;
		if( n < 0 ) {
			perror( "" );
			return 1;
		}

		crypto_blake2b_update( &blake, buf, size_t( n ) );
	}

	u8 digest[ 32 ];
	crypto_blake2b_final( &blake, digest );

	for( size_t i = 0; i < sizeof( digest ); i++ ) {
		ggprint( "{02x}", digest[ i ] );
	}
	ggprint( "\n" );

	return 0;
}
