#pragma once

namespace XS {

	namespace OS {

		void GetCurrentWorkingDirectory( char *cwd, size_t bufferLen );
		bool MkDir( const char *path ); // returns true if file exists
		bool ResolvePath( char *outPath, const char *inPath, size_t pathLen );
		bool Stat( const char *path );

	} // namespace OS

} // namespace XS
