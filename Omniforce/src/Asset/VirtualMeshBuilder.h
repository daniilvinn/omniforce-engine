#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Renderer/Meshlet.h>

#include <span>
#include <shared_mutex>

namespace Omni {

	struct VirtualMesh {
		std::vector<byte> vertices;
		std::vector<uint32> indices;
		std::vector<uint8> local_indices;
		std::vector<RenderableMeshlet> meshlets;
		std::vector<MeshClusterGroup> meshlet_groups;
		std::vector<MeshletCullBounds> cull_bounds;
		uint32 vertex_stride;
	};

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

		VirtualMesh BuildClusterGraph(const std::vector<byte>& vertices, const std::vector<uint32>& indices, uint32 vertex_stride);

	private:
		inline static std::shared_mutex m_Mutex;

	};

}