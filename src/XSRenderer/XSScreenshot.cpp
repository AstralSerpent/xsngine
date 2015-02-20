#include "XSCommon/XSCommon.h"
#include "XSCommon/XSCvar.h"
#include "XSCommon/XSCommand.h"
#include "XSRenderer/XSRenderer.h"
#include "XSRenderer/XSBackend.h"
#include "XSRenderer/XSImagePNG.h"
#include "XSRenderer/XSRenderCommand.h"

namespace XS {

	namespace Renderer {

		namespace Backend {

			static const size_t numScreenshotsPerFrame = 4u;

			static const char *GetScreenshotName( void ) {
				static char timestamp[numScreenshotsPerFrame][XS_MAX_FILENAME];
				time_t rawtime;
				time( &rawtime );
				static uint32_t index = 0u;
				char *p = timestamp[index];
				//TODO: stop name clash?
				strftime( p, sizeof(timestamp[index]), "%Y-%m-%d_%H-%M-%S.png", localtime( &rawtime ) );
				index++;
				index &= numScreenshotsPerFrame;
				return p;
			}

			void Cmd_Screenshot( const CommandContext * const context ) {
				glBindBuffer( GL_PIXEL_PACK_BUFFER, defaultPbo );
				glReadPixels( 0, 0, state.window.width, state.window.height, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
				GLsync sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );

				RenderCommand cmd( CommandType::Screenshot );
				cmd.screenshot.name = GetScreenshotName();
				cmd.screenshot.width = state.window.width;
				cmd.screenshot.height = state.window.height;
				cmd.screenshot.pbo = defaultPbo;
				cmd.screenshot.sync = sync;

				//TODO: wait until next frame
				cmd.Execute();
			}

		} // namespace Backend

	} // namespace Renderer

} // namespace XS
