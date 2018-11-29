#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include "intrinsics.h"
#include "array.h"
#include "str.h"
#include "strlcpy.h"
#include "patterns.h"
#include "ggformat.h"
#include "platform_exec.h"
#include "discord.h"

#include "libs/curl/curl.h"
#include "libs/picohttpparser/picohttpparser.h"

static int listen_fd;
static int req_fd;

static DiscordState state;

static CURLM * curl_multi;

static char req_buffer[ 2048 ];
static size_t req_size;

static char res_buffer[ 4096 ];
static size_t res_size;

static str< 128 > token;

static str< 256 > username;
static str< 128 > discriminator;
static str< 128 > user_id;
static str< 128 > avatar;

void discord_init() {
	state = DiscordState_Error;
	req_fd = -1;

	curl_multi = curl_multi_init();
	if( curl_multi == NULL )
		return;

	listen_fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( listen_fd == -1 )
		return;

	int one = 1;
	setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof( one ) );

	struct sockaddr_in server_addr = { };
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	server_addr.sin_port = htons( 13337 );
	int ok_bind = bind( listen_fd, ( struct sockaddr * )&server_addr, sizeof( server_addr ) );
	if( ok_bind == -1 ) {
		close( listen_fd );
		listen_fd = -1;
		return;
	}

	int ok_listen = listen( listen_fd, 1 );
	if( ok_listen == -1 ) {
		close( listen_fd );
		listen_fd = -1;
		return;
	}

	state = DiscordState_Unauthenticated;
}

void discord_shutdown() {
	if( listen_fd != -1 )
		close( listen_fd );

	curl_multi_cleanup( curl_multi );
}

DiscordState discord_state() {
	return state;
}

template< typename F >
void try_set_opt( CURL * curl, CURLoption option, F f ) {
	STATIC_ASSERT( !SAME_TYPE( F, int ) || SAME_TYPE( int, long ) );

	CURLcode r = curl_easy_setopt( curl, option, f );
	if( r != CURLE_OK )
		FATAL( "curl_easy_setopt: {}", curl_easy_strerror( r ) );
}

static size_t discord_data_received( char * data, size_t size, size_t nmemb, void * user_data ) {
	size_t to_copy = min( size * nmemb, sizeof( res_buffer ) - res_size );
	memcpy( res_buffer, data, to_copy );
	res_size += to_copy;
	return to_copy;
}

template< size_t N >
static bool parse_json_string( str< N > * s, array< const char > json, const char * key ) {
	Matches matches;
	str< 256 > pattern( "\"{}\"%s*:%s*\"([^\"]+)\"", key );
	bool ok = match( &matches, array< const char >( res_buffer, res_size ), pattern.c_str() );
	if( ok )
		( *s ) += matches[ 0 ];
	return ok;
}

static CURL * make_curl( const char * url ) {
	CURL * curl = curl_easy_init();
	if( curl == NULL )
		FATAL( "curl_easy_init" );

	try_set_opt( curl, CURLOPT_URL, url );
	try_set_opt( curl, CURLOPT_WRITEFUNCTION, discord_data_received );
	try_set_opt( curl, CURLOPT_FOLLOWLOCATION, 1l );
	try_set_opt( curl, CURLOPT_NOSIGNAL, 1l );
	try_set_opt( curl, CURLOPT_CONNECTTIMEOUT, 10l );
	try_set_opt( curl, CURLOPT_LOW_SPEED_TIME, 10l );
	try_set_opt( curl, CURLOPT_LOW_SPEED_LIMIT, 10l );

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

	return curl;
}

DiscordState discord_update() {
	if( state == DiscordState_Authenticating ) {
		if( req_fd == -1 ) {
			pollfd p;
			p.events = POLLIN;
			p.fd = listen_fd;

			int r = poll( &p, 1, 0 );
			if( r > 0 ) {
				req_fd = accept( listen_fd, NULL, NULL );
				req_size = 0;
			}
		}

		if( req_fd != -1 ) {
			pollfd p;
			p.events = POLLIN;
			p.fd = req_fd;

			int r = poll( &p, 1, 0 );
			if( r > 0 ) {
				ssize_t b = read( req_fd, req_buffer + req_size, sizeof( req_buffer ) - req_size );

				if( b == 0 ) {
					close( req_fd );
					req_fd = -1;
				}
				else {
					const char * method;
					size_t method_len;

					const char * path;
					size_t path_len;

					int minor_version;

					struct phr_header headers[ 16 ];
					size_t num_headers = ARRAY_COUNT( headers );

					ssize_t ok = phr_parse_request( req_buffer, req_size + b, &method, &method_len, &path, &path_len, &minor_version, headers, &num_headers, req_size );
					req_size += b;

					if( ok >= 0 ) {
						const char * body = "<html><body><h3>Cocaine Diesel auth page - you can close this tab</h3></body></html>";
						str< 512 > response( "HTTP/1.1 200 OK\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", strlen( body ), body );
						write( req_fd, response.c_str(), response.len() );
						close( req_fd );
						req_fd = -1;

						res_size = 0;

						size_t prefix_len = strlen( "/?code=" );
						if( path_len >= prefix_len ) {
							array< const char > code = array< const char >( path + prefix_len, path_len - prefix_len );
							str< 512 > post( "client_id={}&client_secret={}&grant_type=authorization_code&redirect_uri={}&scope=identity&code={}",
								"517407996968697858",
								"KcApoQDmcTGFHc5GUy7VzejGhDQGMZ2e",
								"http://localhost:13337",
								code );

							CURL * curl = make_curl( "https://discordapp.com/api/oauth2/token" );
							try_set_opt( curl, CURLOPT_COPYPOSTFIELDS, post.c_str() );
							curl_multi_add_handle( curl_multi, curl );
						}
						else {
							state = DiscordState_Error;
						}
					}
					else if( ok == -1 ) {
						state = DiscordState_Error;
					}
				}
			}
		}
	}
	
	while( true ) {
		int dont_care;
		CURLMcode r = curl_multi_perform( curl_multi, &dont_care );
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

		long http_status;
		curl_easy_getinfo( msg->easy_handle, CURLINFO_RESPONSE_CODE, &http_status ); 

		array< const char > response( res_buffer, res_size );

		if( msg->data.result == CURLE_OK && http_status / 100 == 2 ) {
			if( token.len() == 0 ) {
				if( parse_json_string( &token, response, "access_token" ) ) {
					res_size = 0;
					CURL * curl = make_curl( "https://discordapp.com/api/users/@me" );

					str< 256 > oauth_header( "Authorization: Bearer {}", token );
					struct curl_slist * headers = NULL;
					headers = curl_slist_append( headers, oauth_header.c_str() );
					try_set_opt( curl, CURLOPT_HTTPHEADER, headers );

					curl_multi_add_handle( curl_multi, curl );
				}
				else {
					state = DiscordState_Error;
				}
			}
			else {
				bool ok = true;
				ok = ok && parse_json_string( &username, response, "username" );
				ok = ok && parse_json_string( &discriminator, response, "discriminator" );
				ok = ok && parse_json_string( &user_id, response, "id" );
				ok = ok && parse_json_string( &avatar, response, "avatar" );

				state = ok ? DiscordState_Authenticated : DiscordState_Error;
			}
		}
		else {
			state = DiscordState_Error;
		}

		curl_multi_remove_handle( curl_multi, msg->easy_handle );
	}
	
	return state;
}

void discord_login() {
	ASSERT( state == DiscordState_Unauthenticated );

	token.clear();
	username.clear();
	discriminator.clear();
	user_id.clear();
	avatar.clear();

	bool ok = open_in_browser( "https://discordapp.com/api/oauth2/authorize?client_id=517407996968697858&redirect_uri=http%3A%2F%2Flocalhost%3A13337&response_type=code&scope=identify" );
	state = ok ? DiscordState_Authenticating : DiscordState_Error;
}

DiscordUser discord_user() {
	DiscordUser user;
	user.username = username.c_str();
	user.discriminator = discriminator.c_str();
	user.id = user_id.c_str();
	user.avatar = avatar.c_str();
	return user;
}
