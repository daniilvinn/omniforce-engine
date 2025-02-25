#pragma once

#include <Foundation/Common.h>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") DeviceTexture {
		uint32 texture_index;
		uint32 uv_channel_index;
	};

}