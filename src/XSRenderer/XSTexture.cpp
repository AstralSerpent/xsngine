#include "XSSystem/XSInclude.h"
#include "XSSystem/XSPlatform.h"

#include "GLee/GLee.h"
#include "SDL2/SDL.h"

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSFile.h"
#include "XSCommon/XSCvar.h"
#include "XSCommon/XSString.h"
#include "XSRenderer/XSInternalFormat.h"
#include "XSRenderer/XSTexture.h"
#include "XSRenderer/XSRenderCommand.h"
#include "XSRenderer/XSView.h"
#include "XSRenderer/XSRenderer.h"

namespace XS {

	namespace Renderer {

		static Cvar *r_textureAnisotropy;
		static Cvar *r_textureAnisotropyMax;
		static Cvar *r_textureFilter;

		static bool anisotropy;
		static float maxAnisotropy;

		static const struct {
			const char *name;
			GLint min, mag;
		} filterTable[] = {
			{ "GL_NEAREST",					GL_NEAREST,					GL_NEAREST },
			{ "GL_LINEAR",					GL_LINEAR,					GL_LINEAR },
			{ "GL_NEAREST_MIPMAP_NEAREST",	GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST },
			{ "GL_LINEAR_MIPMAP_NEAREST",	GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR },
			{ "GL_NEAREST_MIPMAP_LINEAR",	GL_NEAREST_MIPMAP_LINEAR,	GL_NEAREST },
			{ "GL_LINEAR_MIPMAP_LINEAR",	GL_LINEAR_MIPMAP_LINEAR,	GL_LINEAR }
		};
		static const size_t numFilters = ARRAY_LEN( filterTable );
		static size_t GetTextureFilter( const char *string ) {
			for ( size_t filter=0; filter<numFilters; filter++ ) {
				if ( !String::Compare( string, filterTable[filter].name ) )
					return filter;
			}
			return 0;
		}

		void Texture::Init( void ) {
			r_textureAnisotropy = Cvar::Create( "r_textureAnisotropy", "1", CVAR_ARCHIVE );
			r_textureAnisotropyMax = Cvar::Create( "r_textureAnisotropyMax", "16.0", CVAR_ARCHIVE );
			r_textureFilter = Cvar::Create( "r_textureFilter", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );

			// get anisotropic filtering settings
			if ( SDL_GL_ExtensionSupported( "GL_EXT_texture_filter_anisotropic" ) )
				anisotropy = true;
			glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
		}

		Texture::Texture( unsigned int width, unsigned int height, internalFormat_t internalFormat, byte *data ) {
			size_t filterMode = GetTextureFilter( r_textureFilter->GetCString() );

			glGenTextures( 1, &id );
			if ( !id )
				throw( "Failed to create blank texture" );

			this->width		= width;
			this->height	= height;

			// fp16 textures don't play nicely with filtering (performance, support)
			//	if ( internalFormat == IF_RGBA16F )
			//		filterMode = GL_NEAREST;

			glBindTexture( GL_TEXTURE_2D, id );

			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

			if ( anisotropy && r_textureAnisotropy->GetBool() )
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
					std::min( r_textureAnisotropyMax->GetFloat(), maxAnisotropy ) );

			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterTable[filterMode].min );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterTable[filterMode].mag );

			glTexImage2D( GL_TEXTURE_2D, 0, GetGLInternalFormat( internalFormat ), width, height, 0,
				GetGLFormat( internalFormat ), GetDataTypeForFormat( internalFormat ), data );
			glBindTexture( GL_TEXTURE_2D, 0 );
 		}

		Texture::~Texture() {
			glDeleteTextures( 1, &id );
		}

	} // namespace Renderer

} // namespace XS
