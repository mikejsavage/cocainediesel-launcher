#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "str.h"
#include "icon.h"
#include "gl.h"

#include "glad.h"
#include "libs/stb/stb_image.h"

#define GLFW_INCLUDE_NONE
#include "libs/glfw/include/GLFW/glfw3.h"

#define RESET "\x1b[0m"
#define RED "\x1b[1;31m"
#define YELLOW "\x1b[1;32m"
#define GREEN "\x1b[1;33m"

static v2u32 window_size;

static const char * type_string( GLenum type ) {
	switch( type ) {
		case GL_DEBUG_TYPE_ERROR:
		case GL_DEBUG_CATEGORY_API_ERROR_AMD:
			return "error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
			return "deprecated";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
			return "undefined";
		case GL_DEBUG_TYPE_PORTABILITY:
			return "nonportable";
		case GL_DEBUG_TYPE_PERFORMANCE:
		case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
			return "performance";
		case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
			return "window system";
		case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
			return "shader compiler";
		case GL_DEBUG_CATEGORY_APPLICATION_AMD:
			return "application";
		case GL_DEBUG_TYPE_OTHER:
		case GL_DEBUG_CATEGORY_OTHER_AMD:
			return "other";
		default:
			return "idk";
	}
}

static const char * severity_string( GLenum severity ) {
	switch( severity ) {
		case GL_DEBUG_SEVERITY_LOW:
		// case GL_DEBUG_SEVERITY_LOW_AMD:
			return GREEN "low" RESET;
		case GL_DEBUG_SEVERITY_MEDIUM:
		// case GL_DEBUG_SEVERITY_MEDIUM_AMD:
			return YELLOW "medium" RESET;
		case GL_DEBUG_SEVERITY_HIGH:
		// case GL_DEBUG_SEVERITY_HIGH_AMD:
			return RED "high" RESET;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			return "notice";
		default:
			return "idk";
	}
}

static void gl_debug_output_callback(
	GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
	const GLchar * message, const void * _
) {
	if(
	    source == 33352 || // shader compliation errors
	    id == 131169 ||
	    id == 131185 ||
	    id == 131218 ||
	    id == 131204
	) {
		return;
	}

	if( severity == GL_DEBUG_SEVERITY_NOTIFICATION || severity == GL_DEBUG_SEVERITY_NOTIFICATION_KHR ) {
		return;
	}

	const char * nl = "\n";
	size_t message_len = strlen( message );
	if( message_len > 0 && message[ message_len - 1 ] == '\n' )
		nl = "";

	WARN( "GL {} [{} - {}]: {}{}",
		source,
		type_string( type ),
		severity_string( severity ),
		message, nl );

	if( severity == GL_DEBUG_SEVERITY_HIGH ) {
		ASSERT( 0 );
	}
}

static void gl_debug_output_callback_amd(
	GLuint id, GLenum type, GLenum severity, GLsizei length,
	const GLchar * message, const void * _
) {
	gl_debug_output_callback( GL_DONT_CARE, type, id, severity, length, message, _ );
}

static void glfw_error_callback( int code, const char * message ) {
	WARN( "GLFW error {}: {}", code, message );
}

static void glfw_focus_callback( GLFWwindow * window, int focused ) {
	if( focused == GLFW_TRUE ) {
		glfwSetCursorPos( window, window_size.x / 2, window_size.y / 2 );
	}
}

GLFWwindow * gl_init() {
	glfwSetErrorCallback( glfw_error_callback );

	if( !glfwInit() ) {
		FATAL( "glfwInit" );
	}

	glfwWindowHint( GLFW_RESIZABLE, 0 );

	int width = 691;
	int height = 518;

	GLFWmonitor * monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode * mode = glfwGetVideoMode( monitor );

	glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_API );
	// glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
	// glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );
#if !RELEASE_BUILD && 0
	glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, 1 );
#endif

	GLFWwindow * window = glfwCreateWindow( width, height, "Cocaine Diesel", NULL, NULL );
	if( !window ) {
		FATAL( "glfwCreateWindow" );
	}

	GLFWimage icon;
	icon.pixels = stbi_load_from_memory( icon_png, icon_png_len, &icon.width, &icon.height, NULL, 4 );
	ASSERT( icon.pixels != NULL );
	glfwSetWindowIcon( window, 1, &icon );
	stbi_image_free( icon.pixels );

	int frame_top, frame_bottom, frame_left, frame_right;
	glfwGetWindowFrameSize( window, &frame_left, &frame_top, &frame_right, &frame_bottom );

	int monitor_top, monitor_left;
	glfwGetMonitorPos( monitor, &monitor_left, &monitor_top );

	int total_width = width + frame_left + frame_right;
	int total_height = height + frame_top + frame_bottom;

	glfwSetWindowPos( window,
		monitor_left + mode->width / 2 - total_width / 2,
		monitor_top + mode->height / 2 - total_height / 2 );

	window_size = v2u32( checked_cast< u32 >( width ), checked_cast< u32 >( height ) );

	glfwMakeContextCurrent( window );

	if( gladLoadGLLoader( ( GLADloadproc ) glfwGetProcAddress ) != 1 ) {
		FATAL( "gladLoadGL" );
	}

	struct {
		const char * name;
		int enabled;
	} exts[] = {
#if !RELEASE_BUILD
		{ "KHR_debug", GLAD_GL_KHR_debug },
		{ "AMD_debug_output", GLAD_GL_AMD_debug_output },
#endif
		{ "EXT_texture_sRGB", GLAD_GL_EXT_texture_sRGB },
		{ "EXT_texture_sRGB_decode", GLAD_GL_EXT_texture_sRGB_decode },
		{ "EXT_texture_compression_s3tc", GLAD_GL_EXT_texture_compression_s3tc },
	};

	const char * vendor = ( const char * ) glGetString( GL_VENDOR );
	const char * version = ( const char * ) glGetString( GL_VERSION );
	INFO( "Version {}", version );
	INFO( "Vendor {}", vendor );

	INFO( "OpenGL extensions:" );
	for( size_t i = 0; i < ARRAY_COUNT( exts ); i++ ) {
		INFO( "{}: {}", exts[ i ].name, exts[ i ].enabled == 0 ? "missing" : "present" );
	}

#if !RELEASE_BUILD && 0
	bool nvidia_and_windows = false;
#if PLATFORM_WINDOWS
	if( strstr( vendor, "NVIDIA" ) ) {
		nvidia_and_windows = true;
	}
#endif

	if( GLAD_GL_KHR_debug != 0 && !nvidia_and_windows ) {
		GLint context_flags;
		glGetIntegerv( GL_CONTEXT_FLAGS, &context_flags );
		if( context_flags & GL_CONTEXT_FLAG_DEBUG_BIT ) {
			INFO( "Initialising debug output" );

			glEnable( GL_DEBUG_OUTPUT );
			glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
			glDebugMessageCallback( ( GLDEBUGPROC ) gl_debug_output_callback, NULL );
			glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE );
		}
	}
	else if( GLAD_GL_AMD_debug_output != 0 ) {
		INFO( "Initialising AMD debug output" );

		glDebugMessageCallbackAMD( ( GLDEBUGPROCAMD ) gl_debug_output_callback_amd, NULL );
		glDebugMessageEnableAMD( 0, 0, 0, NULL, GL_TRUE );
	}
#endif

	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LESS );

	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	return window;
}

void gl_term() {
	// gl_check_for_leaks();
	glfwTerminate();
}

v2u32 get_window_size() {
	return window_size;
}

float get_aspect_ratio() {
	return float( window_size.x ) / float( window_size.y );
}
