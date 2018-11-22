#include <stdlib.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "intrinsics.h"
#include "log.h"
#include "gl.h"
#include "ggformat.h"
#include "png.h"
#include "str.h"
#include "strlcpy.h"
#include "strtonum.h"
#include "patterns.h"
#include "platform.h"
#include "platform_exec.h"
#include "liberation.h"

#define GLFW_INCLUDE_NONE
#include "libs/glfw/include/GLFW/glfw3.h"

#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_glfw.h"
#include "libs/imgui/imgui_impl_opengl2.h"

#include "libs/monocypher/monocypher.h"

#include "libs/whereami/whereami.h"

#if PLATFORM_WINDOWS
#define CURL_STATICLIB
#include "libs/curl/curl.h"
#else
#include <curl/curl.h>
#endif

#if PLATFORM_WINDOWS
#define GAME_BINARY "client.exe"
#else
#define GAME_BINARY "client"
#endif

#define HEX256_PATTERN "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x"

#define BLAKE2B256_DIGEST_LENGTH 32
#define BLAKE2B256_PATTERN HEX256_PATTERN

#define SIGNATURE_PATTERN HEX256_PATTERN HEX256_PATTERN

static const u8 logo[] = {
#include "logo.h"
};

static const u8 PUBLIC_KEY[] = {
	0x3b, 0x0a, 0xc7, 0xa1, 0xd1, 0x9e, 0xbd, 0x0c,
	0x71, 0x75, 0x4f, 0xfc, 0x2a, 0xef, 0xcf, 0x91,
	0x93, 0xd0, 0x58, 0x94, 0x79, 0xc2, 0xeb, 0x16,
	0x30, 0x74, 0x62, 0x88, 0xe8, 0x18, 0x03, 0xb6,
};

static std::vector< std::string > log_lines;

template< typename... Rest >
static void LOG( const char * fmt, const Rest & ... rest ) {
	log_lines.push_back( std::string() );

	size_t space_required = ggformat( nullptr, 0, fmt, rest... );

	std::string & result = log_lines.back();
	result.resize( space_required + 1 ); // + 1 so there's space for the null terminator...
	ggformat( &result[ 0 ], space_required + 1, fmt, rest... );
	result.resize( space_required ); // ...and then trim it off
}

// TODO: errno/GetLastError
#define LOGERROR LOG

struct Blake2b256 {
	StaticArray< u8, BLAKE2B256_DIGEST_LENGTH > digest;

	Blake2b256() { }

	explicit Blake2b256( const void * data, size_t len ) {
		crypto_blake2b_general( digest.ptr(), digest.num_bytes(), NULL, 0, ( const u8 * ) data, len );
	}
};

static bool operator==( const Blake2b256 & a, const Blake2b256 & b ) {
	return memcmp( a.digest.ptr(), b.digest.ptr(), a.digest.num_bytes() ) == 0;
}

static bool operator!=( const Blake2b256 & a, const Blake2b256 & b ) {
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

struct ManifestEntry {
	Blake2b256 checksum;
	u64 file_size;
	bool platform_specific;
};

struct Version {
	u32 a, b, c, d;
};

static bool operator==( const Version & a, const Version & b ) {
	return a.a == b.a && a.b == b.b && a.c == b.c && a.d == b.d;
}

static bool operator!=( const Version & a, const Version & b ) {
	return !( a == b );
}

static bool parse_version( Version * v, const char * str ) {
	int fields = sscanf( str, "%u.%u.%u.%u", &v->a, &v->b, &v->c, &v->d );
	if( fields != 4 ) {
		v = { };
		return false;
	}
	return true;
}

static void format( FormatBuffer * fb, const Version & v, const FormatOpts & opts ) {
	str< 128 > s( "{}.{}.{}.{}", v.a, v.b, v.c, v.d );
	format( fb, s, opts );
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
	u64 game_size = 0;
};

static Updater updater;
static bool autostart_update = false;

// #define HOST "cocainediesel.mikejsavage.co.uk"
#define HOST "127.0.0.1:8000"

#if PLATFORM_WINDOWS
static void set_registry_key( HKEY hkey, const char * path, const char * value, const char * data ) {
	HKEY key;
	LONG ok_open = RegOpenKeyExA( hkey, path, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &key );
	if( ok_open != ERROR_SUCCESS ) {
		WARN( "couldn't open registry key {} {} ({})", path, value, ok_open );
		return;
	}
	SCOPE_EXIT( RegCloseKey( key ) );

	LONG ok_set = RegSetValueExA( key, value, 0, REG_SZ, ( const BYTE * ) data, checked_cast< DWORD >( strlen( data ) + 1 ) );
	if( ok_set != ERROR_SUCCESS ) {
		WARN( "couldn't write registry key ({})", ok_set );
	}
}

static void set_registry_key( HKEY hkey, const char * path, const char * value, u32 data ) {
	HKEY key;
	LONG ok_open = RegOpenKeyExA( hkey, path, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &key );
	if( ok_open != ERROR_SUCCESS ) {
		WARN( "couldn't open registry key {} {} ({})", path, value, ok_open );
		return;
	}
	SCOPE_EXIT( RegCloseKey( key ) );

	LONG ok_set = RegSetValueExA( key, value, 0, REG_DWORD, ( const BYTE * ) &data, sizeof( data ) );
	if( ok_set != ERROR_SUCCESS ) {
		WARN( "couldn't write registry key ({})", ok_set );
	}
}
#endif

CURLM * curl_multi;

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
	LOG( "Downloading {}", url );

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

	curl_multi_add_handle( curl_multi, curl );
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
	// if( signature_ok != 0 )
	// 	return false;

	return parse_manifest( manifest, matches[ 1 ].ptr() );
}

#if PLATFORM_WINDOWS

#include <windows.h>

static bool rename_replace( const char * old_path, const char * new_path ) {
	if( MoveFileExA( old_path, new_path, MOVEFILE_REPLACE_EXISTING ) != 0 )
		return true;
	// TODO log
	return false;
}

static bool change_directory( const char * path ) {
	return SetCurrentDirectory( path ) != 0;
}

static bool make_directory( const char * path ) {
	bool ok = CreateDirectoryA( path, NULL ) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
	if( !ok )
		LOGERROR( "Couldn't create directory {}", path );
	return ok;
}

static bool remove_file( const char * path ) {
	return DeleteFileA( path ) != 0;
}

static bool remove_directory( const char * path ) {
	return RemoveDirectoryA( path ) != 0;
}

#elif PLATFORM_UNIX

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

static bool rename_replace( const char * old_path, const char * new_path ) {
	return rename( old_path, new_path ) == 0;
}

static bool change_directory( const char * path ) {
	return chdir( path ) == 0;
}

static bool make_directory( const char * path ) {
	bool ok = mkdir( path, 0755 ) == 0 || errno == EEXIST;
	if( !ok )
		LOGERROR( "Couldn't create directory {}", path );
	return ok;
}

static bool remove_file( const char * path ) {
	return unlink( path ) == 0;
}

static bool remove_directory( const char * path ) {
	return rmdir( path ) == 0;
}

#else
#error new platform
#endif

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

static void step_updater( double now ) {
	bool retry = now >= updater.retry_at;

	int remaining_transfers;
	while( true ) {
		CURLMcode r = curl_multi_perform( curl_multi, &remaining_transfers );
		if( r == CURLM_OK )
			break;
		if( r != CURLM_CALL_MULTI_PERFORM )
			FATAL( "curl_multi_perform" );
	}

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

		if( msg->data.result == CURLE_OK && http_status == 200 ) {
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
					LOG( "Couldn't parse version.txt, retrying in 5 seconds" );
				}
			}
			else {
				LOG( "Couldn't download version.txt, retrying in 5 seconds" );
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
					LOG( "Couldn't parse manifest, retrying in 5 seconds" );
				}
			}
			else {
				LOG( "Couldn't download manifest, retrying in 5 seconds" );
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
					LOG( "Downloading {} failed, retrying in {} seconds", dl.url, delay );
					dl.state = DownloadState_Retry;
					dl.retry_at = now + delay;
				}

				if( dl.state == DownloadState_Retry && now >= dl.retry_at ) {
					dl.state = DownloadState_Downloading;
					download( dl.url.c_str(), &dl );
				}
			}

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
				LOG( "Verifying {}", file_name );

				Blake2b256 checksum( updater.downloads[ i ].body.c_str(), updater.downloads[ i ].body.size() );

				if( checksum != updater.remote_manifest[ file_name ].checksum ) {
					LOG( "Incorrect checksum for {}", file_name );
					updater = Updater();
					updater.retry_at = now + 5;
					return;
				}
			}

			std::vector< std::string > update_dirs;

			// write the update to disk
			for( size_t i = 0; i < updater.downloads.size(); i++ ) {
				const std::string & file_name = updater.files_to_update[ i ];

				std::string download_path = "update/" + file_name;

				if( !make_relative_directories( &update_dirs, download_path.c_str() ) )
					return;

				FILE * f = fopen( download_path.c_str(), "wb" );
				if( f == NULL )
					return;

				// TODO: handle error
				fwrite( updater.downloads[ i ].body.c_str(), 1, updater.downloads[ i ].body.size(), f );

#if PLATFORM_UNIX
				if( updater.remote_manifest[ file_name ].platform_specific ) {
					// mark executable
					// TODO: handle error
					int fd = fileno( f );
					fchmod( fd, 0755 );
				}
#endif

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
					return;

				// move exes out of the way before installing the new one
				if( file_name == launcher_exe ) {
					std::string old = file_name + ".old";
					if( !rename_replace( file_name.c_str(), old.c_str() ) )
						return;
				}

				std::string download_path = "update/" + file_name;
				LOG( "Moving {} to {}", download_path, file_name );
				if( !rename_replace( download_path.c_str(), file_name.c_str() ) )
					return;
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
			LOG( "Writing version.txt" );

			FILE * version_txt = fopen( "version.txt", "w" );
			if( version_txt == NULL )
				return;

			// TODO: handle error
			str< 128 > version_str( "{}", updater.remote_version );
			fwrite( version_str.c_str(), 1, version_str.len(), version_txt );
			fclose( version_txt );

			LOG( "Writing manifest.txt" );

			FILE * manifest_txt = fopen( "manifest.txt", "w" );
			if( manifest_txt == NULL )
				return;

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
}

static void launcher_main() {
	std::string game_folder = get_executable_directory();
	if( !change_directory( game_folder.c_str() ) )
		FATAL( "change_directory" );

	curl_global_init( CURL_GLOBAL_ALL );
	curl_multi = curl_multi_init();
	if( curl_multi == NULL )
		FATAL( "curl_multi_init" );

	if( curl_multi_setopt( curl_multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, 8l ) != CURLM_OK )
		FATAL( "curl_multi_setopt" );

	GLFWwindow * window = gl_init();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	glfwSwapInterval( 2 );

	u32 tex = load_png_memory( logo, sizeof( logo ) );

	ImFont * small;
	ImFont * large;

	{
		u8 * data = LiberationSans_Regular_ttf;
		int size = LiberationSans_Regular_ttf_len;
		ImGuiIO & io = ImGui::GetIO();
		ImFontConfig config;
		config.FontDataOwnedByAtlas = false;
		io.IniFilename = NULL;
		io.Fonts->AddFontFromMemoryTTF( data, size, 16.0f, &config );
		small = io.Fonts->AddFontFromMemoryTTF( data, size, 12.0f, &config );
		large = io.Fonts->AddFontFromMemoryTTF( data, size, 32.0f, &config );
	}

	{
		ImGuiStyle & style = ImGui::GetStyle();
		style.WindowRounding = 0;
		style.FramePadding = ImVec2( 8, 8 );
		style.FrameBorderSize = 0;
		style.WindowBorderSize = 0;
		style.Colors[ ImGuiCol_WindowBg ] = ImColor( 0x1a, 0x1a, 0x1a );
		style.ItemSpacing.y = 8;
	}

	double last_frame_time = glfwGetTime();
	while( !glfwWindowShouldClose( window ) ) {
		if( updater.state == UpdaterState_NeedsUpdate || updater.state == UpdaterState_ReadyToPlay )
			glfwWaitEvents();
		else
			glfwPollEvents();

		bool enter_key_pressed = glfwGetKey( window, GLFW_KEY_ENTER ) == GLFW_PRESS || glfwGetKey( window, GLFW_KEY_SPACE ) == GLFW_PRESS;

		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		double now = glfwGetTime();
		double dt = now - last_frame_time;

		step_updater( now );

		float logo_height = 256;

		v2u32 window_size = get_window_size();
		ImGui::GetStyle().WindowPadding = ImVec2( 0, 0 );
		ImGui::SetNextWindowPos( ImVec2() );
		ImGui::SetNextWindowSize( ImVec2( window_size.x, logo_height ) );
		ImGui::Begin( "logo", NULL, 0
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoScrollWithMouse
		);

		ImGui::Image( ( void * ) checked_cast< uptr >( tex ), ImVec2( 750, logo_height ) );

		ImGui::End();

		ImGui::GetStyle().WindowPadding = ImVec2( 32, 16 );
		ImGui::SetNextWindowPos( ImVec2( 0, logo_height ) );
		ImGui::SetNextWindowSize( ImVec2( window_size.x, window_size.y - logo_height ) );
		ImGui::Begin( "controls", NULL, 0
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoScrollWithMouse
		);

		ImGui::Text( "v%hhu.%hhu.%hhu.%hhu", updater.local_version.a, updater.local_version.b, updater.local_version.c, updater.local_version.d );

		ImGui::PushFont( large );
		if( updater.state == UpdaterState_ReadyToPlay ) {
			ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 0x29, 0x8a, 0x67, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0x30, 0xa1, 0x78, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0x38, 0xb9, 0x8a, 0xff ) );

			bool launch = ImGui::Button( "Play", ImVec2( -1, 50 ) );
			ImGui::PopStyleColor( 3 );

			if( launch || enter_key_pressed ) {
				exec_and_quit( GAME_BINARY );
			}
		}
		else if( updater.state == UpdaterState_NeedsUpdate ) {
			ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 0x29, 0x8a, 0x67, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0x30, 0xa1, 0x78, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0x38, 0xb9, 0x8a, 0xff ) );

			str< 256 > button_text( "Update to v{} - {.2}MB", updater.remote_version, updater.update_size / 1000.0 / 1000.0 );
			bool update = ImGui::Button( button_text.c_str(), ImVec2( -1, 50 ) );
			ImGui::PopStyleColor( 3 );

			if( update || enter_key_pressed ) {
#if PLATFORM_WINDOWS
				exec_and_quit( "elevate_for_update.exe" );
#else
				updater.state = UpdaterState_StartUpdate;
#endif
			}
		}
		else if( updater.state == UpdaterState_DownloadingUpdate ) {
			static u64 last_bytes_downloaded = 0;
			float frac = float( double( updater.bytes_downloaded ) / double( updater.update_size ) );

			static double smooth_download_speed;
			static double latched_download_speed;
			static double latched_set_time;
			static bool download_speed_set = false;

			if( download_speed_set ) {
				double speed = ( updater.bytes_downloaded - last_bytes_downloaded ) / dt;
				smooth_download_speed += ( speed - smooth_download_speed ) / ( 0.5 / dt );

				if( now - latched_set_time > 0.5 ) {
					latched_download_speed = smooth_download_speed;
					latched_set_time = now;
				}
			}
			else {
				smooth_download_speed = updater.bytes_downloaded * 60.0;
				latched_download_speed = smooth_download_speed;
				latched_set_time = now;
				download_speed_set = true;
			}

			last_bytes_downloaded = updater.bytes_downloaded;

			ImGui::PushStyleColor( ImGuiCol_PlotHistogram, IM_COL32( 0x29, 0x8a, 0x67, 0xff ) );

			const char * unit = "KB/s";
			double display_speed = latched_download_speed / 1000.0;
			if( display_speed > 1000.0 ) {
				unit = "MB/s";
				display_speed /= 1000.0;
			}

			str< 256 > progress_text( "{.2}/{.2}MB. {.2}% {.2}{}",
				updater.bytes_downloaded / 1000.0 / 1000.0,
				updater.update_size / 1000.0 / 1000.0,
				frac * 100,
				display_speed,
				unit
				);
			ImGui::ProgressBar( frac, ImVec2( -1, 50 ), progress_text.c_str() );
			ImGui::PopStyleColor();
		}
		else if( updater.state == UpdaterState_InstallingUpdate ) {
			ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::Button( "Installing update...", ImVec2( -1, 50 ) );
			ImGui::PopStyleColor( 3 );
		}
		else {
			ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::Button( "Checking for updates...", ImVec2( -1, 50 ) );
			ImGui::PopStyleColor( 3 );
		}

		ImGui::PopFont();

		ImGui::BeginChildFrame( 1337, ImVec2(), 0
			| ImGuiWindowFlags_AlwaysVerticalScrollbar
		);
		ImGui::PushFont( small );
		for( const std::string & line : log_lines ) {
			ImGui::TextWrapped( "%s", line.c_str() );
		}
		ImGui::PopFont();
		ImGui::EndChildFrame();

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData( ImGui::GetDrawData() );

		glfwSwapBuffers( window );

		last_frame_time = now;
	}

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	gl_term();

	curl_multi_cleanup( curl_multi );
	curl_global_cleanup();
}

#if PLATFORM_WINDOWS

int WinMain( HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nShowCmd ) {
	if( strcmp( lpszCmdLine, "--start-update" ) == 0 ) {
		autostart_update = true;
	}

	launcher_main();

	return 0;
}

#else

int main( int argc, char ** argv ) {
	if( argc == 2 && strcmp( argv[ 1 ], "--start-update" ) == 0 ) {
		autostart_update = true;
	}

	launcher_main();

	return 0;
}

#endif
