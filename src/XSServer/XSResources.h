#pragma once

namespace XS {

	class ByteBuffer;

	namespace ServerGame {

		// ???
		uint32_t GetResourceID(
			const char *name
		) XS_WARN_UNUSED_RESULT;

		// ???
		void NetworkResources(
			bool force
		);

	} // namespace ServerGame

} // namespace XS
