#include "XSSystem/XSInclude.h"

#include "SDL2/SDL_keycode.h"

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSCommand.h"
#include "XSCommon/XSString.h"
#include "XSCommon/XSFile.h"
#include "XSCommon/XSCvar.h"
#include "XSCommon/XSConsole.h"
#include "XSClient/XSKeys.h"

namespace XS {

	namespace Command {

		// commands
		static std::unordered_map<std::string, commandFunc_t> commands;

		// command buffer
		static std::vector<std::string> buffer;


		static void Cmd_SetCvar( const commandContext_t *context ) {
			if ( context->args.size() < 2 ) {
				Console::Print( "\"set\" failed. Must specify a cvar and value\n" );
				return;
			}

			Cvar *cv = Cvar::Get( context->args[0] );

			size_t size = context->args.size();
			std::string value;
			for ( size_t i=1; i<size; i++ ) {
				value += context->args[i];
				if ( i != size-1 )
					value += " ";
			}
			cv->Set( value );
		}

		static void Cmd_ToggleCvar( const commandContext_t *context ) {
			Cvar *cv = Cvar::Get( context->args[0] );

			cv->Set( !cv->GetBool() );
		}

		void Init( void ) {
			AddCommand( "set", Cmd_SetCvar );
			AddCommand( "toggle", Cmd_ToggleCvar );
			AddCommand( "toggleconsole", Console::Toggle );
			AddCommand( "bind", Client::Cmd_SetBind );
		}

		// command buffer

		void Append( const char *cmd ) {
			buffer.push_back( cmd );
		}

		void ExecuteBuffer( void ) {
			for ( auto it = buffer.begin(); it != buffer.end(); ++it ) {
				commandContext_t context;

				// tokenise the arguments
				const char delim = ' ';
				size_t start = it->find( delim );
				std::string name = it->substr( 0, start );

				if ( start != std::string::npos && start != it->size()-1 ) {
					context.args = String::Split( &(*it)[start+1], ' ' );

					// strip any quotes around the arguments
					for ( auto tok = context.args.begin(); tok != context.args.end(); ++tok )
						tok->erase( std::remove( tok->begin(), tok->end(), '"' ), tok->end() );

				//	if ( context.args.size() == 0 )
				//		continue;
				}

				commandFunc_t func = commands[name];
				if ( func ) {
					func( &context );
					continue;
				}
			}
			buffer.clear();
		}

		// commands

		bool AddCommand( const char *name, commandFunc_t cmd ) {
			commandFunc_t &func = commands[name];
			if ( func )
				return false;

			func = cmd;
			return true;
		}

	} // namespace Command

} // namespace XS
