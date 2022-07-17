#include <string>
#include <vector>

#include "intrinsics.h"
#include "updater.h"
#include "log.h"
#include "gl.h"
#include "ggformat.h"
#include "png.h"
#include "str.h"
#include "platform_fs.h"
#include "platform_exec.h"
#include "platform_taskbar.h"

#define CURL_STATICLIB
#include "libs/curl/curl.h"

#define GLFW_INCLUDE_NONE
#include "libs/glfw/include/GLFW/glfw3.h"

#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_glfw.h"
#include "libs/imgui/imgui_impl_opengl2.h"

#if PLATFORM_WINDOWS
#define GAME_BINARY "client.exe"
#else
#define GAME_BINARY "client"
#endif

static const u8 logo[] = {
#include "logo.h"
};
static u8 montserrat[] = {
#include "montserrat.h"
};

static bool show_log = false;
static std::vector< std::string > log_lines;

static void log_cb( const char * msg ) {
	log_lines.push_back( std::string( msg ) );
}

template< typename... Rest >
static void right_text( const char * fmt, const Rest & ... rest ) {
	str< 256 > s( fmt, rest... );
	float width = ImGui::CalcTextSize( s.c_str() ).x;
	ImGui::SetCursorPosX( ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - width );
	ImGui::TextUnformatted( s.c_str() );
}

static void launcher_main( bool autostart ) {
	updater_init( autostart, log_cb, NULL );
	GLFWwindow * window = gl_init();
	taskbar_init();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL( window, true );
	ImGui_ImplOpenGL2_Init();

	glfwSwapInterval( 2 );

	u32 tex = load_png_memory( logo, sizeof( logo ) );

	ImFont * font_small;
	ImFont * font_large;

	{
		ImGuiIO & io = ImGui::GetIO();
		ImFontConfig config;
		config.FontDataOwnedByAtlas = false;
		config.OversampleV = 4;
		io.IniFilename = NULL;
		io.Fonts->AddFontFromMemoryTTF( montserrat, sizeof( montserrat ), 16.0f, &config );
		font_small = io.Fonts->AddFontFromMemoryTTF( montserrat, sizeof( montserrat ), 12.0f, &config );
		font_large = io.Fonts->AddFontFromMemoryTTF( montserrat, sizeof( montserrat ), 32.0f, &config );
	}

	{
		ImGuiStyle & style = ImGui::GetStyle();
		style.WindowRounding = 0;
		style.FramePadding = ImVec2( 8, 8 );
		style.FrameBorderSize = 3;
		style.WindowBorderSize = 0;
		style.Colors[ ImGuiCol_Border ] = ImColor( 0xee, 0xee, 0xee, 0xee );
		style.Colors[ ImGuiCol_Button ] = ImColor( 0, 0, 0, 0 );
		style.Colors[ ImGuiCol_ButtonHovered ] = ImColor( 0, 0, 0, 64 );
		style.Colors[ ImGuiCol_ButtonActive ] = ImColor( 0, 0, 0, 128 );
		style.Colors[ ImGuiCol_FrameBg ] = ImColor( 0, 0, 0, 0 );
		style.Colors[ ImGuiCol_PlotHistogram ] = ImColor( 0, 0, 0, 128 );
		style.Colors[ ImGuiCol_WindowBg ] = ImColor( 0, 0, 0, 0 );
		style.ItemSpacing.y = 8;
	}

	UpdaterState updater_state = UpdaterState_Init;

	while( !glfwWindowShouldClose( window ) ) {
		if( ( updater_state == UpdaterState_NeedsUpdate || updater_state == UpdaterState_ReadyToPlay ) )
			glfwWaitEvents();
		else
			glfwPollEvents();

		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		v2u32 window_size = get_window_size();
		ImGui::GetStyle().WindowPadding = ImVec2( 0, 0 );
		ImGui::SetNextWindowPos( ImVec2() );
		ImGui::SetNextWindowSize( ImVec2( window_size.x, window_size.y ) );
		ImGui::Begin( "logo", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus );

		ImGui::Image( ( void * ) checked_cast< uptr >( tex ), ImGui::GetWindowSize() );

		ImGui::End();

		ImGui::GetStyle().WindowPadding = ImVec2( 32, 16 );
		ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
		ImGui::SetNextWindowSize( ImVec2( window_size.x, window_size.y ) );
		ImGui::Begin( "controls", NULL, ImGuiWindowFlags_NoDecoration );

		updater_state = updater_update();

		ImGui::SameLine();

		const ImVec2 log_size = ImVec2( 0, 310 );
		if( show_log ) {
			if( ImGui::Button( "Hide log" ) ) {
				show_log = false;
				glfwPostEmptyEvent();
			}

			ImGui::PushStyleColor( ImGuiCol_FrameBg, IM_COL32( 0, 0, 0, 128 ) );
			ImGui::BeginChildFrame( 1337, log_size, ImGuiWindowFlags_AlwaysVerticalScrollbar );
			ImGui::PushFont( font_small );
			for( const std::string & line : log_lines ) {
				ImGui::TextWrapped( "%s", line.c_str() );
			}
			ImGui::PopFont();
			ImGui::EndChildFrame();
			ImGui::PopStyleColor();
		}
		else {
			if( ImGui::Button( "Show log" ) ) {
				show_log = true;
				glfwPostEmptyEvent();
			}
			ImGui::Dummy( log_size );
		}

		right_text( "v{}", updater_local_version() );

		bool enter_key_pressed = glfwGetKey( window, GLFW_KEY_ENTER ) == GLFW_PRESS || glfwGetKey( window, GLFW_KEY_SPACE ) == GLFW_PRESS;

		ImGui::PushFont( font_large );
		if( updater_state == UpdaterState_ReadyToPlay ) {
			bool launch = ImGui::Button( "Play", ImVec2( -1, 50 ) );

			if( launch || enter_key_pressed ) {
				run_game( GAME_BINARY );
			}
		}
		else if( updater_state == UpdaterState_NeedsUpdate ) {
			str< 256 > button_text( "Update to v{} - {.2}MB", updater_remote_version(), updater_update_size() / 1000.0 / 1000.0 );
			bool update = ImGui::Button( button_text.c_str(), ImVec2( -1, 50 ) );

			if( update || enter_key_pressed ) {
#if PLATFORM_WINDOWS
				exec_and_quit( "elevate_for_update.exe" );
#else
				updater_start_update();
#endif
			}
		}
		else if( updater_state == UpdaterState_DownloadingUpdate ) {
			float progress = updater_download_progress();

			const char * unit = "KB/s";
			double display_speed = updater_download_speed() / 1000.0;
			if( display_speed > 1000.0 ) {
				unit = "MB/s";
				display_speed /= 1000.0;
			}

			str< 256 > progress_text( "{}%. {.1}{}", int( progress * 100 ), display_speed, unit );
			ImGui::ProgressBar( progress, ImVec2( -1, 50 ), progress_text.c_str() );

			taskbar_progress( window, updater_bytes_downloaded(), updater_update_size() );
		}
		else {
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0, 0, 0, 0 ) );

			if( updater_state == UpdaterState_InstallingUpdate ) {
				ImGui::Button( "Installing update...", ImVec2( -1, 50 ) );

				taskbar_clear( window );
			}
			else {
				ImGui::Button( "Checking for updates...", ImVec2( -1, 50 ) );
			}

			ImGui::PopStyleColor( 2 );
		}

		ImGui::PopFont();

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData( ImGui::GetDrawData() );

		glfwSwapBuffers( window );
	}

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	taskbar_term();
	gl_term();
	updater_term();
}

#if PLATFORM_WINDOWS

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nShowCmd ) {
	bool autostart = strcmp( lpszCmdLine, "--start-update" ) == 0;
	launcher_main( autostart );

	return 0;
}

#else

int main( int argc, char ** argv ) {
	bool autostart = argc == 2 && strcmp( argv[ 1 ], "--start-update" ) == 0;
	launcher_main( autostart );

	return 0;
}

#endif
