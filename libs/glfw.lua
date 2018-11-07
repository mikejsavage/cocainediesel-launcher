lib( "glfw", {
	"libs/glfw/src/context", "libs/glfw/src/init", "libs/glfw/src/input",
	"libs/glfw/src/monitor", "libs/glfw/src/vulkan", "libs/glfw/src/window",
} )

if OS == "windows" then
	lib( "glfw", {
		"libs/glfw/src/win32_init", "libs/glfw/src/win32_monitor", "libs/glfw/src/win32_window",
		"libs/glfw/src/win32_joystick", "libs/glfw/src/win32_time", "libs/glfw/src/win32_thread",
		"libs/glfw/src/wgl_context", "libs/glfw/src/egl_context", "libs/glfw/src/osmesa_context",
	} )

	obj_cxxflags( "libs/glfw/src/%", "/c /TC /D_GLFW_WIN32" ) -- compile as c
	obj_cxxflags( "libs/glfw/src/%", "/wd4152 /wd4204 /wd4244 /wd4456" )
elseif OS == "linux" then
	lib( "glfw", {
		"libs/glfw/src/x11_init", "libs/glfw/src/x11_monitor", "libs/glfw/src/x11_window",
		"libs/glfw/src/xkb_unicode", "libs/glfw/src/linux_joystick", "libs/glfw/src/posix_time",
		"libs/glfw/src/posix_thread", "libs/glfw/src/glx_context", "libs/glfw/src/egl_context",
		"libs/glfw/src/osmesa_context",
	} )

	obj_replace_cxxflags( "libs/glfw/src/%", "-c -x c -O2 -D_GLFW_X11" )
elseif OS == "macos" then
	lib( "glfw", {
		"libs/glfw/src/cocoa_init", "libs/glfw/src/cocoa_monitor", "libs/glfw/src/cocoa_window",
		"libs/glfw/src/cocoa_joystick", "libs/glfw/src/cocoa_time", "libs/glfw/src/posix_thread",
		"libs/glfw/src/egl_context", "libs/glfw/src/nsgl_context", "libs/glfw/src/osmesa_context",
	} )

	obj_replace_cxxflags( "libs/glfw/src/%", "-c -x c -O2 -D_GLFW_COCOA -mmacosx-version-min=10.9" )

	-- build .m files properly
	obj( "libs/glfw/src/cocoa_init", "libs/glfw/src/cocoa_init.m" )
	obj( "libs/glfw/src/cocoa_monitor", "libs/glfw/src/cocoa_monitor.m" )
	obj( "libs/glfw/src/cocoa_window", "libs/glfw/src/cocoa_window.m" )
	obj( "libs/glfw/src/cocoa_joystick", "libs/glfw/src/cocoa_joystick.m" )
	obj( "libs/glfw/src/nsgl_context", "libs/glfw/src/nsgl_context.m" )

	obj_replace_cxxflags( "libs/glfw/src/cocoa_init", "-c -O2 -D_GLFW_COCOA -mmacosx-version-min=10.9" )
	obj_replace_cxxflags( "libs/glfw/src/cocoa_monitor", "-c -O2 -D_GLFW_COCOA -Wno-deprecated-declarations -mmacosx-version-min=10.9" )
	obj_replace_cxxflags( "libs/glfw/src/cocoa_window", "-c -O2 -D_GLFW_COCOA -Wno-deprecated-declarations -mmacosx-version-min=10.9" )
	obj_replace_cxxflags( "libs/glfw/src/cocoa_joystick", "-c -O2 -D_GLFW_COCOA -mmacosx-version-min=10.9" )
	obj_replace_cxxflags( "libs/glfw/src/nsgl_context", "-c -O2 -D_GLFW_COCOA -Wno-deprecated-declarations -mmacosx-version-min=10.9" )
else
	error( "don't know how to build GLFW on this platform" )
end
