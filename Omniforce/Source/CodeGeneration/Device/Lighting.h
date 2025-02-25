#pragma once

#include <Foundation/Common.h>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") PointLight {
		glm::vec3 position;
		glm::vec3 color;
		float32 intensity;
		float32 min_radius;
		float32 radius;
	};

}