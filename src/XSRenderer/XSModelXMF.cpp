#include <inttypes.h>

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSConsole.h"
#include "XSCommon/XSFile.h"
#include "XSCommon/XSString.h"
#include "XSCommon/XSTokenParser.h"
#include "XSCommon/XSVector.h"
#include "XSRenderer/XSModelXMF.h"
#include "XSRenderer/XSInternalFormat.h"
#include "XSRenderer/XSMesh.h"
#include "XSRenderer/XSMaterial.h"
#include "XSRenderer/XSShaderProgram.h"
#include "XSRenderer/XSTexture.h"
#include "XSRenderer/XSVertexAttributes.h"
#include "XSRenderer/XSImagePNG.h"

namespace XS {

	namespace Renderer {

		uint32_t XMF::version = 2u;

		void XMF::Process( Mesh *mesh ) {
			// create texture
			if ( !mesh->texture ) {
				uint8_t *texture = new uint8_t[128 * 128 * 4];
				std::memset( texture, 255, sizeof(*texture) );
				mesh->texture = new Texture( 128, 128, InternalFormat::RGBA8, texture );
				delete[] texture;
			}

			// create shader program
			static const VertexAttribute attributes[] = {
				{ 0, "in_position" },
				{ 1, "in_normal" },
				{ 2, "in_texCoord" },
			};
			mesh->shader = new ShaderProgram( "model", "model", attributes, ARRAY_LEN( attributes ) );

			// create material
			mesh->material = new Material();
			Material::SamplerBinding samplerBinding( mesh->texture );
			mesh->material->samplerBindings.push_back( samplerBinding );
			mesh->material->shaderProgram = mesh->shader;

			mesh->Upload();
			meshes.push_back( mesh );
		}

		bool XMF::LoadMeshes( void ) {
			const File f( modelPath.c_str() );
			if ( !f.open ) {
				console.Print( PrintLevel::Normal, "%s failed to open file '%s'\n", XS_FUNCTION, modelPath.c_str() );
				return false;
			}
			char *buffer = new char[f.length];
			f.Read( reinterpret_cast<uint8_t *>( buffer ) );
			TokenParser parser( buffer );

			// check the version identifier
			const char *token = parser.ParseToken();
			if ( !String::CompareCase( token, "version" ) ) {
				uint32_t ui = 0u;
				parser.ParseUInt32( &ui );
				if ( ui != XMF::version ) {
					console.Print( PrintLevel::Normal,
						"%s invalid XMF file '%s' wrong version identifier (%u != %u)\n",
						XS_FUNCTION,
						modelPath.c_str(),
						ui,
						XMF::version
					);
					return false;
				}
			}
			else {
				console.Print( PrintLevel::Normal,
					"%s invalid XMF file '%s' missing version identifier!\n",
					XS_FUNCTION,
					modelPath.c_str()
				);
				return false;
			}

			// should be a valid XMF, try parsing it out
			Mesh *mesh = nullptr;
			while ( true ) {
				const char *token = parser.ParseToken();
				if ( !token[0] ) {
					// end of file
					if ( mesh ) {
						Process( mesh );
					}
					mesh = nullptr;
					break;
				}
				else if ( !String::CompareCase( token, "#" ) ) {
					// # is used as a comment
					parser.SkipLine();
				}
				else if ( !String::CompareCase( token, "o" ) ) {
					// new object/mesh
					if ( mesh && mesh->vertices.size() > 0 ) {
						Process( mesh );
					}
					mesh = new Mesh();
					console.Print( PrintLevel::Debug,
						"%s loading new mesh for '%s' at 0x%" PRIXPTR "\n",
						XS_FUNCTION,
						modelPath.c_str(),
						mesh
					);

					const char *str = nullptr;
					parser.ParseString( &str );
					if ( str ) {
						uint32_t width, height;
						uint8_t *textureContents = LoadPNG( str, &width, &height );
						mesh->texture = new Texture( width, height, InternalFormat::RGBA8, textureContents );
						delete textureContents;
					}

					parser.SkipLine();
				}
				else if ( !String::CompareCase( token, "v" ) ) {
					// new vertex
					if ( !mesh ) {
						// something went wrong, we can't parse vertices out if we don't know what mesh they're
						//	related to
						console.Print( PrintLevel::Normal,
							"%s tried to parse vertices for '%s' without specifying a mesh!\n",
							XS_FUNCTION,
							modelPath.c_str()
						);
						break;
					}

					vector3 vertex;
					for ( int i = 0; i < 3; i++ ) {
						parser.ParseReal32( &vertex.raw[i] );
					}
					mesh->vertices.push_back( vertex );
					parser.SkipLine();
				}
				else if ( !String::CompareCase( token, "uv" ) ) {
					// new texture coordinates
					if ( !mesh ) {
						// something went wrong, we can't parse texture coordinates out if we don't know what mesh
						//	they're related to
						console.Print( PrintLevel::Normal,
							"%s tried to parse UVs for '%s' without specifying a mesh!\n",
							XS_FUNCTION,
							modelPath.c_str()
						);
						break;
					}

					vector2 uv;
					for ( int i = 0; i < 2; i++ ) {
						parser.ParseReal32( &uv.raw[i] );
					}
					real32_t tmp = uv.x;
					uv.x = uv.y;
					uv.y = tmp;
					mesh->UVs.push_back( uv );
					parser.SkipLine();
				}
				else if ( !String::CompareCase( token, "f" ) ) {
					if ( !mesh ) {
						// something went wrong, we can't parse faces out if we don't know what mesh they're related
						//	to
						console.Print( PrintLevel::Normal,
							"%s tried to parse face for '%s' without specifying a mesh!\n",
							XS_FUNCTION,
							modelPath.c_str()
						);
						break;
					}

					// vertex indices
					uint32_t a = 0, b = 0, c = 0;
					const char *str = nullptr;
					parser.ParseString( &str );
					if ( str ) {
						sscanf( str, "%i/%i/%i", &a, &b, &c );
						mesh->indices.push_back( a );
						mesh->indices.push_back( b );
						mesh->indices.push_back( c );
					}

					parser.SkipLine();
				}
				else {
					parser.SkipLine();
				}
			}

			delete[] buffer;
			return true;
		}

	} // namespace Renderer

} // namespace XS
