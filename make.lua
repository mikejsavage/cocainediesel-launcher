require( "scripts.gen_ninja" )

require( "libs/imgui" )
require( "libs/glfw" )
require( "libs/monocypher" )
require( "libs/stb" )
require( "libs/whereami" )

local game_ldflags = "-lX11 -lcurl"

if OS == "macos" then
	game_ldflags = "-framework Cocoa -framework CoreVideo -framework IOKit"
end

local srcs = {
	"main.cc", -- "cocainediesel_manifest.cc",
	"ggformat.cc", "strlcpy.cc", "strtonum.cc", "patterns.cc",
	"gl.cc", "glad.cc", "liberation.cc", "png.cc",
}
local libs = { "glfw", "imgui", "monocypher", "stb_image", "stb_truetype", "whereami" }

bin( "cocainediesel", srcs, libs )
msvc_bin_ldflags( "cocainediesel", "opengl32.lib gdi32.lib Ws2_32.lib" )
rc( "cocainediesel_manifest" )
gcc_bin_ldflags( "cocainediesel", game_ldflags )

if config == "release" then
	return
end

if io.open( "secret_key.h" ) then
	bin( "b2sum", { "b2sum.cc", "ggformat.cc" }, { "monocypher" } )
	gcc_obj_cxxflags( "b2sum.cc", "-O2" )
	msvc_obj_cxxflags( "b2sum.cc", "/O2" )

	bin( "genkeys", { "genkeys.cc", "ggformat.cc" }, { "monocypher" } )
	bin( "sign", { "sign.cc", "ggformat.cc" }, { "monocypher" } )
end
