lib( "blake2b-avx", { "libs/blake2b-avx/blake2b.cc" } )
msvc_obj_cxxflags( "libs/blake2b%-avx/blake2b%.cc", "/O2 /arch:AVX2" )
gcc_obj_cxxflags( "libs/blake2b%-avx/blake2b%.cc", "-O3 -march=native -g0" )
