#pragma once

#if __OMNI_COMPILE_HOST_CODE
#include <glm/glm.hpp>

namespace Omni {

	using float4 = glm::vec4;
	using float3 = glm::vec3;
	using float2 = glm::vec2;
	
	using half4 = glm::u16vec4;
	using half3 = glm::u16vec3;
	using half2 = glm::u16vec2;
	using half  = glm::uint16;

	using float4x4 = glm::mat4;

	using uint = uint32;


}
#endif