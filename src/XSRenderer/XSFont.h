#pragma once

namespace XS {

	namespace Renderer {

		struct font_s {
			font_s() {
				file = "";
				size = 24;
			}
			font_s( const char *name, uint16_t size );
			std::string		file;
			uint16_t		size;
			struct {
				uint32_t		texture;
				vector4			dimensions;
			} glyph[0xff];
		};

		// should not be instantiated
		class Font {
		private:
			Font(){}

		public:
			static void Init( void );
			static font_s *Register( const std::string &name, uint16_t size );
			static void Draw( const vector2 pos, const std::string &text, const font_s *font );
		};

	}

}
