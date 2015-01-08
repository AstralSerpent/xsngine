#include <SDL2/SDL.h>

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSVector.h"
#include "XSCommon/XSConsole.h"
#include "XSRenderer/XSRenderer.h"
#include "XSRenderer/XSBuffer.h"
#include "XSRenderer/XSRenderCommand.h"
#include "XSRenderer/XSMaterial.h"
#include "XSRenderer/XSMesh.h"
#include "XSRenderer/XSModel.h"
#include "XSRenderer/XSImagePNG.h"
#include "XSRenderer/XSMaterial.h"
#include "XSRenderer/XSVertexAttributes.h"
#include "XSRenderer/XSTexture.h"
#include "XSRenderer/XSShaderProgram.h"

namespace XS {

	namespace Renderer {

		// render commands are actually backend
		using namespace Backend;

		static Buffer *quadsVertexBuffer;
		static Buffer *quadsIndexBuffer;

		static ShaderProgram *quadProgram = nullptr;
		static Material *quadMaterial = nullptr;
		static Texture *quadTexture = nullptr;

		void RenderCommand::Init( void ) {
			const unsigned short quadIndices[6] = { 0, 2, 1, 1, 2, 3 };

			quadsVertexBuffer = new Buffer( Buffer::Type::VERTEX, nullptr, 144 * sizeof(float) );
			quadsIndexBuffer = new Buffer( Buffer::Type::INDEX, quadIndices, sizeof(quadIndices) );

			// create null quad material
			static const VertexAttribute attributes[] = {
				{ 0, "in_Position" },
				{ 1, "in_TexCoord" },
				{ 2, "in_Color" }
			};

			quadProgram = new ShaderProgram( "quad", "quad", attributes, ARRAY_LEN( attributes ) );

			// create texture
			static const size_t numChannels = 4;
			static const size_t textureSize = 1;
			uint8_t textureBuffer[textureSize * textureSize * numChannels] = {};
			std::memset( textureBuffer, 0xFF, sizeof( textureBuffer ) );
			quadTexture = new Texture( textureSize, textureSize, InternalFormat::RGBA8, textureBuffer );

			// create material
			quadMaterial = new Material();
			Material::SamplerBinding samplerBinding;
			samplerBinding.unit = 0;
			samplerBinding.texture = quadTexture;
			quadMaterial->samplerBindings.push_back( samplerBinding );
			quadMaterial->shaderProgram = quadProgram;
		}

		void RenderCommand::Shutdown( void ) {
			delete quadsVertexBuffer;
			delete quadsIndexBuffer;
		}

		static void DrawQuad( const rcDrawQuad_t *quad ) {
			if ( quad->material ) {
				quad->material->Bind();
			}
			else {
				quadMaterial->Bind();
			}

			vector2 vertices[4];
			vector2 texcoords[4];
			vector4 colour( 1.0f, 1.0f, 1.0f, 1.0f );
			if ( quad->colour ) {
				colour = *quad->colour;
			}

			// Top-left
			vertices[0].x = quad->x;
			vertices[0].y = quad->y;
			texcoords[0].x = quad->s1;
			texcoords[0].y = quad->t1;

			// Top-right
			vertices[1].x = quad->x + quad->w;
			vertices[1].y = quad->y;
			texcoords[1].x = quad->s2;
			texcoords[1].y = quad->t1;

			// Bottom-left
			vertices[2].x = quad->x;
			vertices[2].y = quad->y + quad->h;
			texcoords[2].x = quad->s1;
			texcoords[2].y = quad->t2;

			// Bottom-right
			vertices[3].x = quad->x + quad->w;
			vertices[3].y = quad->y + quad->h;
			texcoords[3].x = quad->s2;
			texcoords[3].y = quad->t2;

			float *vertexBuffer = static_cast<float *>( quadsVertexBuffer->Map() );
			{
				std::memcpy( vertexBuffer, vertices, sizeof(vertices) );
				vertexBuffer += 8;

				std::memcpy( vertexBuffer, texcoords, sizeof(texcoords) );
				vertexBuffer += 8;

				for ( int i = 0; i < 4; i++ ) {
					*vertexBuffer++ = colour._raw[i];
				}
			}
			quadsVertexBuffer->Unmap();

			quadsIndexBuffer->Bind();

			glEnableVertexAttribArray( 0 );
			glEnableVertexAttribArray( 1 );
			glEnableVertexAttribArray( 2 );
				intptr_t offset = 0;

				glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, 0 );
				offset += sizeof(vertices);

				glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid *>( offset ) );
				offset += sizeof(texcoords);

				glVertexAttribPointer( 2, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid *>( offset ) );
				offset += sizeof(colour);

				glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0 );
			glDisableVertexAttribArray( 2 );
			glDisableVertexAttribArray( 1 );
			glDisableVertexAttribArray( 0 );
		}

		static void DrawMesh( const Mesh *mesh ) {
			SDL_assert( mesh->material && "DrawMesh with invalid material" );

			mesh->material->Bind();

			// bind the vertex/normal/uv buffers
			if ( mesh->vertexBuffer ) {
				mesh->vertexBuffer->Bind();
				glEnableVertexAttribArray( 0 );
				glEnableVertexAttribArray( 1 );
				glEnableVertexAttribArray( 2 );

				size_t offset = 0u;

				glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid *>( offset ) );
				offset += mesh->vertices.size() * sizeof(vector3);

				glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid *>( offset ) );
				offset += mesh->normals.size() * sizeof(vector3);

				glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid *>( offset ) );
				offset += mesh->UVs.size() * sizeof(vector2);
			}

			// issue the draw command
			if ( mesh->indexBuffer ) {
				mesh->indexBuffer->Bind();
				int size;
				glGetBufferParameteriv( GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size );
				glDrawElements( GL_TRIANGLES, size / sizeof(uint16_t), GL_UNSIGNED_SHORT, 0 );
			}
			else {
				glDrawArrays( GL_TRIANGLES, 0, mesh->vertices.size() );
			}

			// clean up state
			if ( mesh->vertexBuffer ) {
				glDisableVertexAttribArray( 2 );
				glDisableVertexAttribArray( 1 );
				glDisableVertexAttribArray( 0 );
			}
		}

		static void DrawModel( const rcDrawModel_t *model ) {
			for ( const auto &mesh : model->model->meshes ) {
				DrawMesh( mesh );
			}
		}

		static void Screenshot( const rcScreenshot_t *ss ) {
			GLint signalled;

			glGetSynciv( ss->sync, GL_SYNC_STATUS, 1, NULL, &signalled );
			//TODO: remove this when we wait until next frame
			while ( signalled != GL_SIGNALED ) {
				SDL_Delay( 1 );
				glGetSynciv( ss->sync, GL_SYNC_STATUS, 1, NULL, &signalled );
			}

			if ( signalled == GL_SIGNALED ) {
				glDeleteSync( ss->sync );
				glBindBuffer( GL_PIXEL_PACK_BUFFER, ss->pbo );
				void *data = glMapBuffer( GL_PIXEL_PACK_BUFFER, GL_READ_ONLY );
					console.Print( "Writing screenshot %s (%ix%i)...\n", ss->name, ss->width, ss->height );
					WritePNG( ss->name, (uint8_t*)data, ss->width, ss->height, 4 );
				glUnmapBuffer( GL_PIXEL_PACK_BUFFER );
			}
		}

		void RenderCommand::Execute( void ) const {
			switch( type ) {
			case Type::DRAWQUAD: {
				DrawQuad( &drawQuad );
			} break;
			case Type::DRAWMODEL: {
				DrawModel( &drawModel );
			} break;
			case Type::SCREENSHOT: {
				Screenshot( &screenshot );
			} break;
			default: {
			} break;
			}
		}

	} // namespace Renderer

} // namespace XS
