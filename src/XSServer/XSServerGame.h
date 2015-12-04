#pragma once

#include <list>
#include <vector>

namespace XS {

	namespace Physics {
		class Scene;
	}

	namespace ServerGame {

		class Entity;

		extern struct GameState {
			std::list<Entity *>		entities;
			Physics::Scene			*physicsScene = nullptr;

			struct NetworkState {
				std::vector<uint32_t>	removedEntities; // removed this frame, notify client
			} net;
		} svgState;

		// initialise the ServerGame
		void Init(
			void
		);

		// ???
		void Shutdown(
			void
		);

		// run a frame, simulate entities
		void RunFrame(
			real64_t dt
		);

		// ???
		void Connect(
			const char *ip,
			uint64_t guid
		);

	} // namespace ServerGame

} // namespace XS
