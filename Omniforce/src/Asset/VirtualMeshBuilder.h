#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Renderer/Meshlet.h>

namespace Omni {

	// Utility function for building virtual clusterized mesh
	class VirtualMeshBuilder {
	public:
		// Takes a list of meshlets and mesh data, outputs a list of groups of meshlets
		std::vector<std::vector<uint32>> GroupMeshClusters(
			std::vector<RenderableMeshlet>& meshlets, 
			std::vector<uint32>& indices, 
			std::vector<uint8>& local_indices, 
			const std::vector<byte>& vertices, 
			uint32 vertex_stride
		);

	};

}