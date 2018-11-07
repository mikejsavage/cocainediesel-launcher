require( "scripts.gen_makefile" )

require( "libs/imgui" )
require( "libs/glfw" )
require( "libs/monocypher" )
require( "libs/stb" )
require( "libs/whereami" )

local game_ldflags = "-lX11 -lcurl"

if OS == "macos" then
	game_ldflags = "-framework Cocoa -framework CoreVideo -framework IOKit"
end

local objs = {
	"main", "cocainediesel_manifest",
	"ggformat", "strlcpy", "strtonum", "patterns",
	"gl", "glad", "liberation", "png",
}
local libs = { "glfw", "imgui", "monocypher", "stb_image", "stb_truetype", "whereami" }

bin( "cocainediesel", objs, libs )
msvc_bin_ldflags( "cocainediesel", "opengl32.lib gdi32.lib Ws2_32.lib" )
rc( "cocainediesel_manifest" )
gcc_bin_ldflags( "cocainediesel", game_ldflags )

if config == "release" then
	return
end

if io.open( "secret_key.h" ) then
	bin( "b2sum", { "b2sum", "ggformat" }, { "monocypher" } )
	gcc_obj_cxxflags( "b2sum", "-O2" )
	msvc_obj_cxxflags( "b2sum", "/O2" )

	bin( "genkeys", { "genkeys", "ggformat" }, { "monocypher" } )
	bin( "sign", { "sign", "ggformat" }, { "monocypher" } )
end
