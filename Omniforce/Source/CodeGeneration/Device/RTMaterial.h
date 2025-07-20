#pragma once

#include <Foundation/Common.h>
#include <CodeGeneration/Device/Texture.h>
#include <CodeGeneration/Device/SpecialTypes.h>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") RTMaterialMetadata {
		uint32 HasBaseColor : 1;
		uint32 HasNormal : 1;
		uint32 HasMetallicRoughness : 1;
		uint32 HasOcclusion : 1;
		uint32 DoubleSided : 1;
	};

	struct META(ShaderExpose, Module = "RenderingGenerated") RTMaterialTransmission {
		float IOR = 1.5;
		float Thickness = 0.01;
		float Factor = 1.0;
	};

	// Example of the new system with template arguments and field-level conditions
	struct META(ShaderExpose, Module = "RenderingGenerated", TemplateArguments = "SurfaceDomain Domain = SurfaceDomain.Opaque") RTMaterial {
		RTMaterialMetadata Metadata;
		DeviceTexture BaseColor;
		DeviceTexture Normal;
		DeviceTexture MetallicRoughness;
		DeviceTexture Occlusion;
		glm::vec4 UniformColor;
		META(Condition = "Domain == SurfaceDomain.Transmissive") ShaderConditional<RTMaterialTransmission> TransmissionData;
	};

}