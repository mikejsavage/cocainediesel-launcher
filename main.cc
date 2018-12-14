#include <string>
#include <vector>

#include "intrinsics.h"
#include "updater.h"
#include "discord.h"
#include "log.h"
#include "gl.h"
#include "ggformat.h"
#include "png.h"
#include "str.h"
#include "platform_exec.h"
#include "platform_taskbar.h"
#include "liberation.h"

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

static std::vector< std::string > log_lines;

static void log_cb( const char * msg ) {
	log_lines.push_back( std::string( msg ) );
}

static const char * discord_step() {
	DiscordState state = discord_update();

	if( state == DiscordState_Unauthenticated ) {
		bool login = ImGui::Button( "Link with with Discord" );
		if( login ) {
			discord_login();
		}
	}
	else if( state == DiscordState_Authenticating ) {
		ImGui::Text( "Linking... check your browser" );
	}
	else if( state == DiscordState_Authenticated ) {
		DiscordUser user = discord_user();
		ImGui::Text( "Hi %s#%s. Run the game and then you won't need to do this again", user.username, user.discriminator );
		return user.id;
	}

	return NULL;
}

static void launcher_main( bool autostart ) {
	updater_init( autostart, log_cb );
	discord_init();
	GLFWwindow * window = gl_init();
	taskbar_init();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	glfwSwapInterval( 2 );

	u32 tex = load_png_memory( logo, sizeof( logo ) );

	ImFont * small;
	ImFont * large;

	{
		u8 * data = LiberationSans_Regular_ttf;
		int size = LiberationSans_Regular_ttf_len;
		ImGuiIO & io = ImGui::GetIO();
		ImFontConfig config;
		config.FontDataOwnedByAtlas = false;
		config.OversampleV = 4;
		io.IniFilename = NULL;
		io.Fonts->AddFontFromMemoryTTF( data, size, 16.0f, &config );
		small = io.Fonts->AddFontFromMemoryTTF( data, size, 12.0f, &config );
		large = io.Fonts->AddFontFromMemoryTTF( data, size, 32.0f, &config );
	}

	{
		ImGuiStyle & style = ImGui::GetStyle();
		style.WindowRounding = 0;
		style.FramePadding = ImVec2( 8, 8 );
		style.FrameBorderSize = 0;
		style.WindowBorderSize = 0;
		style.Colors[ ImGuiCol_WindowBg ] = ImColor( 0x1a, 0x1a, 0x1a );
		style.ItemSpacing.y = 8;
	}

	UpdaterState updater_state = UpdaterState_Init;

	while( !glfwWindowShouldClose( window ) ) {
		if( ( updater_state == UpdaterState_NeedsUpdate || updater_state == UpdaterState_ReadyToPlay ) && discord_state() != DiscordState_Authenticating )
			glfwWaitEvents();
		else
			glfwPollEvents();

		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		float logo_height = 256;

		v2u32 window_size = get_window_size();
		ImGui::GetStyle().WindowPadding = ImVec2( 0, 0 );
		ImGui::SetNextWindowPos( ImVec2() );
		ImGui::SetNextWindowSize( ImVec2( window_size.x, logo_height ) );
		ImGui::Begin( "logo", NULL, ImGuiWindowFlags_NoDecoration );

		ImGui::Image( ( void * ) checked_cast< uptr >( tex ), ImVec2( 750, logo_height ) );

		ImGui::End();

		ImGui::GetStyle().WindowPadding = ImVec2( 32, 16 );
		ImGui::SetNextWindowPos( ImVec2( 0, logo_height ) );
		ImGui::SetNextWindowSize( ImVec2( window_size.x, window_size.y - logo_height ) );
		ImGui::Begin( "controls", NULL, ImGuiWindowFlags_NoDecoration );

		updater_state = updater_update();
		const char * discord_user_id = discord_step();

		bool enter_key_pressed = glfwGetKey( window, GLFW_KEY_ENTER ) == GLFW_PRESS || glfwGetKey( window, GLFW_KEY_SPACE ) == GLFW_PRESS;

		ImGui::PushFont( large );
		if( updater_state == UpdaterState_ReadyToPlay ) {
			ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 0x29, 0x8a, 0x67, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0x30, 0xa1, 0x78, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0x38, 0xb9, 0x8a, 0xff ) );

			bool launch = ImGui::Button( "Play", ImVec2( -1, 50 ) );
			ImGui::PopStyleColor( 3 );

			if( launch || enter_key_pressed ) {
				run_game( GAME_BINARY, discord_user_id );
			}
		}
		else if( updater_state == UpdaterState_NeedsUpdate ) {
			ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 0x29, 0x8a, 0x67, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0x30, 0xa1, 0x78, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0x38, 0xb9, 0x8a, 0xff ) );

			str< 256 > button_text( "Update to v{} - {.2}MB", updater_remote_version(), updater_update_size() / 1000.0 / 1000.0 );
			bool update = ImGui::Button( button_text.c_str(), ImVec2( -1, 50 ) );
			ImGui::PopStyleColor( 3 );

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

			str< 256 > progress_text( "{.2}/{.2}MB. {.2}% {.2}{}",
				updater_bytes_downloaded() / 1000.0 / 1000.0,
				updater_update_size() / 1000.0 / 1000.0,
				progress * 100, display_speed, unit );

			ImGui::PushStyleColor( ImGuiCol_PlotHistogram, IM_COL32( 0x29, 0x8a, 0x67, 0xff ) );
			ImGui::ProgressBar( progress, ImVec2( -1, 50 ), progress_text.c_str() );
			ImGui::PopStyleColor();

			taskbar_progress( window, updater_bytes_downloaded(), updater_update_size() );
		}
		else if( updater_state == UpdaterState_InstallingUpdate ) {
			ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::Button( "Installing update...", ImVec2( -1, 50 ) );
			ImGui::PopStyleColor( 3 );

			taskbar_clear( window );
		}
		else {
			ImGui::PushStyleColor( ImGuiCol_Button, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, IM_COL32( 0x1c, 0x2a, 0x59, 0xff ) );
			ImGui::Button( "Checking for updates...", ImVec2( -1, 50 ) );
			ImGui::PopStyleColor( 3 );
		}

		ImGui::PopFont();

		Version local_version = updater_local_version();
		ImGui::Text( "Currently installed: v%hhu.%hhu.%hhu.%hhu", local_version.a, local_version.b, local_version.c, local_version.d );

		ImGui::BeginChildFrame( 1337, ImVec2(), 0
			| ImGuiWindowFlags_AlwaysVerticalScrollbar
		);
		ImGui::PushFont( small );
		for( const std::string & line : log_lines ) {
			ImGui::TextWrapped( "%s", line.c_str() );
		}
		ImGui::SetScrollHereY();
		ImGui::PopFont();
		ImGui::EndChildFrame();

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
	discord_term();
	updater_term();
}

#if PLATFORM_WINDOWS

int WinMain( HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nShowCmd ) {
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
