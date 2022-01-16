lib( "monocypher", { "libs/monocypher/monocypher.cc" } )
msvc_obj_cxxflags( "libs/monocypher/monocypher%.cc", "/O2" )
gcc_obj_cxxflags( "libs/monocypher/monocypher%.cc", "-O3 -march=native -g0" )
