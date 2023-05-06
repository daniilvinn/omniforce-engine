#pragma once

#include "SceneCommon.h"

namespace Omni {

	struct OMNIFORCE_API Sprite {
		fvec4 color_tint;
		fvec2 position;
		fvec2 size;
		float32 rotation;
		uint32 texture_index;

		Sprite() = default;
	};

}