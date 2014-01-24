#include "XSSystem/XSInclude.h"

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSError.h"
#include "XSRenderer/XSRenderCommand.h"
#include "XSRenderer/XSView.h"
#include "XSRenderer/XSBackend.h"
#include "XSRenderer/XSInternalFormat.h"
#include "XSRenderer/XSTexture.h"
#include "XSRenderer/XSRenderer.h"

namespace XS {

	namespace Renderer {

		void View::PreRender( void ) const {
			// set up 2d/3d perspective
			if ( is2D )
				Backend::Begin2D( width, height );
			else
				Backend::Begin3D( width, height );
		}

		void View::PostRender( void ) const {
			// ...
		}

		void View::Bind( void ) {
			SetView( this );
		}

		void View::Register( void ) {
			if ( !width || !height )
				throw( XSError( "Registered view with 0 width or 0 height" ) );

			RegisterView( this );
		}

	} // namespace Renderer

} // namespace XS
