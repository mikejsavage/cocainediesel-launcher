lib( "imgui", { "libs/imgui/imgui", "libs/imgui/imgui_draw", "libs/imgui/imgui_impl_glfw_gl3" } )
msvc_obj_cxxflags( "libs/imgui/imgui_draw", "/DIMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION /DIMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION" )
gcc_obj_cxxflags( "libs/imgui/imgui_draw", "-Wno-maybe-uninitialized -DIMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION -DIMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION" )
