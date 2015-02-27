#include "XSCommon/XSCommon.h"
#include "XSCommon/XSCvar.h"
#include "XSCommon/XSCommand.h"
#include "XSRenderer/XSRenderer.h"
#include "XSRenderer/XSBackend.h"
#include "XSRenderer/XSScreenshot.h"

namespace XS {

	namespace Renderer {

		namespace Backend {

			Cvar *r_fov = nullptr;
			Cvar *r_zRange = nullptr;

			GLuint defaultVao = 0u;
			GLuint defaultPbo = 0u;

			static void RegisterCvars( void ) {
				r_fov = Cvar::Create( "r_fov", "110", "Field of view", CVAR_ARCHIVE );
				r_zRange = Cvar::Create( "r_zRange", "4.0 1000.0", "Clipping plane range", CVAR_ARCHIVE );
			}

			void Init( void ) {
				RegisterCvars();
				Command::AddCommand( "screenshot", Cmd_Screenshot );

				glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
				glClear( GL_COLOR_BUFFER_BIT );
				glClearDepth( 1.0f );

				// state changes
				ToggleDepthTest( true );
				SetDepthFunction( DepthFunc::LessOrEqual );

				// back-face culling
				glDisable( GL_CULL_FACE );
				glCullFace( GL_BACK );
				glFrontFace( GL_CCW );

				// alpha blending
				ToggleAlphaBlending( true );
				SetBlendFunction( BlendFunc::SourceAlpha, BlendFunc::OneMinusSourceAlpha );

				// activate the default vertex array object
				glGenVertexArrays( 1, &defaultVao );
				glBindVertexArray( defaultVao );

				glGenBuffers( 1, &defaultPbo );
				glBindBuffer( GL_PIXEL_PACK_BUFFER, defaultPbo );
				glBufferData( GL_PIXEL_PACK_BUFFER,
					4 * state.window.width * state.window.height,
					NULL,
					GL_STREAM_COPY
				);
			}

			void Shutdown( void ) {
				glDeleteVertexArrays( 1, &defaultVao );
				glDeleteBuffers( 1, &defaultPbo );
			}

			void ToggleDepthTest( bool enabled ) {
				if ( enabled ) {
					glEnable( GL_DEPTH_TEST );
				}
				else {
					glDisable( GL_DEPTH_TEST );
				}
			}

			void SetDepthFunction( DepthFunc func ) {
				GLenum glFunc = GL_LEQUAL;

				switch( func ) {

				case DepthFunc::LessOrEqual: {
					glFunc = GL_LEQUAL;
				} break;

				default: {
					// ...
				} break;

				}

				glDepthFunc( glFunc );
			}

			void ToggleAlphaBlending( bool enabled ) {
				if ( enabled ) {
					glEnable( GL_BLEND );
				}
				else {
					glDisable( GL_BLEND );
				}
			}

			static GLenum GetGLBlendFunction( BlendFunc func ) {
				GLenum result = GL_ONE;

				switch( func ) {

				case BlendFunc::Zero: {
					result = GL_ZERO;
				} break;

				case BlendFunc::One: {
					result = GL_ONE;
				} break;

				case BlendFunc::SourceColour: {
					result = GL_SRC_COLOR;
				} break;

				case BlendFunc::OneMinusSourceColour: {
					result = GL_ONE_MINUS_SRC_COLOR;
				} break;

				case BlendFunc::DestColour: {
					result = GL_DST_COLOR;
				} break;

				case BlendFunc::OneMinusDestColour: {
					result = GL_ONE_MINUS_DST_COLOR;
				} break;

				case BlendFunc::SourceAlpha: {
					result = GL_SRC_ALPHA;
				} break;

				case BlendFunc::OneMinusSourceAlpha: {
					result = GL_ONE_MINUS_SRC_ALPHA;
				} break;

				case BlendFunc::DestAlpha: {
					result = GL_DST_ALPHA;
				} break;

				case BlendFunc::OneMinusDestAlpha: {
					result = GL_ONE_MINUS_DST_ALPHA;
				} break;

				case BlendFunc::ConstantColour: {
					result = GL_CONSTANT_COLOR;
				} break;

				case BlendFunc::OneMinusConstantColour: {
					result = GL_ONE_MINUS_CONSTANT_COLOR;
				} break;

				case BlendFunc::ConstantAlpha: {
					result = GL_CONSTANT_ALPHA;
				} break;

				case BlendFunc::OneMinusConstantAlpha: {
					result = GL_ONE_MINUS_CONSTANT_ALPHA;
				} break;

				case BlendFunc::SourceAlphaSaturate: {
					result = GL_SRC_ALPHA_SATURATE;
				} break;

				default: {
					result = GL_ONE;
				} break;

				}

				return result;
			}

			void SetBlendFunction( BlendFunc sourceFunc, BlendFunc destFunc ) {
				GLenum glSourceFunc = GetGLBlendFunction( sourceFunc );
				GLenum glDestFunc = GetGLBlendFunction( destFunc );

				glBlendFunc( glSourceFunc, glDestFunc );
			}

		} // namespace Backend

	} // namespace Renderer

} // namespace XS
