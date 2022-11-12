#include <windows.h>

#include "intrinsics.h"
#include "log.h"
#include "win32_fs.h"
#include "win32_exec.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, char * szCmdLine, int iCmdShow ) {
	exec_and_quit( "cocainediesel.exe", RunAsAdmin_Yes, "--start-update" );
	return 1;
}
