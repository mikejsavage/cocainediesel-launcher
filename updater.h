#pragma once

#include "intrinsics.h"
#include "ggformat.h"
#include "array.h"
#include "str.h"

#define BLAKE2B256_DIGEST_LENGTH 32

enum UpdaterState {
	UpdaterState_Init,

	UpdaterState_DownloadingVersion,
	UpdaterState_DownloadingVersion_Retry,

	UpdaterState_DownloadingManifest,
	UpdaterState_DownloadingManifest_Retry,

	UpdaterState_NeedsUpdate,
	UpdaterState_StartUpdate,

	// UpdaterState_ReservingSpace,

	UpdaterState_DownloadingUpdate,

	UpdaterState_InstallingUpdate,
	UpdaterState_InstallingUpdateFailed,

	UpdaterState_ReadyToPlay,
};

struct Version {
	u32 a, b, c, d;
};

inline void format( FormatBuffer * fb, const Version & v, const FormatOpts & opts ) {
	str< 128 > s( "{}.{}.{}.{}", v.a, v.b, v.c, v.d );
	format( fb, s, opts );
}

typedef void ( * LogCallback )( const char * msg );

void updater_init( bool autostart, LogCallback log_cb, const char * version );
void updater_term();

void updater_start_update();

UpdaterState updater_update();
UpdaterState updater_wait();

Version updater_local_version();
Version updater_remote_version();
u64 updater_update_size();
u64 updater_bytes_downloaded();
float updater_download_progress();
double updater_download_speed();
