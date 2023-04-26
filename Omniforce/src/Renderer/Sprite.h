#pragma once

#include "RendererCommon.h"

namespace Omni {

	struct OMNIFORCE_API Sprite {
		glm::mat4 transform;
		uint32 texture_index;
	};

}