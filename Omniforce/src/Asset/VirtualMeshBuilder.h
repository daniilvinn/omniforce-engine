#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Renderer/Meshlet.h>
#include <Core/KDTree.h>
#include "Importers/ModelImporter.h"

#include <span>
#include <shared_mutex>
#include <atomic>

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

	struct LODGenerationPassStatistics {
		uint32 input_meshlet_count = 0; // input meshlet count
		std::atomic<uint32> output_meshlet_count = 0; // output meshlet count
		uint32 group_count = 0; // how many groups were generated
		uint32 welded_vertex_count = 0; // how many vertices were welded to another vertex
		uint32 input_vertex_count = 0; // how many vertices in the input (must equal to previous pass `output_index_count`)
		uint32 locked_vertex_count = 0; // how many vertices were locked during welding due to edge detection
		std::atomic<uint32> group_simplification_failure_count = 0; // how many groups failed to simplify
		std::atomic<uint32> degenerate_triangles_erased = 0; // how many degenerate triangles were removed after welding
		float32 mesh_scale = 0.0f;
		float32 min_welder_vertex_distance = 0.0f;
	};

	// Utility class for virtual clusterized mesh build
	class VirtualMeshBuilder {
	public:
		// Generates a Virtual mesh - a hierarchy of meshlets, representing variable level of detail between each LOD level.
		VirtualMesh BuildClusterGraph(
			const std::vector<byte>& vertices, 
			const std::vector<uint32>& indices, 
			uint32 vertex_stride, 
			const VertexAttributeMetadataTable& vertex_metadata
		);

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

		// Finds edge vertices so they are not involved in welding
		std::vector<bool> GenerateEdgeMap(
			std::span<RenderableMeshlet> meshlets, 
			const std::span<uint32> current_meshlets, 
			const std::vector<byte>& vertices, 
			std::vector<uint32>& indices, 
			const std::vector<uint8>& local_indices, 
			uint32 vertex_stride
		);

		// Performs vertex welding
		std::vector<uint32> GenerateVertexWelderRemapTable(
			const std::vector<byte>& vertices, 
			uint32 vertex_stride, 
			const KDTree& kd_tree, 
			const rh::unordered_flat_set<uint32>& lod_indices, 
			const std::vector<bool>& edge_vertex_map, 
			float32 min_vertex_distance,
			float32 min_uv_distance,
			const VertexAttributeMetadataTable& vertex_metadata,
			LODGenerationPassStatistics& stats
		);

	private:
#ifndef METIS_HAS_THREADLOCAL
		inline static std::shared_mutex m_Mutex;
#endif

		
	};

}