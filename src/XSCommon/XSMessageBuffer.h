#pragma once

#include <vector>
#include <string>

namespace XS {

	class Logger;

	class MessageBuffer {
	private:
		std::vector<std::string>	 buffer;
		Logger						*log;

	public:
		// don't allow default instantiation
		MessageBuffer() = delete;
		MessageBuffer( const MessageBuffer& ) = delete;
		MessageBuffer& operator=( const MessageBuffer& ) = delete;

		MessageBuffer(
			const char *logfile
		);

		~MessageBuffer();

		// append a message to the buffer
		void Append(
			std::string message
		);

		// check if the buffer is empty
		bool IsEmpty(
			void
		) const XS_WARN_UNUSED_RESULT;

		// clear the message buffer
		inline void Clear( void ) {
			buffer.clear();
		}

		// retrieve count lines from index start
		std::vector<std::string> FetchLines(
			uint32_t start,
			uint32_t count
		) const XS_WARN_UNUSED_RESULT;

		// query the number of lines the buffer contains
		uint32_t GetNumLines(
			void
		) const XS_WARN_UNUSED_RESULT;
	};

} // namespace XS
