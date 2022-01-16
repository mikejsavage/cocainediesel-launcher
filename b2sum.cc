#include <stdio.h>

#include "intrinsics.h"
#include "ggformat.h"

#include "libs/blake2b-avx/blake2.h"

int main() {
	blake2b_state blake;
	blake2b_init( &blake, 32 );

	while( true ) {
		u8 buf[ 8192 ];
		int n = checked_cast< int >( fread( buf, 1, sizeof( buf ), stdin ) );
		if( n == 0 )
			break;
		if( n < 0 ) {
			perror( "" );
			return 1;
		}

		blake2b_update( &blake, buf, size_t( n ) );
	}

	u8 digest[ 32 ];
	blake2b_final( &blake, digest, sizeof( digest ) );

	for( size_t i = 0; i < sizeof( digest ); i++ ) {
		ggprint( "{02x}", digest[ i ] );
	}
	ggprint( "\n" );

	return 0;
}
