require( "scripts.gen_ninja" )

require( "libs/imgui" )
require( "libs/glfw" )
require( "libs/monocypher" )
require( "libs/stb" )
require( "libs/whereami" )

bin( "cocainediesel", {
	srcs = {
		"main.cc", "ggformat.cc", "strlcpy.cc", "strtonum.cc", "patterns.cc",
		"gl.cc", "glad.cc", "liberation.cc", "png.cc",
	},

	libs = { "glfw", "imgui", "monocypher", "stb_image", "stb_truetype", "whereami" },
	prebuilt_libs = { "curl", OS == "linux" and "mbedtls" },

	rc = "cocainediesel_manifest",

	msvc_extra_ldflags = "opengl32.lib gdi32.lib Ws2_32.lib crypt32.lib",
	gcc_extra_ldflags = "-lX11",
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
