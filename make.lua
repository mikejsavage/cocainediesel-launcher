require( "scripts.gen_ninja" )

require( "libs/imgui" )
require( "libs/glfw" )
require( "libs/monocypher" )
require( "libs/stb" )
require( "libs/whereami" )

local gcc_ldflags = "-lX11 -lcurl"
local prebuild_libs

if OS == "windows" then
	prebuilt_libs = { "curl" }
end

if OS == "macos" then
	gcc_ldflags = "-framework Cocoa -framework CoreVideo -framework IOKit"
end

bin( "cocainediesel", {
	srcs = {
		"main.cc", "ggformat.cc", "strlcpy.cc", "strtonum.cc", "patterns.cc",
		"gl.cc", "glad.cc", "liberation.cc", "png.cc",
	},

	libs = { "glfw", "imgui", "monocypher", "stb_image", "stb_truetype", "whereami" },
	prebuilt_libs = prebuilt_libs,

	rc = "cocainediesel_manifest",

	msvc_extra_ldflags = "opengl32.lib gdi32.lib Ws2_32.lib",
	gcc_extra_ldflags = gcc_ldflags,
} )

if config == "release" then
	return
end

if io.open( "secret_key.h" ) then
	bin( "b2sum", {
		srcs = { "b2sum.cc", "ggformat.cc" },
		libs = { "monocypher" },
	} )
	gcc_obj_cxxflags( "b2sum.cc", "-O2" )
	msvc_obj_cxxflags( "b2sum.cc", "/O2" )

	bin( "genkeys", {
		srcs = { "genkeys.cc", "ggformat.cc" },
		libs = { "monocypher" }
	} )
	bin( "sign", {
		srcs = { "sign.cc", "ggformat.cc" },
		libs = { "monocypher" }
	} )
end
