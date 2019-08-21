#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "intrinsics.h"
#include "updater.h"
#include "log.h"
#include "ggformat.h"
#include "str.h"
#include "strlcpy.h"
#include "strtonum.h"
#include "patterns.h"
#include "platform_fs.h"
#include "platform_time.h"

#define CURL_STATICLIB
#include "libs/curl/curl.h"

#include "libs/monocypher/monocypher.h"

#include "libs/whereami/whereami.h"

#define HOST "https://update.cocainediesel.fun"

#define HEX256_PATTERN "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x"

#define BLAKE2B256_PATTERN HEX256_PATTERN

#define SIGNATURE_PATTERN HEX256_PATTERN HEX256_PATTERN

struct Blake2b256 {
	StaticArray< u8, BLAKE2B256_DIGEST_LENGTH > digest;

	Blake2b256() { }

	explicit Blake2b256( const void * data, size_t len );
};

struct ManifestEntry {
	Blake2b256 checksum;
	u64 file_size;
	bool platform_specific;
};

enum DownloadState {
	DownloadState_Downloading,
	DownloadState_Done,
	DownloadState_Failed,
	DownloadState_Retry,
};

struct Download {
	Download( const char * u ) : url( u ) { }
	DownloadState state = DownloadState_Downloading;
	std::string url;
	std::string body;
	double retry_at;
};

struct SmoothValue {
	bool first = true;
	double value;
	double latched;
	double last_update_time;
	double last_latch_time;
};

static CURLM * curl_multi;
static LogCallback log_callback;
static bool autostart_update;


struct Updater {
	UpdaterState state = UpdaterState_Init;
	double retry_at;

	Version local_version, remote_version;
	std::unordered_map< std::string, ManifestEntry > local_manifest, remote_manifest;

	std::vector< Download > downloads;

	std::vector< std::string > files_to_update;
	std::vector< std::string > files_to_remove;

	u64 update_size = 0;
	u64 bytes_downloaded = 0;
	u64 last_bytes_downloaded = 0;
	u64 game_size = 0;
	SmoothValue download_speed;
};

static Updater updater;

static const u8 PUBLIC_KEY[] = {
	0x3b, 0x0a, 0xc7, 0xa1, 0xd1, 0x9e, 0xbd, 0x0c,
	0x71, 0x75, 0x4f, 0xfc, 0x2a, 0xef, 0xcf, 0x91,
	0x93, 0xd0, 0x58, 0x94, 0x79, 0xc2, 0xeb, 0x16,
	0x30, 0x74, 0x62, 0x88, 0xe8, 0x18, 0x03, 0xb6,
};

Blake2b256::Blake2b256( const void * data, size_t len ) {
	crypto_blake2b_general( digest.ptr(), digest.num_bytes(), NULL, 0, ( const u8 * ) data, len );
}

static bool operator==( const Blake2b256 & a, const Blake2b256 & b ) {
	return memcmp( a.digest.ptr(), b.digest.ptr(), a.digest.num_bytes() ) == 0;
}

static bool operator!=( const Blake2b256 & a, const Blake2b256 & b ) {
	return !( a == b );
}

static bool operator==( const Version & a, const Version & b ) {
	return a.a == b.a && a.b == b.b && a.c == b.c && a.d == b.d;
}

static bool operator!=( const Version & a, const Version & b ) {
	return !( a == b );
}

static void format( FormatBuffer * fb, const Blake2b256 & h, const FormatOpts & opts ) {
	str< BLAKE2B256_DIGEST_LENGTH * 2 + 1 > s;
	static const char hex[] = "0123456789abcdef";
	for( u8 b : h.digest ) {
		s += hex[ b >> 4 ];
		s += hex[ b & 0x0f ];
	}
	format( fb, s, opts );
}

static void format( FormatBuffer * fb, const std::string & s, const FormatOpts & opts ) {
	format( fb, s.c_str(), opts );
}

static u8 hex2dec( char hex ) {
	if( hex == '0' ) return 0;
	if( hex == '1' ) return 1;
	if( hex == '2' ) return 2;
	if( hex == '3' ) return 3;
	if( hex == '4' ) return 4;
	if( hex == '5' ) return 5;
	if( hex == '6' ) return 6;
	if( hex == '7' ) return 7;
	if( hex == '8' ) return 8;
	if( hex == '9' ) return 9;

	if( hex == 'a' || hex == 'A' ) return 10;
	if( hex == 'b' || hex == 'B' ) return 11;
	if( hex == 'c' || hex == 'C' ) return 12;
	if( hex == 'd' || hex == 'D' ) return 13;
	if( hex == 'e' || hex == 'E' ) return 14;
	if( hex == 'f' || hex == 'F' ) return 15;

	return 255;
}

static bool parse_hex( const array< const char > hex, u8 * bytes ) {
	if( hex.n % 2 != 0 ) {
		return false;
	}

	for( size_t i = 0; i < hex.n / 2; i++ ) {
		u8 a = hex2dec( hex[ i * 2 ] );
		u8 b = hex2dec( hex[ i * 2 + 1 ] );

		if( a == 255 || b == 255 ) {
			return false;
		}

		bytes[ i ] = a * 16 + b;
	}

	return true;
}

static bool parse_digest( Blake2b256 * h, const array< const char > hex ) {
	if( hex.n != h->digest.size() * 2 )
		return false;
	return parse_hex( hex, h->digest.ptr() );
}

static const char * file_get_contents_or_empty( const char * path ) {
	FILE * file = fopen( path, "rb" );
	if( file == NULL )
		return "";

	fseek( file, 0, SEEK_END );
	size_t len = checked_cast< size_t >( ftell( file ) );
	ASSERT( len < SIZE_MAX );
	fseek( file, 0, SEEK_SET );

	char * contents = ( char * ) malloc( len + 1 );
	if( contents == NULL )
		FATAL( "malloc" );
	size_t bytes_read = fread( contents, 1, len, file );
	contents[ len ] = '\0';
	ASSERT( bytes_read == len );

	fclose( file );

	return contents;
}

static void update_smooth( SmoothValue * smooth, double value, double now ) {
	if( smooth->first ) {
		smooth->first = false;
		smooth->value = value;
		smooth->latched = value;
		smooth->last_update_time = now;
	}
	else {
		double dt = now - smooth->last_update_time;
		smooth->value += ( value / dt - smooth->value ) / ( 0.5 / dt );

		if( now - smooth->last_latch_time > 0.5 ) {
			smooth->latched = smooth->value;
			smooth->last_latch_time = now;
		}
	}

	smooth->last_update_time = now;
}

static std::string get_executable_path( int * dir_length ) {
	int path_length = wai_getExecutablePath( NULL, 0, NULL );
	std::string result;
	result.resize( path_length );
	wai_getExecutablePath( &result[ 0 ], path_length, dir_length );
	return result;
}

static std::string get_executable_directory() {
	int dir_length;
	std::string result = get_executable_path( &dir_length );
	result.resize( dir_length );
	return result;
}

template< typename... Rest >
static std::string ggformat_string( const char * fmt, const Rest & ... rest ) {
	size_t space_required = ggformat( nullptr, 0, fmt, rest... );

	std::string s;
	s.resize( space_required + 1 ); // + 1 so there's space for the null terminator...
	ggformat( &s[ 0 ], space_required + 1, fmt, rest... );
	s.resize( space_required ); // ...and then trim it off

	return s;
}

template< typename... Rest >
static void log( const char * fmt, const Rest & ... rest ) {
	std::string s = ggformat_string( fmt, rest... );
	log_callback( s.c_str() );
}

template< typename... Rest >
static void log_errno( const char * fmt, const Rest & ... rest ) {
	std::string s = ggformat_string( fmt, rest... );
	s += ": ";
	s += strerror( errno );
	log_callback( s.c_str() );
}

#if PLATFORM_WINDOWS

#include <windows.h>

template< typename... Rest >
static void log_gle( const char * fmt, const Rest & ... rest ) {
	std::string s = ggformat_string( fmt, rest... );
	s += ": ";

	DWORD error = GetLastError();
	char buf[ 1024 ];
	FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buf, sizeof( buf ), NULL );
	s += buf;

	log_callback( s.c_str() );
}

#else

template< typename... Rest >
static void log_gle( const char * fmt, const Rest & ... rest ) {
	log_errno( fmt, rest... );
}

#endif

#if PLATFORM_WINDOWS

static void set_registry_key( HKEY hkey, const char * path, const char * value, const char * data ) {
	HKEY key;
	LONG ok_open = RegOpenKeyExA( hkey, path, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &key );
	if( ok_open != ERROR_SUCCESS ) {
		log( "Couldn't open registry key {} {} ({})", path, value, ok_open );
		return;
	}
	defer { RegCloseKey( key ); };

	LONG ok_set = RegSetValueExA( key, value, 0, REG_SZ, ( const BYTE * ) data, checked_cast< DWORD >( strlen( data ) + 1 ) );
	if( ok_set != ERROR_SUCCESS ) {
		log( "Couldn't write registry key ({})", ok_set );
	}
}

static void set_registry_key( HKEY hkey, const char * path, const char * value, u32 data ) {
	HKEY key;
	LONG ok_open = RegOpenKeyExA( hkey, path, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &key );
	if( ok_open != ERROR_SUCCESS ) {
		log( "Couldn't open registry key {} {} ({})", path, value, ok_open );
		return;
	}
	defer { RegCloseKey( key ); };

	LONG ok_set = RegSetValueExA( key, value, 0, REG_DWORD, ( const BYTE * ) &data, sizeof( data ) );
	if( ok_set != ERROR_SUCCESS ) {
		log( "Couldn't write registry key ({})", ok_set );
	}
}

#endif

template< typename F >
void try_set_opt( CURL * curl, CURLoption option, F f ) {
	STATIC_ASSERT( !SAME_TYPE( F, int ) || SAME_TYPE( int, long ) );

	CURLcode r = curl_easy_setopt( curl, option, f );
	if( r != CURLE_OK )
		FATAL( "curl_easy_setopt: {}", curl_easy_strerror( r ) );
}

static size_t data_received( char * data, size_t size, size_t nmemb, void * user_data ) {
	Download * dl = ( Download * ) user_data;
	size_t len = size * nmemb;

	dl->body.append( data, len );
	updater.bytes_downloaded += len;

	return len;
}

static void download( const char * url, Download * reuse = NULL ) {
	log( "Downloading {}", url );

	CURL * curl = curl_easy_init();
	if( curl == NULL )
		FATAL( "curl_easy_init" );

	if( reuse == NULL ) {
		updater.downloads.push_back( Download( url ) );
		/*
		 * we either only download one thing at a time (version/manifest)
		 * or have already reserved enough space in the downloads vector, so
		 * taking a pointer here is ok
		 */
		reuse = &updater.downloads.back();
	}

	try_set_opt( curl, CURLOPT_URL, url );
	try_set_opt( curl, CURLOPT_WRITEFUNCTION, data_received );
	try_set_opt( curl, CURLOPT_FOLLOWLOCATION, 1l );
	try_set_opt( curl, CURLOPT_NOSIGNAL, 1l );
	try_set_opt( curl, CURLOPT_CONNECTTIMEOUT, 10l );
	try_set_opt( curl, CURLOPT_LOW_SPEED_TIME, 10l );
	try_set_opt( curl, CURLOPT_LOW_SPEED_LIMIT, 10l );

	str< 128 > range( "{}-", reuse->body.size() );
	try_set_opt( curl, CURLOPT_RANGE, range.c_str() );

	try_set_opt( curl, CURLOPT_WRITEDATA, reuse );
	try_set_opt( curl, CURLOPT_PRIVATE, reuse );

#if PLATFORM_LINUX
	/*
	 * some ISP was doing broken caching and serving the wrong files over
	 * HTTP so we use HTTPS. we have our own integrity checks and setting
	 * up certs on Linux is really annoying so just don't bother
	 *
	 * we also use HTTPS for discord oauth but who cares about that
	 */
	try_set_opt( curl, CURLOPT_SSL_VERIFYPEER, 0l );
#endif

	curl_multi_add_handle( curl_multi, curl );
}

static bool parse_version( Version * v, const char * str ) {
	int fields = sscanf( str, "%u.%u.%u.%u", &v->a, &v->b, &v->c, &v->d );
	if( fields != 4 ) {
		v = { };
		return false;
	}
	return true;
}

template< typename T >
u32 clamp_u32( T x ) {
	if( x < 0 )
		return 0;
	if( x > U32_MAX )
		return U32_MAX;
	return u32( x );
}

static bool parse_manifest( std::unordered_map< std::string, ManifestEntry > & manifest, const char * data ) {
	for( array< array< const char > > line : gmatch( data, "([^\n]+)" ) ) {
		if( line.n != 1 )
			return false;

		Matches matches;
		bool ok = match( &matches, line[ 0 ], "^(%S+) (" BLAKE2B256_PATTERN ") (%d+)%s*(%w*)$" );
		if( !ok )
			return false;

		if( matches[ 1 ].n != BLAKE2B256_DIGEST_LENGTH * 2 )
			return false;

		const str< 256 > file_name( "{}", matches[ 0 ] );
		const str< 16 > file_size( "{}", matches[ 2 ] );
		const str< 32 > file_platform( "{}", matches[ 3 ] );
		u64 size = u64( strtonum( file_size.c_str(), 1, S64_MAX, NULL ) );

		ManifestEntry entry;
		bool ok_parse = parse_digest( &entry.checksum, matches[ 1 ] );

		if( matches[ 0 ].n > file_name.len() || matches[ 2 ].n > file_size.len() || matches[ 3 ].n > file_platform.len() || size == 0 || !ok_parse ) {
			manifest.clear();
			return false;
		}

		if( file_platform != "" && file_platform != PLATFORM_NAME ) {
			continue;
		}

		entry.file_size = size;
		entry.platform_specific = file_platform != "";

		manifest[ file_name.c_str() ] = entry;
	}

	return true;
}

static bool parse_signed_manifest( std::unordered_map< std::string, ManifestEntry > & manifest, const char * data ) {
	// check the manifest signature is ok
	Matches matches;
	bool ok = match( &matches, data, "(" SIGNATURE_PATTERN ")\n(.*)" );
	if( !ok )
		return false;

	u8 signature[ 64 ];
	bool ok_hex = parse_hex( matches[ 0 ], signature );
	ASSERT( ok_hex );

	int signature_ok = crypto_check( signature, PUBLIC_KEY, ( const u8 * ) matches[ 1 ].ptr(), matches[ 1 ].n );
	if( signature_ok != 0 )
		return false;

	return parse_manifest( manifest, matches[ 1 ].ptr() );
}

static bool make_relative_directories( std::vector< std::string > * dirs_created, const std::string & path ) {
	std::string cur;
	for( array< array< const char > > matches : gmatch( path.c_str(), "([^\\/]+)[\\/]" ) ) {
		cur.append( matches[ 0 ].ptr(), matches[ 0 ].n );
		cur += '/';
		if( !make_directory( cur.c_str() ) )
			return false;
		if( dirs_created != NULL ) {
			dirs_created->push_back( cur );
		}
	}

	return true;
}

static double last_failures[ 4 ];
static size_t last_failures_count = 0;

static int retry_delay( double now ) {
	last_failures[ last_failures_count % ARRAY_COUNT( last_failures ) ] = now;
	last_failures_count++;

	size_t recent_failures = 0;

	for( size_t i = 0; i < min( last_failures_count, ARRAY_COUNT( last_failures ) ); i++ ) {
		if( last_failures[ i ] >= now - 5 )
			recent_failures++;
	}

	return recent_failures == ARRAY_COUNT( last_failures ) ? 10 : 0;
}

void updater_init( bool autostart, LogCallback log_cb ) {
	autostart_update = autostart;
	log_callback = log_cb;

	std::string game_folder = get_executable_directory();
	if( !change_directory( game_folder.c_str() ) )
		FATAL( "change_directory" );

	curl_global_init( CURL_GLOBAL_DEFAULT );
	curl_multi = curl_multi_init();
	if( curl_multi == NULL )
		FATAL( "curl_multi_init" );

	if( curl_multi_setopt( curl_multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, 8l ) != CURLM_OK )
		FATAL( "curl_multi_setopt" );
}

void updater_term() {
	curl_multi_cleanup( curl_multi );
	curl_global_cleanup();
}

static UpdaterState updater_update( bool wait ) {
	while( true ) {
		int dont_care;
		if( wait ) {
			CURLMcode ok = curl_multi_wait( curl_multi, NULL, 0, -1, NULL );
			if( ok != CURLM_OK )
				FATAL( "curl_multi_wait: {}", curl_multi_strerror( ok ) );
		}
		CURLMcode r = curl_multi_perform( curl_multi, &dont_care );
		if( r == CURLM_OK )
			break;
		if( r != CURLM_CALL_MULTI_PERFORM )
			FATAL( "curl_multi_perform: {}", curl_multi_strerror( r ) );
	}

	double now = get_time();
	bool retry = now >= updater.retry_at;

	while( true ) {
		int dont_care;
		CURLMsg * msg = curl_multi_info_read( curl_multi, &dont_care );
		if( msg == NULL )
			break;

		ASSERT( msg->msg == CURLMSG_DONE );

		Download * dl;
		curl_easy_getinfo( msg->easy_handle, CURLINFO_PRIVATE, &dl );

		long http_status;
		curl_easy_getinfo( msg->easy_handle, CURLINFO_RESPONSE_CODE, &http_status ); 

		if( msg->data.result == CURLE_OK && http_status / 100 == 2 ) {
			dl->state = DownloadState_Done;
		}
		else {
			dl->state = DownloadState_Failed;
		}

		curl_multi_remove_handle( curl_multi, msg->easy_handle );
	}

	switch( updater.state ) {
		case UpdaterState_Init: {
			const char * local_version_str = file_get_contents_or_empty( "version.txt" );
			const char * local_manifest_str = file_get_contents_or_empty( "manifest.txt" );

			updater.retry_at = now;
			parse_version( &updater.local_version, local_version_str );
			parse_manifest( updater.local_manifest, local_manifest_str );

			updater.state = UpdaterState_DownloadingVersion_Retry;
		} break;

		case UpdaterState_DownloadingVersion:
			ASSERT( updater.downloads.size() == 1 );
			if( updater.downloads.front().state == DownloadState_Downloading )
				break;

			if( updater.downloads.front().state == DownloadState_Done ) {
				bool ok = parse_version( &updater.remote_version, updater.downloads.front().body.c_str() );
				if( ok ) {
					if( updater.remote_version == updater.local_version ) {
						updater.state = UpdaterState_ReadyToPlay;
					}
					else {
						updater.state = UpdaterState_DownloadingManifest_Retry;
						updater.retry_at = now;
					}
					updater.downloads.erase( updater.downloads.begin() );
					break;
				}
				else {
					log( "Couldn't parse version.txt, retrying in 5 seconds" );
				}
			}
			else {
				log( "Couldn't download version.txt, retrying in 5 seconds" );
			}

			updater.downloads.erase( updater.downloads.begin() );

			// TODO: backoff
			updater.state = UpdaterState_DownloadingVersion_Retry;
			updater.retry_at = now + 5;

			break;

		case UpdaterState_DownloadingVersion_Retry:
			if( retry ) {
				download( HOST "/version.txt" );
				updater.state = UpdaterState_DownloadingVersion;
			}
			break;

		case UpdaterState_DownloadingManifest:
			ASSERT( updater.downloads.size() == 1 );
			if( updater.downloads.front().state == DownloadState_Downloading )
				break;

			if( updater.downloads.front().state == DownloadState_Done ) {
				bool ok = parse_signed_manifest( updater.remote_manifest, updater.downloads.front().body.c_str() );
				if( ok ) {
					// look for files that have changed and files that should be removed
					for( const auto & kv : updater.local_manifest ) {
						const auto & it = updater.remote_manifest.find( kv.first );
						if( it == updater.remote_manifest.end() ) {
							updater.files_to_remove.push_back( kv.first );
						}
						else if( it->second.checksum != kv.second.checksum ) {
							updater.files_to_update.push_back( kv.first );
							updater.update_size += it->second.file_size;
						}
					}

					// look for new files
					for( const auto & kv : updater.remote_manifest ) {
						const auto & it = updater.local_manifest.find( kv.first );
						if( it == updater.local_manifest.end() ) {
							updater.files_to_update.push_back( kv.first );
							updater.update_size += kv.second.file_size;
						}
						updater.game_size += kv.second.file_size;
					}

					updater.state = autostart_update ? UpdaterState_StartUpdate : UpdaterState_NeedsUpdate;
					updater.downloads.erase( updater.downloads.begin() );
					break;
				}
				else {
					log( "Couldn't parse manifest, retrying in 5 seconds" );
				}
			}
			else {
				log( "Couldn't download manifest, retrying in 5 seconds" );
			}

			updater.downloads.erase( updater.downloads.begin() );

			// TODO: backoff
			updater.state = UpdaterState_DownloadingVersion_Retry;
			updater.retry_at = now + 5;

			break;

		case UpdaterState_DownloadingManifest_Retry:
			if( retry ) {
				str< 256 > url( HOST "/{}.txt", updater.remote_version );
				download( url.c_str() );
				updater.state = UpdaterState_DownloadingManifest;
			}
			break;

		case UpdaterState_NeedsUpdate:
			break;

		case UpdaterState_StartUpdate:
			updater.bytes_downloaded = 0;
			updater.downloads.reserve( updater.files_to_update.size() );
			for( size_t i = 0; i < updater.files_to_update.size(); i++ ) {
				str< 256 > url( HOST "/{}", updater.remote_manifest[ updater.files_to_update[ i ] ].checksum );
				download( url.c_str() );
			}

			updater.state = UpdaterState_DownloadingUpdate;
			break;

		case UpdaterState_DownloadingUpdate: {
			bool all_done = true;
			for( Download & dl : updater.downloads ) {
				if( dl.state != DownloadState_Done )
					all_done = false;

				if( dl.state == DownloadState_Failed ) {
					int delay = retry_delay( now );
					log( "Downloading {} failed, retrying in {} seconds", dl.url, delay );
					dl.state = DownloadState_Retry;
					dl.retry_at = now + delay;
				}

				if( dl.state == DownloadState_Retry && now >= dl.retry_at ) {
					dl.state = DownloadState_Downloading;
					download( dl.url.c_str(), &dl );
				}
			}

			update_smooth( &updater.download_speed, double( updater.bytes_downloaded - updater.last_bytes_downloaded ), now );
			updater.last_bytes_downloaded = updater.bytes_downloaded;

			if( all_done ) {
				updater.state = UpdaterState_InstallingUpdate;
			}
		} break;

		case UpdaterState_InstallingUpdate: {
			updater.state = UpdaterState_InstallingUpdateFailed;

			// TODO: make dirs/preallocate space before downloading
			// TODO: finish the error checking

			// verify checksums
			// TODO: should retry downloads a few times before failing
			// TODO: should save the downloads that did succeed too
			for( size_t i = 0; i < updater.downloads.size(); i++ ) {
				const std::string & file_name = updater.files_to_update[ i ];
				log( "Verifying {}", file_name );

				Blake2b256 checksum( updater.downloads[ i ].body.c_str(), updater.downloads[ i ].body.size() );

				if( checksum != updater.remote_manifest[ file_name ].checksum ) {
					log( "Incorrect checksum for {}", file_name );
					updater = Updater();
					updater.retry_at = now + 5;
					return updater.state;
				}
			}

			std::vector< std::string > update_dirs;

			// write the update to disk
			for( size_t i = 0; i < updater.downloads.size(); i++ ) {
				const std::string & file_name = updater.files_to_update[ i ];

				std::string download_path = "update/" + file_name;

				if( !make_relative_directories( &update_dirs, download_path.c_str() ) )
					return updater.state;

				FILE * f = fopen( download_path.c_str(), "wb" );
				if( f == NULL )
					return updater.state;

				// TODO: handle error
				fwrite( updater.downloads[ i ].body.c_str(), 1, updater.downloads[ i ].body.size(), f );

				if( updater.remote_manifest[ file_name ].platform_specific ) {
					mark_executable( f );
				}

				// TODO: handle error
				fclose( f );
			}

			int dirname_length;
			std::string launcher_path = get_executable_path( &dirname_length );
			std::string launcher_exe = launcher_path.substr( dirname_length + 1 );

			// move the update into place
			// TODO: reexec when launcher is updated
			for( const std::string & file_name : updater.files_to_update ) {
				if( !make_relative_directories( NULL, file_name.c_str() ) )
					return updater.state;

				// move exes out of the way before installing the new one
				if( file_name == launcher_exe ) {
					std::string old = file_name + ".old";
					if( !rename_replace( file_name.c_str(), old.c_str() ) ) {
						log_gle( "Couldn't move {} to {}", file_name, old );
						return updater.state;
					}
				}

				std::string download_path = "update/" + file_name;
				log( "Moving {} to {}", download_path, file_name );
				if( !rename_replace( download_path.c_str(), file_name.c_str() ) ) {
					log_gle( "Couldn't move {} to {}", download_path, file_name );
					return updater.state;
				}
			}

			// remove update directory tree
			std::reverse( update_dirs.begin(), update_dirs.end() );
			for( const std::string & dir : update_dirs ) {
				remove_directory( dir.c_str() );
			}

			// remove files that are no longer needed
			// TODO: remove dead dirs too
			for( const std::string & file_name : updater.files_to_remove ) {
				remove_file( file_name.c_str() );
			}

			// write new version.txt/manifest.txt
			log( "Writing version.txt" );

			FILE * version_txt = fopen( "version.txt", "w" );
			if( version_txt == NULL )
				break;

			// TODO: handle error
			str< 128 > version_str( "{}", updater.remote_version );
			fwrite( version_str.c_str(), 1, version_str.len(), version_txt );
			fclose( version_txt );

			log( "Writing manifest.txt" );

			FILE * manifest_txt = fopen( "manifest.txt", "w" );
			if( manifest_txt == NULL )
				break;

			// TODO: handle error
			for( const auto & kv : updater.remote_manifest ) {
				ggprint_to_file( manifest_txt, "{} {} {}\n", kv.first.c_str(), kv.second.checksum, kv.second.file_size );
			}
			fclose( manifest_txt );

#if PLATFORM_WINDOWS
			set_registry_key( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CocaineDiesel", "DisplayVersion", version_str.c_str() );
			set_registry_key( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CocaineDiesel", "EstimatedSize", clamp_u32( updater.game_size / 1024 ) );
#endif

			updater = Updater();
		} break;

		case UpdaterState_InstallingUpdateFailed:
			break;

		case UpdaterState_ReadyToPlay:
			break;
	}

	return updater.state;
}

UpdaterState updater_update() {
	return updater_update( false );
}

UpdaterState updater_wait() {
	return updater_update( true );
}

Version updater_local_version() {
	return updater.local_version;
}

Version updater_remote_version() {
	return updater.remote_version;
}

void updater_start_update() {
	updater.state = UpdaterState_StartUpdate;
}

u64 updater_update_size() {
	return updater.update_size;
}

u64 updater_bytes_downloaded() {
	return updater.bytes_downloaded;
}

float updater_download_progress() {
	return float( double( updater.bytes_downloaded ) / double( updater.update_size ) );
}

double updater_download_speed() {
	return updater.download_speed.latched;
}

