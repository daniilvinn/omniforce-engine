#pragma once

#include "SceneCommon.h"

namespace Omni {

	struct OMNIFORCE_API Sprite {
		glm::mat4 transform;
		fvec4 color_tint;
		uint32 texture_index;
	};

}