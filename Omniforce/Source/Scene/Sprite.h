#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <glm/gtc/packing.hpp>

namespace Omni {

	struct OMNIFORCE_API Sprite {
		fvec4 color_tint;
		fvec3 position;
		fvec2 size;
		glm::u16vec4 rotation;
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