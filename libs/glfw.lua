local common_srcs = {
	"libs/glfw/src/context.cc", "libs/glfw/src/init.cc", "libs/glfw/src/input.cc",
	"libs/glfw/src/monitor.cc", "libs/glfw/src/vulkan.cc", "libs/glfw/src/window.cc",
}

if OS == "windows" then
	lib( "glfw", {
		common_srcs,
		"libs/glfw/src/win32_init.cc", "libs/glfw/src/win32_monitor.cc", "libs/glfw/src/win32_window.cc",
		"libs/glfw/src/win32_joystick.cc", "libs/glfw/src/win32_time.cc", "libs/glfw/src/win32_thread.cc",
		"libs/glfw/src/wgl_context.cc", "libs/glfw/src/egl_context.cc", "libs/glfw/src/osmesa_context.cc",
	} )

	obj_cxxflags( "libs/glfw/src/.+", "/c /TC /D_GLFW_WIN32" ) -- compile as c
	obj_cxxflags( "libs/glfw/src/.+", "/wd4152 /wd4204 /wd4244 /wd4456" )
elseif OS == "linux" then
	lib( "glfw", {
		common_srcs,
		"libs/glfw/src/x11_init.cc", "libs/glfw/src/x11_monitor.cc", "libs/glfw/src/x11_window.cc",
		"libs/glfw/src/xkb_unicode.cc", "libs/glfw/src/linux_joystick.cc", "libs/glfw/src/posix_time.cc",
		"libs/glfw/src/posix_thread.cc", "libs/glfw/src/glx_context.cc", "libs/glfw/src/egl_context.cc",
		"libs/glfw/src/osmesa_context.cc",
	} )

	obj_replace_cxxflags( "libs/glfw/src/.+", "-c -x c -O2 -D_GLFW_X11" )
else
	error( "don't know how to build GLFW on this platform" )
end
