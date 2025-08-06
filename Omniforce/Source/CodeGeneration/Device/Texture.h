#pragma once

#include <Foundation/Common.h>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") DeviceTexture {
		uint32 TextureIndex;
		uint32 UVChannelIndex;
	};

}