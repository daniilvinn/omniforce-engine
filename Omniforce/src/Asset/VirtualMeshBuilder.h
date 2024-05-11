#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Renderer/Meshlet.h>

#include <span>
#include <shared_mutex>

namespace Omni {

	// Utility function for building virtual clusterized mesh
	class VirtualMeshBuilder {
	public:
		// Takes a list of meshlets and mesh data, outputs a list of groups of meshlets
		std::vector<std::vector<uint32>> GroupMeshClusters(
			const std::span<RenderableMeshlet>& meshlets,
			const std::span<MeshClusterGroup>& groups,
			std::vector<uint32>& indices,
			std::vector<uint8>& local_indices,
			const std::vector<byte>& vertices,
			uint32 vertex_stride
		);

		void BuildClusterGraph(const std::vector<byte>& vertices, const std::vector<uint32>& indices, uint32 vertex_stride);

	private:
		uint32 FetchIndex(const std::vector<uint32> indices, uint32 offset, uint8 local_index);
		inline static std::shared_mutex m_Mutex;

	};

}