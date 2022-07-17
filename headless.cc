#include "intrinsics.h"
#include "updater.h"
#include "ggformat.h"
#include "platform_time.h"

static void log_cb( const char * msg ) {
	printf( "%s\n", msg );
}

int main( int argc, char ** argv ) {
	if( argc > 2 ) {
		printf( "Usage: %s [optional target version, e.g. v1.2.3.4]\n", argv[ 0 ] );
		return 1;
	}

	updater_init( true, log_cb, argc == 2 ? argv[ 1 ] : NULL );
	updater_wait();

	ggprint( "Local version {}\n", updater_local_version() );

	while( true ) {
		UpdaterState state = updater_wait();

		if( state == UpdaterState_ReadyToPlay ) {
			break;
		}
		else if( state == UpdaterState_NeedsUpdate ) {
			ggprint( "Updating to {}\n", updater_remote_version() );
		}
		else if( state == UpdaterState_DownloadingUpdate ) {
			float progress = updater_download_progress();

			const char * unit = "KB/s";
			double display_speed = updater_download_speed() / 1000.0;
			if( display_speed > 1000.0 ) {
				unit = "MB/s";
				display_speed /= 1000.0;
			}

			static u64 last_print = 0;
			double now = get_time();
			if( last_print == 0 || u64( now ) / 5 != last_print ) {
				ggprint( "{.2}/{.2}MB. {.2}% {.2}{}\n",
					updater_bytes_downloaded() / 1000.0 / 1000.0,
					updater_update_size() / 1000.0 / 1000.0,
					progress * 100,
					display_speed,
					unit
					);
				last_print = u64( now ) / 5;
			}
		}
	}

	updater_term();

	return 0;
}
