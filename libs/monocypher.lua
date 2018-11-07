lib( "monocypher", { "libs/monocypher/monocypher" } )
msvc_obj_cxxflags( "libs/monocypher/monocypher", "/O2" )
gcc_obj_cxxflags( "libs/monocypher/monocypher", "-O3 -g0" )
