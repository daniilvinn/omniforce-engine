#pragma once

#include "SceneCommon.h"

namespace Omni {

	struct OMNIFORCE_API Sprite {
		fvec4 color_tint;
		fvec3 position;
		fvec2 size;
		float32 rotation;
		uint32 texture_index;

		Sprite() {
			color_tint = { 1.0f, 1.0f, 1.0f, 1.0f };
			position = {0.0f, 0.0f};
			size = { 1.0f, 1.0f };
			rotation = 0.0f;
			texture_index = 0;
		};
	};

}