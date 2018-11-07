#! /bin/sh

set -e

cd libs/glad
python -m glad --profile=core --api=gl=3.3 --generator=c --spec=gl --out-path gladout --omit-khrplatform \
	--extensions GL_KHR_debug,GL_AMD_debug_output,GL_EXT_texture_sRGB,GL_EXT_texture_sRGB_decode,GL_EXT_texture_compression_s3tc,GL_EXT_texture_filter_anisotropic,GL_ARB_get_program_binary,GL_NVX_gpu_memory_info,GL_ATI_meminfo
mv gladout/src/glad.c ../../glad.cc
mv gladout/include/glad/glad.h ../../glad.h
rm -r gladout
sed -i "s/#include <glad\\/glad\\.h>/#include \"glad.h\"/" ../../glad.cc
