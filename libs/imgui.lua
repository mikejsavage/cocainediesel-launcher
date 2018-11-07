lib( "imgui", { "libs/imgui/imgui.cc", "libs/imgui/imgui_draw.cc", "libs/imgui/imgui_impl_glfw_gl3.cc" } )
msvc_obj_cxxflags( "libs/imgui/imgui_draw.cc", "/DIMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION /DIMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION" )
gcc_obj_cxxflags( "libs/imgui/imgui_draw.cc", "-Wno-maybe-uninitialized -DIMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION -DIMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION" )
