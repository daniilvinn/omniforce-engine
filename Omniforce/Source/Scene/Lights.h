#pragma once

#include <Foundation/Common.h>

#include <CodeGeneration/Device/SpecialTypes.h>

#include <glm/glm.hpp>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") PointLight {
		glm::vec3 Position;
		glm::vec3 Color;
		float32 Intensity;
		float32 Radius;
	};

	struct META(ShaderExpose, Module = "RenderingGenerated") ScenePointLights {
		uint32 Count;
		ShaderRuntimeArray<PointLight> Lights;
	};

}