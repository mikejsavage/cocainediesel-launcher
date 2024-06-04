lib( "stb_image", { "libs/stb/stb_image.cc" } )
obj_cxxflags( "libs/stb/stb_image.cc", "-DSTBI_NO_BMP -DSTBI_NO_GIF -DSTBI_NO_HDR -DSTBI_NO_LINEAR -DSTBI_NO_PIC -DSTBI_NO_PNM -DSTBI_NO_PSD -DSTBI_NO_TGA" )
msvc_obj_cxxflags( "libs/stb/stb_image.cc", "/O2 /wd4244 /wd4456" )
gcc_obj_cxxflags( "libs/stb/stb_image.cc", "-O2 -w" )
