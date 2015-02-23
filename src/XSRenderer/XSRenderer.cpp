#include <SDL2/SDL.h>

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSConsole.h"
#include "XSCommon/XSString.h"
#include "XSCommon/XSError.h"
#include "XSCommon/XSCvar.h"
#include "XSRenderer/XSRenderer.h"
#include "XSRenderer/XSView.h"
#include "XSRenderer/XSRenderCommand.h"
#include "XSRenderer/XSBackend.h"
#include "XSRenderer/XSFramebuffer.h"
#include "XSRenderer/XSShaderProgram.h"
#include "XSRenderer/XSTexture.h"
#include "XSRenderer/XSFont.h"

#if defined(XS_OS_MAC)
#include <OpenCL/cl_gl_ext.h>
#endif

namespace XS {

	namespace Renderer {

		State state = {};

		static SDL_Window *window = nullptr;
		static SDL_GLContext context;

		Cvar *r_clear = nullptr;
		Cvar *r_debug = nullptr;
		Cvar *r_multisample = nullptr;
		Cvar *r_skipRender = nullptr;
		Cvar *r_swapInterval = nullptr;
		Cvar *vid_height = nullptr;
		Cvar *vid_noBorder = nullptr;
		Cvar *vid_width = nullptr;

		std::vector<View *> views;
		static View *currentView = nullptr;

		static const char *GLErrSeverityToString( GLenum severity ) {
			switch ( severity ) {
			case GL_DEBUG_SEVERITY_HIGH_ARB: {
				return "High";
			} break;

			case GL_DEBUG_SEVERITY_MEDIUM_ARB: {
				return "Medium";
			} break;

			case GL_DEBUG_SEVERITY_LOW_ARB: {
				return "Low";
			} break;

			default: {
				return "?";
			} break;
			}
		}

		static const char *GLErrSourceToString( GLenum source ) {
			switch ( source ) {
			case GL_DEBUG_SOURCE_API_ARB: {
				return "API";
			} break;

			case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: {
				return "WS";
			} break;

			case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: {
				return "GLSL";
			} break;

			case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: {
				return "3rd";
			} break;

			case GL_DEBUG_SOURCE_APPLICATION_ARB: {
				return "App";
			} break;

			case GL_DEBUG_SOURCE_OTHER_ARB: {
				return "Other";
			} break;

			default: {
				return "?";
			} break;
			}
		}

		static const char *GLErrTypeToString( GLenum type ) {
			switch ( type ) {
			case GL_DEBUG_TYPE_ERROR_ARB: {
				return "Error";
			} break;

			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: {
				return "Deprecated";
			} break;

			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: {
				return "UB";
			} break;

			case GL_DEBUG_TYPE_PORTABILITY_ARB: {
				return "Portability";
			} break;

			case GL_DEBUG_TYPE_PERFORMANCE_ARB: {
				return "Performance";
			} break;

			case GL_DEBUG_TYPE_OTHER_ARB: {
				return "Other";
			} break;

			default: {
				return "?";
			} break;
			}
		}

		static void OnGLError( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
			const GLchar *message, GLvoid *userParam )
		{
			const int level = r_debug->GetInt32();
			if ( !level || (level == 1 && type == GL_DEBUG_TYPE_PERFORMANCE_ARB) ) {
				return;
			}

			console.Print( PrintLevel::Normal,
				"[%s] [%s] %s: %s\n",
				GLErrSeverityToString( severity ),
				GLErrSourceToString( source ),
				GLErrTypeToString( type ),
				message
			);
		}

		void Init( void ) {
			state.valid = true;

			RegisterCvars();

			CreateDisplay();

			glewExperimental = GL_TRUE;
			GLenum error = glewInit();
			if ( error != GLEW_OK ) {
				throw( XSError( String::Format( "Failed to initialise GLEW: %s\n",
					glewGetErrorString( error ) ).c_str() ) );
			}

			if ( GLEW_ARB_debug_output ) {
				glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
				glDebugMessageCallbackARB( OnGLError, nullptr );
			}

			Backend::Init();

			Texture::Init();
			ShaderProgram::Init();
			Font::Init();

			RenderCommand::Init();

			glViewport( 0, 0, state.window.width, state.window.height );
		}

		void Shutdown( void ) {
			console.Print( PrintLevel::Normal, "Shutting down renderer...\n" );

			RenderCommand::Shutdown();
			Font::Shutdown();
			Backend::Shutdown();

			DestroyDisplay();
		}

		void RegisterCvars( void ) {
			r_clear = Cvar::Create( "r_clear", "0.5 0.0 0.0 1.0", "Colour of the backbuffer", CVAR_ARCHIVE );
			r_debug = Cvar::Create( "r_debug", "0", "Enable debugging information", CVAR_ARCHIVE );
			r_multisample = Cvar::Create( "r_multisample", "2", "Multisample Anti-Aliasing (MSAA) level",
				CVAR_ARCHIVE );
			r_skipRender = Cvar::Create( "r_skipRender", "0", "1 - skip 3D views, 2 - skip 2D views, 3 - skip all "
				"views", CVAR_ARCHIVE );
			r_swapInterval = Cvar::Create( "r_swapInterval", "0", "Enable vertical sync", CVAR_ARCHIVE );
			vid_height = Cvar::Create( "vid_height", "720", "Window height", CVAR_ARCHIVE );
			vid_noBorder = Cvar::Create( "vid_noBorder", "0", "Disable window border", CVAR_ARCHIVE );
			vid_width = Cvar::Create( "vid_width", "1280", "Window width", CVAR_ARCHIVE );
		}

		void CreateDisplay( void ) {
			Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

			if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
				return;
			}

			if ( vid_noBorder->GetBool() ) {
				windowFlags |= SDL_WINDOW_BORDERLESS;
			}

			// targeting OpenGL 3.1 core
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG );

			int multisample = r_multisample->GetInt32();
			if ( multisample ) {
				SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
				SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, multisample );
			}
			else {
				SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
			}

		#ifdef _DEBUG
			// request debug context
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
		#endif

			const int32_t width = vid_width->GetInt32();
			const int32_t height = vid_height->GetInt32();
			window = SDL_CreateWindow(
				WINDOW_TITLE,
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				width, height,
				windowFlags
			);
			SDL_assert( window && "Failed to create window" );

			context = SDL_GL_CreateContext( window );
			SDL_assert( context && "Failed to create OpenGL context on window" );

			SDL_GL_MakeCurrent( window, context );
			state.window.valid = true;
			state.window.width = static_cast<uint32_t>( width );
			state.window.height = static_cast<uint32_t>( height );
			state.window.aspectRatio = vid_height->GetReal64() / vid_width->GetReal64();

			SDL_GL_SetSwapInterval( r_swapInterval->GetInt32() );
		#if defined(XS_OS_MAC)
			//TODO: force vsync flag in CGL, seems to only have an Obj-C API?
			/*
			CGLContextObj cglContext = CGLGetCurrentContext();
			if ( cglContext ) {
				// ...
			}
			*/
		#endif

			state.driver.vendor = reinterpret_cast<const char *>( glGetString( GL_VENDOR ) );
			state.driver.renderer = reinterpret_cast<const char *>( glGetString( GL_RENDERER ) );
			state.driver.coreVersion = reinterpret_cast<const char *>( glGetString( GL_VERSION ) );
			state.driver.shaderVersion = reinterpret_cast<const char *>( glGetString( GL_SHADING_LANGUAGE_VERSION ) );

			console.Print( PrintLevel::Normal,
				"OpenGL device: %s %s\n",
				state.driver.vendor,
				state.driver.renderer
			);
			console.Print( PrintLevel::Normal,
				"OpenGL version: %s with GLSL %s\n",
				state.driver.coreVersion,
				state.driver.shaderVersion
			);
		}

		void DestroyDisplay( void ) {
			SDL_GL_DeleteContext( context );
			context = nullptr;

			SDL_DestroyWindow( window );
			window = nullptr;
			state.window = {};

			SDL_Quit();
		}

		void Update( void ) {
			const vector4 clear( r_clear->GetReal32( 0 ), r_clear->GetReal32( 1 ), r_clear->GetReal32( 2 ),
				r_clear->GetReal32( 3 ) );
			glClearColor( clear.r, clear.g, clear.b, clear.a );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			for ( const auto &view : views ) {
				if ( r_skipRender->GetInt32() & (1 << static_cast<uint32_t>( view->is2D )) ) {
					continue;
				}

				view->PreRender();
				while ( !view->renderCommands.empty() ) {
					const auto &cmd = view->renderCommands.front();

					cmd.Execute();

					view->renderCommands.pop();
				}
				view->PostRender();
			}

			SDL_GL_SwapWindow( window );
		}

		void RegisterView( View *view ) {
			views.push_back( view );
		}

		void BindView( View *view ) {
			currentView = view;
		}

		static void AssertView( void ) {
			if ( !currentView ) {
				throw( XSError( "Attempted to issue render command without binding a view" ) );
			}
		}

		void DrawQuad( real32_t x, real32_t y, real32_t w, real32_t h, real32_t s1, real32_t t1, real32_t s2,
			real32_t t2, const vector4 *colour, const Material *material )
		{
			AssertView();

			RenderCommand cmd( CommandType::DrawQuad );
			cmd.drawQuad.x = x;
			cmd.drawQuad.y = y;
			cmd.drawQuad.w = w;
			cmd.drawQuad.h = h;
			cmd.drawQuad.s1 = s1;
			cmd.drawQuad.t1 = t1;
			cmd.drawQuad.s2 = s2;
			cmd.drawQuad.t2 = t2;
			cmd.drawQuad.colour = colour;
			cmd.drawQuad.material = material;

			currentView->renderCommands.push( cmd );
		}

		void DrawModel( const Model *model ) {
			AssertView();

			RenderCommand cmd( CommandType::DrawModel );
			cmd.drawModel.model = model;
			currentView->renderCommands.push( cmd );
		}

	} // namespace Renderer

} // namespace XS
