#pragma once

#include <SDL2/SDL_keycode.h>

//
// XS event pump
//
//	events are generated throughout the frame and timestamped (i.e. keyboard/mouse input)
//	They are processed at key points in the frame, such as before executing the command buffer and before running a
//		client frame or server frame
//	Internal functions are encapsulated in XSEvent.cpp
//

namespace XS {

	enum EventType {
		KEY = 0,
		NUM_EVENTS
	};

	struct XSEvent {
		// these will be set internally
		EventType	type;
		uint32_t	time;

		union {
			struct {
				SDL_Keycode	key;
				bool		down;
			} keyEvent;
		};
	};

	namespace Event {

		void Init( void );
		void Queue( EventType type, XSEvent *ev );
		void Pump( void );

	} // namespace Event

} // namespace XS
