require( "scripts.gen_ninja" )

require( "libs/imgui" )
require( "libs/glfw" )
require( "libs/monocypher" )
require( "libs/picohttpparser" )
require( "libs/stb" )
require( "libs/whereami" )

bin( "cocainediesel", {
	srcs = {
		"main.cc", "updater.cc", "ggformat.cc", "strlcpy.cc", "strtonum.cc", "patterns.cc",
		"platform_taskbar.cc", "gl.cc", "glad.cc", "png.cc", "discord.cc",
	},

	libs = { "glfw", "imgui", "monocypher", "picohttpparser", "stb_image", "whereami" },
	prebuilt_libs = { "curl", OS == "linux" and "mbedtls" or nil },

	rc = "cocainediesel_manifest",

	msvc_extra_ldflags = "opengl32.lib gdi32.lib ole32.lib Ws2_32.lib crypt32.lib",
	gcc_extra_ldflags = "-lX11",
} )

bin( "headlessupdater", {
	srcs = { "headless.cc", "updater.cc", "ggformat.cc", "strlcpy.cc", "strtonum.cc", "patterns.cc" },

	libs = { "monocypher", "whereami" },
	prebuilt_libs = { "curl", OS == "linux" and "mbedtls" or nil },

	msvc_extra_ldflags = "Ws2_32.lib crypt32.lib",
} )

if OS == "windows" then
        bin( "elevate_for_update", {
		srcs = { "elevate_for_update.cc", "ggformat.cc" },
		rc = "elevate_for_update_manifest",
	} )
end

if config == "release" then
	return
end

bin( "genkeys", {
	srcs = { "genkeys.cc", "ggformat.cc", "ggentropy.cc" },
	libs = { "monocypher" }
} )

bin( "b2sum", {
	srcs = { "b2sum.cc", "ggformat.cc" },
	libs = { "monocypher" },
} )
gcc_obj_cxxflags( "b2sum.cc", "-O2" )
msvc_obj_cxxflags( "b2sum.cc", "/O2" )

if io.open( "secret_key.h" ) then
	bin( "sign", {
		srcs = { "sign.cc", "ggformat.cc" },
		libs = { "monocypher" }
	} )
end
