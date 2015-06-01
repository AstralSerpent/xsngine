#pragma once

#include <vector>

namespace XS {

	namespace ServerGame {

		class Entity;

		enum GameType : uint32_t {
			DM,
			TeamDM,
			CTF,
		};

		extern struct GameState {
			GameType					gameType;
			std::vector<Entity *>		entities;
			uint32_t					numEntities;
		} state;

		// initialise the ServerGame
		void Init(
			void
		);

		// run a frame, simulate entities
		void RunFrame(
			real64_t dt
		);

		// ???
		void GenerateSnapshot(
			void
		);

	} // namespace ServerGame

} // namespace XS
