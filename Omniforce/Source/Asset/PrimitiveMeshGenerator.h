#pragma once

#include <Foundation/Common.h>

namespace Omni {

	class OMNIFORCE_API PrimitiveMeshGenerator {
	public:
		std::pair<std::vector<glm::vec3>, std::vector<uint32>> GenerateIcosphere(uint32 subdivisions = 1);
		std::pair<std::vector<glm::vec3>, std::vector<uint32>> GenerateCube();
	};

}