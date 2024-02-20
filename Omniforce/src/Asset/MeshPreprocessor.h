#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Renderer/Meshlet.h>

#include <glm/glm.hpp>

namespace Omni {

	struct GeneratedMeshlets {
		std::vector<RenderableMeshlet> meshlets;
		std::vector<uint32> indices;
		std::vector<byte> local_indices;
		std::vector<MeshletCullBounds> cull_bounds;
	};

	class OMNIFORCE_API MeshPreprocessor {
	public:
		GeneratedMeshlets* GenerateMeshlets(const std::vector<glm::vec3>& vertices, const std::vector<uint32>& indices);
		Sphere GenerateBoundingSphere(const std::vector<glm::vec3>& points);
		void OptimizeMesh(std::vector<byte>& vertices, std::vector<uint32>& indices, uint8 vertex_stride);

	private:

	};

}