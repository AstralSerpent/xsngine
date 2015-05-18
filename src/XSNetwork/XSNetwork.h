#pragma once

#include <RakNet/MessageIdentifiers.h>

namespace XS {

	namespace Network {

		enum GameMessage {
			ID_XS_PING = ID_USER_PACKET_ENUM + 1,
			ID_XS_TEXT_MESSAGE,
		};

		struct XSPacket {
			GameMessage		 msg;
			const void		*data;
			size_t			 dataLen;

			// don't allow default instantiation
			XSPacket() = delete;
			XSPacket( const XSPacket& ) = delete;
			XSPacket& operator=( const XSPacket& ) = delete;

			XSPacket( GameMessage msg )
			: msg( msg ), data( nullptr ), dataLen( 0u )
			{
			}
		};

		// ???
		void Init(
			void
		);

		// ???
		bool IsConnected(
			void
		);

		// ???
		bool Connect(
			const char *hostname,
			uint16_t port
		);

		// ???
		void Receive(
			void
		);

		// ???
		void Send(
			const XSPacket *packet
		);

	} // namespace Network

} // namespace XS
