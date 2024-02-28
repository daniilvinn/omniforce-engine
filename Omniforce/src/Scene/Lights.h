#pragma once

#include "glm/glm.hpp"

namespace Omni {

	struct PointLight {
		glm::vec3 position;
		glm::vec3 color;
		float32 intensity;
	};

}