#pragma once

#include <windows.h>

inline bool rename_replace( const char * old_path, const char * new_path ) {
	return MoveFileExA( old_path, new_path, MOVEFILE_REPLACE_EXISTING ) != 0;
}

inline bool change_directory( const char * path ) {
	return SetCurrentDirectory( path ) != 0;
}

inline bool make_directory( const char * path ) {
	return CreateDirectoryA( path, NULL ) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

inline bool remove_file( const char * path ) {
	return DeleteFileA( path ) != 0;
}

inline bool remove_directory( const char * path ) {
	return RemoveDirectoryA( path ) != 0;
}
