#pragma once

#include <Foundation/Common.h>

namespace Omni {

	struct META(ShaderExpose, Module = "Common") ViewData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 view_proj;
		glm::vec3 position;
		Frustum frustum;
		glm::vec3 forward_vector;
		float32 fov; // in radians
		float32 viewport_height;
		float32 viewport_width;
		float32 near_clip_distance;
		float32 far_clip_distance;
	};

}
