#pragma once

#include "XSClient/XSBaseCamera.h"

namespace XS {

	namespace ClientGame {

		class FlyCamera : public BaseCamera {
		public:
			inline FlyCamera( real32_t speed )
			: BaseCamera( glm::vec3() ), flySpeed( speed )
			{
			}

			real32_t	flySpeed;

			void Update(
				real64_t dt
			);

		protected:
			void HandleKeyboardInput(
				real64_t dt
			);
			void HandleMouseInput(
				real64_t dt
			);
			void CalculateRotation(
				real64_t dt,
				real64_t xOffset,
				real64_t yOffset
			);
		};

	} // namespace ClientGame

} // namespace XS
