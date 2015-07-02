#include <RakNet/RakPeerInterface.h>

#include <stack>

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSError.h"
#include "XSCommon/XSString.h"
#include "XSCommon/XSCvar.h"
#include "XSCommon/XSConsole.h"
#include "XSCommon/XSVector.h"
#include "XSCommon/XSTimer.h"
#include "XSCommon/XSGlobals.h"
#include "XSCommon/XSEvent.h"
#include "XSCommon/XSByteBuffer.h"
#include "XSClient/XSClient.h"
#include "XSClient/XSClientGame.h"
#include "XSClient/XSClientConsole.h"
#include "XSClient/XSMenuManager.h"
#include "XSClient/XSClientGameState.h"
#include "XSClient/XSCheckersBoard.h"
#include "XSInput/XSInput.h"
#include "XSInput/XSMouse.h"
#include "XSInput/XSKeys.h"
#include "XSNetwork/XSNetwork.h"
#include "XSRenderer/XSFont.h"
#include "XSRenderer/XSView.h"

namespace XS {

	namespace Client {

		uint64_t frameNum = 0u;

		// hud
		static Renderer::View *hudView = nullptr;

		// menus
		MenuManager *menu = nullptr;

		// console input
		ClientConsole *clientConsole = nullptr;

		void Cmd_ToggleConsole( const CommandContext * const context ) {
			if ( !clientConsole ) {
				throw XSError( "Tried to toggle client console without a valid instance" );
			}

			clientConsole->Toggle();
		}

		static void Cmd_Disconnect( const CommandContext * const context ) {
			Network::Disconnect();
		}

		static void Cmd_Connect( const CommandContext * const context ) {
			size_t numArgs = context->size();
			const char *hostname = (numArgs >= 1) ? (*context)[0].c_str() : nullptr;
			uint16_t port = (numArgs >= 2) ? atoi( (*context)[1].c_str() ) : 0u;
			Network::Connect( hostname, port );
		}

		static void Cmd_OpenMenu( const CommandContext * const context ) {
			menu->OpenMenu( (*context)[0].c_str() );
		}

		static void Cmd_ReloadMenu( const CommandContext * const context ) {
			delete menu;
			menu = new MenuManager();
			menu->RegisterMenu( "menus/settings.xmenu" );
		}

		static void RegisterCommands( void ) {
			Command::AddCommand( "disconnect", Cmd_Disconnect );
			Command::AddCommand( "connect", Cmd_Connect );
			Command::AddCommand( "openMenu", Cmd_OpenMenu );
			Command::AddCommand( "reloadMenu", Cmd_ReloadMenu );
		}

		bool MouseMotionEvent( const struct MouseMotionEvent &ev ) {
			if ( menu->isOpen ) {
				menu->MouseMotionEvent( ev );
				return true;
			}

			ClientGame::MouseMotionEvent( ev );

			return false;
		}

		bool MouseButtonEvent( const struct MouseButtonEvent &ev ) {
			if ( menu->isOpen ) {
				menu->MouseButtonEvent( ev );
				return true;
			}

			ClientGame::MouseButtonEvent( ev );

			return false;
		}

		void KeyboardEvent( const struct KeyboardEvent &ev ) {
			// hardcoded console short-circuit
			if ( clientConsole ) {
				if ( ev.down && ev.key == SDLK_BACKQUOTE ) {
					clientConsole->Toggle();
					return;
				}
				else if ( clientConsole->KeyboardEvent( ev ) ) {
					return;
				}
			}

			// let the menu consume key events
			if ( menu->isOpen ) {
				menu->KeyboardEvent( ev );
				return;
			}

			// fall through to gamecode
			keystate[ev.key] = ev.down;
			ExecuteBind( ev );
		}

		void Init( void ) {
			RegisterCommands();

			Network::Init();

			// client game module
			ClientGame::Init();

			// hud
			hudView = new Renderer::View( 0u, 0u, true );
			menu = new MenuManager();
			menu->RegisterMenu( "menus/settings.xmenu" );
		//	menu->OpenMenu( "settings" );

			// console
			clientConsole = new ClientConsole( &console );
		}

		void Shutdown( void ) {
			delete clientConsole;
			delete hudView;
		}

		void NetworkPump( void ) {
			if ( !Network::IsActive() ) {
				return;
			}

			// handle generic messages
			Network::Receive();
		}

		bool ReceivePacket( const RakNet::Packet *packet ) {
			switch ( packet->data[0] ) {

			case Network::ID_XS_SV2CL_PRINT: {
				uint8_t *buffer = packet->data + 1;
				size_t bufferLen = packet->length - 1;
				ByteBuffer bb( buffer, bufferLen );

				const char *msg = nullptr;
				bb.ReadString( &msg, nullptr );
				console.Print( PrintLevel::Normal,
					"%s",
					msg
				);

				console.Print( PrintLevel::Debug, "Receive ID_XS_SV2CL_PRINT: %s\n",
					msg
				);

				delete[] msg;
			} break;

			case Network::ID_XS_SV2CL_SET_PLAYER: {
				uint8_t *buffer = packet->data + 1;
				size_t bufferLen = packet->length - 1;
				ByteBuffer bb( buffer, bufferLen );
				const char *str = nullptr;
				bb.ReadString( &str, nullptr );
				if ( !String::Compare( str, "Red" ) ) {
					ClientGame::state.playing = true;
					ClientGame::state.currentPlayer = ClientGame::CheckersPiece::Colour::Red;
				}
				else if ( !String::Compare( str, "Black" ) ) {
					ClientGame::state.playing = true;
					ClientGame::state.currentPlayer = ClientGame::CheckersPiece::Colour::Black;
				}
				else {
					ClientGame::state.playing = false;
				}

				console.Print( PrintLevel::Debug, "Receive ID_XS_SV2CL_SET_PLAYER: %s\n",
					str
				);

				console.Print( PrintLevel::Normal,
					"Playing as %s (%i)\n",
					str,
					ClientGame::state.currentPlayer
				);
				delete[] str;
			} break;

			case Network::ID_XS_SV2CL_SET_CURRENT_PLAYER: {
				uint8_t *buffer = packet->data + 1;
				size_t bufferLen = packet->length - 1;
				ByteBuffer bb( buffer, bufferLen );
				uint8_t player;
				bb.ReadUInt8( &player );
				ClientGame::state.currentMove = static_cast<ClientGame::CheckersPiece::Colour>( player );

				console.Print( PrintLevel::Debug, "Receive ID_XS_SV2CL_SET_CURRENT_PLAYER: %s\n",
					(ClientGame::state.currentMove == ClientGame::CheckersPiece::Colour::Black)
						? "Black"
						: "Red"
				);
			} break;

			case Network::ID_XS_SV2CL_MOVE_PIECE: {
				uint8_t *buffer = packet->data + 1;
				size_t bufferLen = packet->length - 1;
				ByteBuffer bb( buffer, bufferLen );
				uint8_t offsetFrom, offsetTo;
				bb.ReadUInt8( &offsetFrom );
				bb.ReadUInt8( &offsetTo );

				console.Print( PrintLevel::Debug, "Receive ID_XS_SV2CL_MOVE_PIECE: %i to %i\n",
					offsetFrom,
					offsetTo
				);

				ClientGame::board->UpdatePiece( offsetFrom, offsetTo );
			} break;

			default: {
				return false;
			} break;

			}

			return true;
		}

		void RunFrame( real64_t dt ) {
			static real64_t stepTime = 0.0;
			frameNum++;

			// previousState = currentState;
			// integrate( currentState, stepTime, dt );
			stepTime += dt;

			// process server updates
			// simulate local entities
			//	predict entities whose state is not managed by the server, created by either the client or server
			//	e.g. client may create its own projectiles until the server overrides it
			// movement prediction (movement commands have been generated by input poll)
			ClientGame::RunFrame( dt );
		}

		// lazy initialise on first request per frame
		real64_t GetElapsedTime( TimerResolution resolution ) {
			static uint64_t lastFrame = 0u;
			static real64_t timeSec = 0.0;
			static real64_t timeMsec = 0.0;
			static real64_t timeUsec = 0.0;
			if ( lastFrame != frameNum ) {
				lastFrame = frameNum;
				timeUsec = Common::gameTimer->GetTiming();
				timeMsec = timeUsec * 0.001;
				timeSec = timeUsec * 0.000001;
			}

			switch ( resolution ) {

				case TimerResolution::Seconds: {
					return timeSec;
				} break;

				case TimerResolution::Milliseconds: {
					return timeMsec;
				} break;

				case TimerResolution::Microseconds: {
					return timeUsec;
				} break;

				default: {
					return 0.0;
				} break;
			}
		}

		static void DrawFPS( real64_t frametime, Renderer::Font *font ) {
			static const uint32_t numSamples = 64u;
			static real64_t samples[numSamples];
			static uint32_t index = 0;
			samples[index++] = frametime;
			if ( index >= numSamples ) {
				index = 0u;
			}
			real64_t avg = 0.0;
			for ( uint32_t i = 0; i < numSamples; i++ ) {
				avg += samples[i];
			}
			avg /= static_cast<real64_t>( numSamples );

			std::string fpsText = String::Format( "FPS:%.0f", 1000.0 / avg ).c_str();
			const uint16_t fpsTextSize = 16u;
			real32_t textWidth = 2.0f;
			for ( const char *p = fpsText.c_str(); *p; p++ ) {
				textWidth += font->GetGlyphWidth( *p, fpsTextSize );
			}

			vector2 pos(
				Renderer::state.window.width - textWidth,
				0.0f
			);
			font->Draw( pos, fpsText, fpsTextSize );
		}

		static void DrawMenus( Renderer::Font *font ) {
			if ( menu->isOpen ) {
				menu->PaintMenus();
				menu->DrawCursor();
			}
		}

		static void DrawDebugInfo( Renderer::Font *font ) {
			// ...
		}

		static void DrawHUD( real64_t frametime ) {
			if ( !hudView ) {
				return;
			}

			hudView->Bind();

			static Renderer::Font *font = nullptr;
			if ( !font ) {
				font = Renderer::Font::Register( "menu" );
			}

			DrawFPS( frametime, font );

			DrawMenus( font );

			DrawDebugInfo( font );
		}

		void DrawFrame( real64_t frametime ) {
			// draw game view
			ClientGame::DrawFrame( frametime );

			// draw HUD
			DrawHUD( frametime );

			// draw console
			clientConsole->Draw();
		}

	} // namespace Client

} // namespace XS
