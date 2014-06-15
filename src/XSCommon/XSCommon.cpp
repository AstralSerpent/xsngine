#include <SDL2/SDL_timer.h>
#include <SDL2/SDL.h>

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSCommand.h"
#include "XSCommon/XSConsole.h"
#include "XSCommon/XSCvar.h"
#include "XSCommon/XSEvent.h"
#include "XSCommon/XSFile.h"
#include "XSCommon/XSTimer.h"
#include "XSCommon/XSString.h"
#include "XSClient/XSClient.h"
#include "XSClient/XSInput.h"
#include "XSClient/XSKeys.h"
#include "XSRenderer/XSRenderer.h"


namespace XS {

	namespace Common {

		#define DEFAULT_CONFIG			"cfg/xsn.cfg"
		#define DEFAULT_CONFIG_SERVER	"cfg/xsn_server.cfg"

		Cvar *com_dedicated, *com_developer;
		static Cvar *com_framerate, *r_framerate;

		static void RegisterCvars( void ) {
			Cvar::Create( "com_date", __DATE__, "Compilation date", CVAR_READONLY );
			com_dedicated = Cvar::Create( "com_dedicated", "0", "Running a dedicated server", CVAR_INIT );
#ifdef _DEBUG
			com_developer = Cvar::Create( "com_developer", "1", "Developer mode", CVAR_NONE );
#else
			com_developer = Cvar::Create( "com_developer", "0", "Developer mode", CVAR_NONE );
#endif
			com_framerate = Cvar::Create( "com_framerate", "50", "Game tick rate", CVAR_NONE );
			r_framerate = Cvar::Create( "r_framerate", "120", "Render framerate", CVAR_NONE );
		}

		static void ParseCommandLine( int argc, char **argv ) {
			std::string commandLine;

			// concatenate argv[] to commandLine
			for ( int i=1; i<argc; i++ ) {
				const bool containsSpaces = strchr( argv[i], ' ' ) != NULL;

				if ( containsSpaces ) {
					commandLine += "\"";
				}

				commandLine += argv[i];

				if ( containsSpaces ) {
					commandLine += "\"";
				}

				commandLine += " ";
			}

			// split up commandLine by +
			//	+set x y +set herp derp
			// becomes
			//	set x y
			//	set herp derp
			// then append it to the command buffer
			const char delimiter = '+';
			const size_t start = commandLine.find( delimiter );
			if ( start == std::string::npos ) {
				return;
			}
			Command::Append( &commandLine[start + 1], delimiter );

		#ifdef _DEBUG
			Console::Print( "Startup parameters:\n" );
			Indent indent(1);
			for ( const auto &arg : String::Split( &commandLine[start + 1], delimiter ) ) {
				Console::Print( "%s\n", arg.c_str() );
			}
		#endif
		}

		static void LoadConfig( void ) {
			const char *cfg = com_dedicated->GetBool() ? DEFAULT_CONFIG_SERVER : DEFAULT_CONFIG;
			const File f( cfg, FileMode::READ );

			if ( f.open ) {
				char *buffer = new char[f.length];
					f.Read( (byte *)buffer );

					Command::ExecuteBuffer(); // flush buffer before we issue commands
					char *current = strtok( buffer, "\n" );
					while ( current ) {
						Command::Append( current );
						Command::ExecuteBuffer();
						current = strtok( NULL, "\n" );
					}
				delete[] buffer;
			}
		}

		static void WriteConfig( const char *cfg = NULL ) {
			std::string str = "";
			Cvar::WriteCvars( str );
			if ( !com_dedicated->GetBool() ) {
				Client::WriteBinds( str );
			}

			// default config if none specified
			if ( !cfg ) {
				cfg = com_dedicated->GetBool() ? DEFAULT_CONFIG_SERVER : DEFAULT_CONFIG;
			}

			const File f( cfg, FileMode::WRITE );
			if ( !f.open ) {
				Console::Print( "Failed to write config! (%s)\n", cfg );
				return;
			}

			f.AppendString( str.c_str() );
		}

		static void Cmd_WriteConfig( const commandContext_t * const context ) {
			const char *cfg = NULL;
			if ( context->size() ) {
				 cfg = (*context)[0].c_str();
			 }

			WriteConfig( cfg );
		}

	} // namespace Common

} // namespace XS

static XS::Timer timer;

int main( int argc, char **argv ) {
	try {
		// critical initialisation
		XS::File::Init();
		XS::Common::RegisterCvars();
		XS::Command::Init(); // register commands like exec, vstr
		XS::Command::AddCommand( "writeconfig", XS::Common::Cmd_WriteConfig );
		XS::Common::ParseCommandLine( argc, argv );

		// execute the command line args, so config can be loaded from an overridden base path
		XS::Command::ExecuteBuffer();
		XS::File::SetBasePath();
		XS::Common::LoadConfig();

		//
		// DO NOT LOAD MEDIA BEFORE THIS POINT
		//

		XS::Console::Print( WINDOW_TITLE " (" XSTR( ARCH_WIDTH ) " bits) built on " __DATE__ " [git " REVISION "]\n" );

		if ( !XS::Common::com_dedicated->GetBool() ) {
			XS::Renderer::Init();
		}

		XS::Console::Init();

		XS::Event::Init();
	//	XS::Network::Init();

		if ( XS::Common::com_developer->GetBool() ) {
			double t = timer.GetTiming( true, XS::Timer::Resolution::MILLISECONDS );
			XS::Console::Print( "Init time: %.0f milliseconds\n", (float)t );
		}

		if ( !XS::Common::com_dedicated->GetBool() ) {
			XS::Client::Init();
		}

		// post-init stuff
		XS::Cvar::initialised = true;

		// frame
		XS::Timer gameTimer;
		while ( 1 ) {
			static double currentTime = gameTimer.GetTiming( false, XS::Timer::Resolution::MILLISECONDS );
			static double accumulator = 0.0;

			// calculate delta time for integrating this frame
			const double newTime = gameTimer.GetTiming( false, XS::Timer::Resolution::MILLISECONDS );
			const double dt = 1000.0 / XS::Common::com_framerate->GetDouble();
			const double frameTime = newTime - currentTime;
			currentTime = newTime;

			double sliceMsec = frameTime;
			if ( sliceMsec > 250.0 ) {
				sliceMsec = 250.0; // avoid spiral of death, maximum 250mspf
			}
			accumulator += sliceMsec;

			// input
			if ( !XS::Common::com_dedicated->GetBool() ) {
				XS::Client::input.Poll();
			}

			// event pump
			XS::Event::Pump();
			XS::Command::ExecuteBuffer();

			// server frame, then network (snapshot)
		//	XS::Server::RunFrame( dt );
		//	XS::Server::NetworkPump();

			// event pump
			XS::Event::Pump();
			XS::Command::ExecuteBuffer();

			// outgoing network (client command), then client frame
			XS::Client::NetworkPump();
			while ( accumulator >= dt ) {
				XS::Client::RunFrame( dt );
				accumulator -= dt;
			}

			// alpha = accumulator / dt;
			// lerp( previousState, alpha, currentState )
			XS::Client::DrawFrame( frameTime );
			XS::Renderer::Update( /*state*/ );

			const double renderMsec = 1000.0 / XS::Common::r_framerate->GetDouble();
			if ( frameTime < renderMsec ) {
				SDL_Delay( (uint32_t)(renderMsec - frameTime) );
			}
		}
	}
	catch( const XS::XSError &e ) {
		bool developer = XS::Common::com_developer->GetBool();

		XS::Console::Print( "\n*** XSNGINE Shutdown: %s\n\n"
			"Cleaning up...\n", e.what() );

		if ( developer ) {
			double t = timer.GetTiming( true, XS::Timer::Resolution::MILLISECONDS );
			XS::Console::Print( "Run time: %.0f milliseconds\n", (float)t );
		}

		// indent the console for this scope
		{
			XS::Indent indent(1);
			XS::Client::Shutdown();
			XS::Renderer::Shutdown();

			XS::Common::WriteConfig();
			XS::Cvar::Clean();
		}

		if ( developer ) {
			double t = timer.GetTiming( false, XS::Timer::Resolution::MILLISECONDS );
			XS::Console::Print( "Shutdown time: %.0f milliseconds\n\n\n", (float)t );
		}

		XS::Console::Close();

		return EXIT_SUCCESS;
	}

	// never reached
	return EXIT_FAILURE;
}
