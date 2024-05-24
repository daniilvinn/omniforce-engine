#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Renderer/Meshlet.h>
#include <Core/KDTree.h>

#include <span>
#include <shared_mutex>

#include <robin_hood.h>

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
		// Generates a Virtual mesh - a hiearchy of meshlets, representing variable level of detail between each LOD level.
		VirtualMesh BuildClusterGraph(const std::vector<byte>& vertices, const std::vector<uint32>& indices, uint32 vertex_stride);


		// === Helper methods ===
		// Takes a list of meshlets and mesh data, outputs a list of groups of meshlets
		std::vector<MeshClusterGroup> GroupMeshClusters(
			std::span<RenderableMeshlet> meshlets,
			std::span<uint32> meshlet_indices,
			const std::vector<uint32>& welder_remap_table,
			std::vector<uint32>& indices,
			std::vector<uint8>& local_indices,
			const std::vector<byte>& vertices,
			uint32 vertex_stride
		);

		// Average squared vertex distance, used for vertex welding
		float32 ComputeAverageVertexDistanceSquared(const std::vector<byte> vertices, const std::vector<uint32>& indices, uint32 vertex_stride);
		// Finds edge vertices so they are not involved in welding
		std::vector<bool> GenerateEdgeMap(std::span<RenderableMeshlet> meshlets, const std::span<uint32> current_meshlets, const std::vector<byte>& vertices, std::vector<uint32>& indices, const std::vector<uint8>& local_indices, uint32 vertex_stride);
		// Performs vertex welding
		std::vector<uint32> GenerateVertexWelderRemapTable(const std::vector<byte>& vertices, uint32 vertex_stride, const KDTree& kd_tree, const rh::unordered_flat_set<uint32>& lod_indices, const std::vector<bool>& edge_vertex_map, float32 min_vertex_distance);

	private:
		inline static std::shared_mutex m_Mutex;

	};

}