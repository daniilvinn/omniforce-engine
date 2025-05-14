#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <glm/gtc/packing.hpp>

namespace Omni {

	struct OMNIFORCE_API META(ShaderExpose, Module = "BasicTypes") Sprite {
		glm::vec3 position;
		glm::hvec4 rotation;
		glm::vec2 size;
		glm::vec4 color_tint;
		uint32 texture_id;

		Sprite() {
			color_tint = { 1.0f, 1.0f, 1.0f, 1.0f };
			position = { 0.0f, 0.0f, 0.0f };
			size = { 1.0f, 1.0f };
			rotation = glm::packHalf(glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f));
			texture_id = 0;
		};

	};

}