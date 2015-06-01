#pragma once

namespace XS {

	namespace Renderer {
		class Renderable;
		class View;
	}

	namespace ClientGame {

		class Entity {
		private:

		public:
			Renderer::Renderable	*renderObject;

			Entity()
			: renderObject( nullptr )
			{
			}
			virtual ~Entity();

			virtual void Update(
				const real64_t dt
			);

			void AddToScene(
				Renderer::View *view
			) const;
		};

	} // namespace ClientGame

} // namespace XS
