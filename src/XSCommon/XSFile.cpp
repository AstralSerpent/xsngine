#include "XSSystem/XSInclude.h"

#include "XSCommon/XSCvar.h"
#include "XSCommon/XSCommon.h"
#include "XSCommon/XSFile.h"
#include "XSCommon/XSString.h"
#include "XSCommon/XSCommand.h"
#include "XSCommon/XSConsole.h"

namespace XS {

	Cvar *File::com_path;
	static const char *modes[File::NUM_MODES] = {
		"rb", // READ
		"rb", // READ_BINARY
		"wb", // WRITE
		"wb" // WRITE_BINARY
		"a" // APPEND
	};

	void File::Init( void ) {
		com_path = Cvar::Get( "com_path" );
	}

	void File::GetPath( const char *gamePath, char *outPath, size_t outLen ) {
		String::sprintf( outPath, outLen, "%s/%s", com_path->GetCString(), gamePath );
	}

	File::File( const char *gamePath, Mode mode ) {
		GetPath( gamePath, path, sizeof( path ) );

		this->mode = mode;
		file = fopen( path, modes[mode] );

		if ( !file ) {
			length = 0;
			return;
		}

		fseek( file, 0L, SEEK_END );
			length = ftell( file );
		fseek( file, 0L, SEEK_SET );

		// account for null terminator
		if ( mode == READ )
			length++;
	}

	void File::Read( byte *buf, size_t len ) {
		if ( len == 0 )
			len = length;

		fread( (void *)buf, 1, len, file );

		// account for null terminator
		if ( mode == READ )
			buf[len-1] = '\0';
	}

	void File::AppendString( const char *str ) {
		if ( mode != APPEND ) {
			Console::Print( "Tried to append to file not opened with APPEND mode: %s", path );
			return;
		}

		fwrite( str, 1, strlen(str), file );
	}

	File::~File() {
		if ( file ) {
			fclose( file );
			file = NULL;
		}
	}

} // namespace XS
