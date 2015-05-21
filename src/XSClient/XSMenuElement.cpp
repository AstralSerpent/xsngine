#include <algorithm>

#include "XSCommon/XSCommon.h"
#include "XSCommon/XSString.h"
#include "XSCommon/XSConsole.h"
#include "XSClient/XSClient.h"
#include "XSClient/XSMenuElement.h"

namespace XS {

	namespace Client {

		MenuElement::MenuElement() {
			std::memset( &properties, 0, sizeof(properties) );
		}

		const vector2 *MenuElement::GetSize( void ) const {
			return nullptr;
		}

		MenuElement::~MenuElement() {
			// ...
		}

	} // namespace Client

} // namespace XS
