require( "scripts.gen_ninja" )

require( "libs.curl" )
require( "libs.imgui" )
require( "libs.glfw3" )
require( "libs.mbedtls" )
require( "libs.monocypher" )
require( "libs.stb" )
require( "libs.whereami" )

local platform_curl_libs = { }
if OS ~= "macos" then
	table.insert( platform_curl_libs, "curl" )
end
if OS == "linux" then
	table.insert( platform_curl_libs, "mbedtls" )
end

bin( "cocainediesel", {
	srcs = {
		"main.cc", "updater.cc", "icon.cc", "ggformat.cc", "strlcpy.cc",
		"patterns.cc", "platform_taskbar.cc", "gl.cc", "glad.cc", "png.cc"
	},

	libs = { "glfw3", "imgui", "monocypher", "stb_image", "whereami", platform_curl_libs },

	rc = "cocainediesel_manifest",

	windows_ldflags = "opengl32.lib gdi32.lib ole32.lib Ws2_32.lib crypt32.lib",
	macos_ldflags = "-lcurl -framework Cocoa -framework CoreVideo -framework IOKit",
	linux_ldflags = "-lm -lpthread -ldl",
} )

bin( "headlessupdater", {
	srcs = { "headless.cc", "updater.cc", "ggformat.cc", "strlcpy.cc", "patterns.cc" },
	libs = { "monocypher", "whereami", platform_curl_libs },

	windows_ldflags = "Ws2_32.lib crypt32.lib",
	macos_ldflags = "-lcurl",
	linux_ldflags = "-lm -lpthread",
} )

msvc_obj_cxxflags( "updater%.cc", "/Ilibs/mbedtls" )

if OS == "windows" then
	bin( "elevate_for_update", {
		srcs = { "elevate_for_update.cc", "ggformat.cc" },
		rc = "elevate_for_update_manifest",
	} )
end

if config ~= "release" then
	bin( "genkeys", {
		srcs = { "genkeys.cc", "ggformat.cc", "ggentropy.cc" },
		libs = { "monocypher" },
		linux_ldflags = "-static",
	} )

	bin( "b2sum", {
		srcs = { "b2sum.cc", "ggformat.cc" },
		libs = { "monocypher" },
		linux_ldflags = "-static",
	} )
	msvc_obj_cxxflags( "b2sum%.cc", "/O2" )
	gcc_obj_cxxflags( "b2sum%.cc", "-O3" )

	if io.open( "secret_key.h" ) then
		bin( "sign", {
			srcs = { "sign.cc", "ggformat.cc" },
			libs = { "monocypher" },
			linux_ldflags = "-static",
		} )
	end
end

write_ninja_script()
