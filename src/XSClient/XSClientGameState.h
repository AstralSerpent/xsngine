#pragma once

#include <unordered_map>

#include "XSCommon/XSVector.h"

namespace XS {

	namespace ClientGame {

		class Entity;
		typedef std::unordered_map<uint32_t, Entity *> EntityContainer;

		extern struct GameState {
			vector3			viewAngles;
			vector3			viewDelta;
			EntityContainer	entities;
		} state;

	} // namespace ClientGame

} // namespace XS
