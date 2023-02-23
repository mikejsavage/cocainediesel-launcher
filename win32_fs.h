#pragma once

#include <windows.h>

inline wchar_t * UTF8ToWide( const char * utf8 ) {
	int len = MultiByteToWideChar( CP_UTF8, 0, utf8, -1, NULL, 0 );
	ASSERT( len != 0 );

	wchar_t * wide = alloc_many< wchar_t >( len );
	MultiByteToWideChar( CP_UTF8, 0, utf8, -1, wide, len );

	return wide;
}

inline char * WideToUTF8( const wchar_t * wide ) {
	int len = WideCharToMultiByte( CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL );
	ASSERT( len != 0 );

	char * utf8 = alloc_many< char >( len );
	WideCharToMultiByte( CP_UTF8, 0, wide, -1, utf8, len, NULL, NULL );

	return utf8;
}

inline FILE * open_file( const char * path, const char * mode ) {
	wchar_t * wide_path = UTF8ToWide( path );
	wchar_t * wide_mode = UTF8ToWide( mode );
	defer { free( wide_path ); };
	defer { free( wide_mode ); };
	return _wfopen( wide_path, wide_mode );
}

inline bool rename_replace( const char * old_path, const char * new_path ) {
	wchar_t * wide_old_path = UTF8ToWide( old_path );
	wchar_t * wide_new_path = UTF8ToWide( new_path );
	defer { free( wide_old_path ); };
	defer { free( wide_new_path ); };
	return MoveFileExW( wide_old_path, wide_new_path, MOVEFILE_REPLACE_EXISTING ) != 0;
}

inline bool change_directory( const char * path ) {
	wchar_t * wide_path = UTF8ToWide( path );
	defer { free( wide_path ); };
	return SetCurrentDirectoryW( wide_path ) != 0;
}

inline bool make_directory( const char * path ) {
	wchar_t * wide_path = UTF8ToWide( path );
	defer { free( wide_path ); };
	return CreateDirectoryW( wide_path, NULL ) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

inline bool remove_file( const char * path ) {
	wchar_t * wide_path = UTF8ToWide( path );
	defer { free( wide_path ); };
	return DeleteFileW( wide_path ) != 0;
}

inline bool remove_directory( const char * path ) {
	wchar_t * wide_path = UTF8ToWide( path );
	defer { free( wide_path ); };
	return RemoveDirectoryW( wide_path ) != 0;
}

inline bool mark_executable( FILE * f ) {
	return true;
}
