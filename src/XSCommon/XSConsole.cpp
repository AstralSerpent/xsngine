#include <iostream>

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSConsole.h"
#include "XSCommon/XSLogger.h"
#include "XSCommon/XSCvar.h"
#include "XSCommon/XSString.h"
#include "XSCommon/XSColours.h"
#include "XSRenderer/XSRenderer.h"
#include "XSRenderer/XSImagePNG.h"
#include "XSRenderer/XSTexture.h"
#include "XSRenderer/XSShaderProgram.h"
#include "XSRenderer/XSView.h"
#include "XSRenderer/XSVertexAttributes.h"

#if defined(XS_OS_WINDOWS) && defined(_DEBUG)
	#define WIN32_LEAN_AND_MEAN
	#define VC_EXTRALEAN
	#ifndef NOMINMAX
		#define NOMINMAX /* Don't define min() and max() */
	#endif
	#include <Windows.h>
#endif

namespace XS {

	namespace Console {

		static unsigned int indentation = 0;
		static bool visible = false;

		static std::vector<char*> consoleText;
		static int scrollAmount = 0;
		static unsigned int lineLength = 1280 / 12; // changes at runtime based on window width
		static const uint32_t characterSize = 16;
		static const unsigned int lineCount = 24;

		static Renderer::Texture *fontTexture;
		static Renderer::Material *fontMaterial;
		static Renderer::ShaderProgram *fontShader;
		static Renderer::View *view;

		static Logger consoleLog( "console.log" );

		static void CreateFontMaterial( Renderer::Texture& fontTexture );

		static Cvar *con_fontSize;

		void Init( void ) {
			con_fontSize = Cvar::Create( "con_fontSize", "12.0", "Size of the console font", CVAR_ARCHIVE );

			// register the console view
			view = new Renderer::View( Cvar::Get( "vid_width" )->GetInt(), Cvar::Get( "vid_height" )->GetInt(), true );

			byte *fontTextureData = Renderer::LoadPNG( "fonts/console.png" );
			fontTexture = new Renderer::Texture( 16*characterSize, 16*characterSize, Renderer::InternalFormat::RGBA8,
				fontTextureData );
			delete[] fontTextureData;

			CreateFontMaterial( *fontTexture );
		}

		void Close( void ) {
			for ( const auto &it : consoleText ) {
				delete it;
			}
			consoleText.clear();

			delete fontMaterial;
			delete fontShader;
			delete fontTexture;
			delete view;
		}

		static void CreateFontMaterial( Renderer::Texture& fontTexture ) {
			static const Renderer::VertexAttribute attributes[] = {
				{ 0, "in_Position" },
				{ 1, "in_TexCoord" },
				{ 2, "in_Color" }
			};

			Renderer::Material::SamplerBinding samplerBinding;
			samplerBinding.unit = 0;
			samplerBinding.texture = &fontTexture;

			fontShader = new Renderer::ShaderProgram( "text", "text", attributes,
				sizeof(attributes) / sizeof(attributes[0]) );

			fontMaterial = new Renderer::Material();
			fontMaterial->samplerBindings.push_back( samplerBinding );
			fontMaterial->shaderProgram = fontShader;
		}

		static void Append( const char *text, bool multiLine ) {
			size_t len = strlen( text );
			size_t accumLength = 0;
			size_t i = 0;
			std::string tmp = text;

			if ( scrollAmount < 0 ) {
				if ( consoleText.size() >= lineCount ) {
					scrollAmount = std::max<int>( scrollAmount-1, (signed)(consoleText.size()-lineCount) * -1 );
				}
				else {
					scrollAmount = std::max<int>( scrollAmount-1, 0U );
				}
			}

			char *thisLine = new char[len+1];
			for ( i=0; i<len; i++ ) {
				char *p = (char *)&text[i];

				if ( !IsColourString( p ) && (i>0 && !IsColourString( p-1 )) ) {
					accumLength++;
				}

				if ( accumLength > lineLength && (i>0 && !IsColourString( p-1 )) ) {
					char lastColour = COLOUR_GREEN;
					size_t j = i;
					size_t savedOffset = i;

					// attempt to back-track, find a space (' ') within X characters
					while ( text[i] != ' ' ) {
						if ( i <= 0 || i < savedOffset-16 ) {
							i = j = savedOffset;
							break;
						}
						i--;
						j--;
					}

					String::Copy( thisLine, text, i + 1 );
					consoleText.push_back( thisLine );

					// find the last colour used
					for ( j=i; j>0; j-- ) {
						if ( IsColourString( &text[j] ) ) {
							lastColour = text[j+1];
							break;
						}
					}

					char *nextLine = new char[i + strlen( "^x" ) + 1];
					String::FormatBuffer( nextLine, i + strlen( "^x" ), "%c%c%s", COLOUR_ESCAPE, lastColour, text + i );
					Append( nextLine, true );
					return;
				}
			}

			// didn't split the line, copy the full contents
			String::Copy( thisLine, text, i );
			consoleText.push_back( thisLine );
		}

		void Print( const char *fmt, ... ) {
			size_t size = 128;
			std::string str;
			va_list ap;

			while ( 1 ) {
				str.resize( size );

				va_start( ap, fmt );
				int n = vsnprintf( (char *)str.c_str(), size, fmt, ap );
				va_end( ap );

				if ( n > -1 && n < (signed)size ) {
					str.resize( n );
					break;
				}
				if ( n > -1 ) {
					size = n + 1;
				}
				else {
					size *= 2;
				}
			}

			//FIXME: care about printing twice on same line
			std::string finalOut = "";
			for ( unsigned int i=0; i<indentation; i++ ) {
				finalOut += "  ";
			}
			finalOut += str;

			//TODO: strip colours?
			std::cout << finalOut;
			Append( finalOut.c_str(), false );
			consoleLog.Print( finalOut.c_str() );

		#if defined(XS_OS_WINDOWS) && defined(_DEBUG)
			if ( !finalOut.empty() ) {
				OutputDebugString( finalOut.c_str() );
			}
		#endif
		}

		static void AdjustWidth( void ) {
			Cvar *cv = Cvar::Get( "vid_width" );
			if ( cv ) {
				lineLength = cv->GetInt() / con_fontSize->GetFloat();
			}
		}

		static void DrawChar( float x, float y, char c ) {
			int row, col;
			float frow, fcol;
			const float size = 1.0f / characterSize;

			if ( c == ' ' || c == '\n' || c == '\r' ) {
				return;
			}

			// assumes 16x16
			// sqrt( 256 ) = 16
			row = c>>4;
			col = c&15;

			frow = row*size;
			fcol = col*size;

			Renderer::DrawQuad( x, y, con_fontSize->GetFloat(), con_fontSize->GetFloat(), fcol, frow, fcol+size,
				frow+size, *fontMaterial );
		}

		void Display( void ) {
			if ( !visible ) {
				return;
			}

			if ( consoleText.size() == 0 ) {
				return;
			}

			view->Bind();

			AdjustWidth();

			size_t i = consoleText.size() - std::min( lineCount, (unsigned)consoleText.size() );
			i += scrollAmount;

			for ( size_t line=0; line<lineCount && line<consoleText.size(); i++, line++ ) {
				auto it = consoleText.at(i);
				size_t len = strlen( it );
				for ( size_t c=0; c<len; c++ ) {
					DrawChar( (float)(c*con_fontSize->GetFloat()), (float)(line*con_fontSize->GetFloat()), it[c] );
				}
			}
		}

		void Toggle( const commandContext_t * const context ) {
			(void)context;
			visible = !visible;
		}

		void Indent( int level ) {
			indentation += level;
		}

	} // namespace Console

} // namespace XS
