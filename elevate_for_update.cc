#include <windows.h>

#include "win32_exec.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, char * szCmdLine, int iCmdShow ) {
	exec_and_quit( "launch.exe", "--start-update" );
	return 1;
}
