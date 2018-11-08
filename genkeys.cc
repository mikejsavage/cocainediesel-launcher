#include <stdio.h>

#include "intrinsics.h"
#include "log.h"
#include "ggentropy.h"

#include "libs/monocypher/monocypher.h"

int main( int argc, char ** argv ) {
	u8 secret_key[ 32 ];
	if( !ggentropy( secret_key, sizeof( secret_key ) ) )
		FATAL( "ggentropy" );

	u8 public_key[ 32 ];
	crypto_sign_public_key( public_key, secret_key );

	printf( "const u8 public_key[] = {" );

	for( size_t i = 0; i < sizeof( public_key ); i++ ) {
		if( i % 8 == 0 ) {
			ggprint( "\n\t" );
		}
		else {
			ggprint( " " );
		}

		ggprint( "0x{02x},", public_key[ i ] );
	}

	printf( "\n};\n" );

	printf( "const u8 secret_key[] = {" );

	for( size_t i = 0; i < sizeof( secret_key ); i++ ) {
		if( i % 8 == 0 ) {
			ggprint( "\n\t" );
		}
		else {
			ggprint( " " );
		}

		ggprint( "0x{02x},", secret_key[ i ] );
	}

	printf( "\n};\n" );

	return 0;
}
