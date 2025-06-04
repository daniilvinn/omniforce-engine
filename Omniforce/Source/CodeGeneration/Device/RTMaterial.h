#pragma once

#include <Foundation/Common.h>
#include <CodeGeneration/Device/Texture.h>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") RTMaterialMetadata {
		uint32 HasBaseColor : 1;
		uint32 HasNormal : 1;
		uint32 HasMetallicRoughness : 1;
		uint32 HasOcclusion : 1;
	};

	struct META(ShaderExpose, Module = "RenderingGenerated") RTMaterial {
		RTMaterialMetadata Metadata;
		DeviceTexture BaseColor;
		DeviceTexture Normal;
		DeviceTexture MetallicRoughness;
		DeviceTexture Occlusion;
		glm::vec4 UniformColor;
	};

}