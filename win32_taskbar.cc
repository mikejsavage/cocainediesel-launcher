#include "intrinsics.h"

#include <windows.h>
#include <shobjidl.h>

#define GLFW_INCLUDE_NONE
#include "libs/glfw/include/GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "libs/glfw/include/GLFW/glfw3native.h"

static ITaskbarList3 * taskbar;

void taskbar_init() {
	CoInitialize( NULL );
	CoCreateInstance( CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, ( void ** ) &taskbar );
}

void taskbar_term() {
        taskbar->Release();
        CoUninitialize();
}

void taskbar_progress( GLFWwindow * window, u64 completed, u64 total ) {
	HWND hwnd = glfwGetWin32Window( window );
        taskbar->SetProgressState( hwnd, TBPF_NORMAL );
        taskbar->SetProgressValue( hwnd, completed, total );
}

void taskbar_clear( GLFWwindow * window ) {
	HWND hwnd = glfwGetWin32Window( window );
        taskbar->SetProgressState( hwnd, TBPF_NOPROGRESS );
}
