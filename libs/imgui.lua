lib( "imgui", { "libs/imgui/imgui.cc", "libs/imgui/imgui_draw.cc", "libs/imgui/imgui_widgets.cc", "libs/imgui/imgui_impl_glfw.cc", "libs/imgui/imgui_impl_opengl2.cc" } )
msvc_obj_cxxflags( "libs/imgui/imgui_draw.cc", "/DIMGUI_STB_TRUETYPE_FILENAME=\\\"libs/stb/stb_truetype.h\\\" /DIMGUI_STB_RECT_PACK_FILENAME=\\\"libs/stb/stb_rect_pack.h\\\"" )
gcc_obj_cxxflags( "libs/imgui/imgui_draw.cc", "-Wno-maybe-uninitialized -DIMGUI_STB_TRUETYPE_FILENAME=\\\"libs/stb/stb_truetype.h\\\" -DIMGUI_STB_RECT_PACK_FILENAME=\\\"libs/stb/stb_rect_pack.h\\\"" )
