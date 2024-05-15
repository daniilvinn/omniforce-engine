#include "../VirtualMeshBuilder.h"
#include "../MeshPreprocessor.h"
#include <Log/Logger.h>
#include <Asset/MeshPreprocessor.h>

#include <unordered_map>
#include <fstream>
#include <span>

#include <metis.h>

namespace Omni {

	std::vector<MeshClusterGroup> VirtualMeshBuilder::GroupMeshClusters(
		const std::span<RenderableMeshlet>& meshlets,
		std::vector<uint32>& indices,
		std::vector<uint8>& local_indices,
		const std::vector<byte>& vertices,
		uint32 vertex_stride
	) {
		OMNIFORCE_ASSERT_TAGGED(vertex_stride >= 12, "Vertex stride is less than 12: no vertex position data?");

		if (meshlets.size() < 8) {
			MeshClusterGroup group;
			for (uint32 i = 0; i < meshlets.size(); i++) {
				group.push_back(i);
			}
				
			return { group };
		}

		// meshlets represented by their index into 'meshlets'
		std::unordered_map<MeshletEdge, std::vector<std::size_t>, MeshletEdgeHasher> edges_meshlets_map;
		std::unordered_map<std::size_t, std::vector<MeshletEdge>> meshlets_edges_map; // probably could be a vector
		std::vector<uint32> geometry_only_indices;

		// A hack to satisfity meshopt with `index_count % 3 == 0` requirement
		uint32 ib_padding = 3 - (indices.size() % 3);
		indices.resize(indices.size() + ib_padding);
		MeshPreprocessor mesh_preprocessor = {};
		mesh_preprocessor.GenerateShadowIndexBuffer(
			&geometry_only_indices,
			&indices,
			&vertices,
			12,
			vertex_stride
		);
		indices.resize(indices.size() - ib_padding);

		// for each cluster
		for (std::size_t meshlet_idx = 0; meshlet_idx < meshlets.size(); meshlet_idx++) {
			const auto& meshlet = meshlets[meshlet_idx];

			const std::size_t triangleCount = meshlet.triangle_count;
			// for each triangle of the cluster
			for (std::size_t triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++) {
				// for each edge of the triangle
				for (std::size_t i = 0; i < 3; i++) {
					MeshletEdge edge(
						geometry_only_indices[local_indices[(i + triangleIndex * 3) + meshlet.triangle_offset] + meshlet.vertex_offset],
						geometry_only_indices[local_indices[(((i + 1) % 3) + triangleIndex * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]
					);

					auto& edge_meshlets = edges_meshlets_map[edge];
					auto& meshlet_edges = meshlets_edges_map[meshlet_idx];

					if (edge.first != edge.second) {
						// If meshlet has already been registered for this edge - skip it
						if (std::find(edge_meshlets.begin(), edge_meshlets.end(), meshlet_idx) == edge_meshlets.end())
							edge_meshlets.push_back(meshlet_idx);

						// If meshlet-edges pair already contains an edge - don't register it
						if (std::find(meshlet_edges.begin(), meshlet_edges.end(), edge) == meshlet_edges.end())
							meshlet_edges.emplace_back(edge);
					}
				}
			}
		}

		// Erase non-shared edges
		std::erase_if(edges_meshlets_map, [&](const auto& pair) {
			return pair.second.size() <= 1;
		});

		// If no connections between meshlets were detected, return a group with all meshlets
		if (edges_meshlets_map.empty()) {
			return groupWithAllMeshlets();
		}

		idx_t graph_vertex_count = meshlets.size(); // graph is built from meshlets, hence graph vertex = meshlet
		idx_t num_constrains = 1;
		idx_t num_partitions = meshlets.size() / 4; // Group by 4 meshlets

		OMNIFORCE_ASSERT_TAGGED(num_partitions > 1, "Invalid partition count");

		idx_t metis_options[METIS_NOPTIONS];
		METIS_SetDefaultOptions(metis_options);

		// edge-cut, ie minimum cost betweens groups.
		metis_options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
		metis_options[METIS_OPTION_CCORDER] = 1; // identify connected components first
		metis_options[METIS_OPTION_NUMBERING] = 0;

		// Initialize data for METIS
		std::vector<idx_t> partition;
		partition.resize(graph_vertex_count);

		// xadj
		std::vector<idx_t> x_adjacency;
		x_adjacency.reserve(graph_vertex_count + 1);

		// adjncy
		std::vector<idx_t> edge_adjacency;

		// edgwgts
		std::vector<idx_t> edge_weights;

		for (uint32 meshlet_idx = 0; meshlet_idx < meshlets.size(); meshlet_idx++) {
			uint32 edge_adjacency_range_begin_iterator = edge_adjacency.size();
			for (const auto& edge : meshlets_edges_map[meshlet_idx]) {

				auto connections_iterator = edges_meshlets_map.find(edge);
				if (connections_iterator == edges_meshlets_map.end()) {
					continue;
				}

				const auto& connections = connections_iterator->second;

				for (const auto& meshlet : connections) {
					if (meshlet != meshlet_idx) {
						auto existingEdgeIter = std::find(edge_adjacency.begin() + edge_adjacency_range_begin_iterator, edge_adjacency.end(), meshlet);
						if (existingEdgeIter == edge_adjacency.end()) {
							// First encounter - register connection
							edge_adjacency.emplace_back(meshlet);
							edge_weights.emplace_back(1);
						}
						else {
							// Increment connection count
							std::ptrdiff_t pointer_distance = std::distance(edge_adjacency.begin(), existingEdgeIter);

							OMNIFORCE_ASSERT_TAGGED(pointer_distance >= 0, "Pointer distanceless than zero");
							OMNIFORCE_ASSERT_TAGGED(pointer_distance < edge_weights.size(), "Edge weights and edge adjacency must have the same length");

							edge_weights[pointer_distance]++;
						}
					}
				}
			}
			x_adjacency.push_back(edge_adjacency_range_begin_iterator);
		}
		x_adjacency.push_back(edge_adjacency.size());

		OMNIFORCE_ASSERT_TAGGED(x_adjacency.size() == meshlets.size() + 1, "unexpected count of vertices for METIS graph: invalid xadj");
		OMNIFORCE_ASSERT_TAGGED(edge_adjacency.size() == edge_weights.size(), "Failed during METIS CSR graph generation: invalid adjncy / edgwgts");

		idx_t final_cut_cost; // final cost of the cut found by METIS
		int result = METIS_PartGraphKway(
			&graph_vertex_count,
			&num_constrains,
			x_adjacency.data(),
			edge_adjacency.data(),
			nullptr, /* vertex weights */
			nullptr, /* vertex size */
			edge_weights.data(),
			&num_partitions,
			nullptr,
			nullptr,
			metis_options,
			&final_cut_cost,
			partition.data()
		);

		OMNIFORCE_ASSERT_TAGGED(result == METIS_OK, "Graph partitioning failed!");

		// ===== Group meshlets together
		std::vector<MeshClusterGroup> groups;
		groups.resize(num_partitions);
		for (std::size_t i = 0; i < meshlets.size(); i++) {
			idx_t partitionNumber = partition[i];
			groups[partitionNumber].push_back(i);
		}
		return groups;

	}

	float32 VirtualMeshBuilder::ComputeVertexDensity(const std::vector<byte> vertices, uint32 vertex_stride)
	{
		// Not efficient *at all*. Should be rewritten to Kd-tree implementation
		OMNIFORCE_ASSERT_TAGGED(vertices.size(), "No points provided");
		OMNIFORCE_ASSERT_TAGGED(vertex_stride >= 12, "Invalid vertex stride");

		float32 total_distance = 0.0f;
		uint32 num_iterations = vertices.size() / vertex_stride;

		for (uint32 i = 0; i < num_iterations; i++) {
			glm::vec3 v1 = {};
			memcpy(&v1, vertices.data() + (i * vertex_stride), sizeof glm::vec3);

			for (uint32 j = i + 1; j < num_iterations; j++) {
				glm::vec3 v2 = {};
				memcpy(&v2, vertices.data() + (j * vertex_stride), sizeof glm::vec3);

				total_distance += glm::distance(v1, v2);
			}
		}

		float32 average_distance = total_distance / (num_iterations * (num_iterations - 1) / 2);
		float32 density_per_unit = 1.0f / average_distance;

		return density_per_unit;
	}

	VirtualMesh VirtualMeshBuilder::BuildClusterGraph(const std::vector<byte>& vertices, const std::vector<uint32>& indices, uint32 vertex_stride)
	{
		MeshPreprocessor mesh_preprocessor = {};

		float32 vertex_density = ComputeVertexDensity(vertices, vertex_stride);

		// Clusterize initial mesh, basically build LOD 0
		Scope<ClusterizedMesh> meshlets_data = mesh_preprocessor.GenerateMeshlets(&vertices, &indices, vertex_stride);

		// All mesh's meshlet groups
		// Init with source meshlets, 1 meshlet per group
		std::vector<MeshClusterGroup> mesh_cluster_groups(meshlets_data->meshlets.size());
		for (uint32 i = 0; i < mesh_cluster_groups.size(); i++)
			mesh_cluster_groups[i].push_back(i);

		for (uint32 i = 0; i < meshlets_data->meshlets.size(); i++) {
			meshlets_data->meshlets[i].group = i;
		}

		uint32 lod_idx = 1;
		uint32 current_meshlets_offset = 0;
		const uint32 max_lod = 10;

		while (lod_idx < max_lod) {
			std::span<RenderableMeshlet> meshlets_to_group(meshlets_data->meshlets.begin() + current_meshlets_offset, meshlets_data->meshlets.end());
			auto groups = GroupMeshClusters(meshlets_to_group, meshlets_data->indices, meshlets_data->local_indices, vertices, vertex_stride);

			float t_lod = float(lod_idx) / (float)max_lod;

			uint32 num_newly_created_meshlets = 0;
			std::vector<RenderableMeshlet> new_meshlets;
			for (const auto& group : groups) {
				std::vector<uint32> merged_indices;
				uint32 num_merged_indices = 0;

				for (auto& meshlet_idx : group)
					num_merged_indices += meshlets_data->meshlets[meshlet_idx].triangle_count * 3;

				merged_indices.reserve(num_merged_indices);

				for (auto& meshlet_idx : group) {
					RenderableMeshlet& meshlet = meshlets_data->meshlets[meshlet_idx + current_meshlets_offset];

					for (uint32 index_idx = 0; index_idx < meshlet.triangle_count * 3; index_idx++) {
						merged_indices.push_back(meshlets_data->indices[meshlet.vertex_offset + meshlets_data->local_indices[meshlet.triangle_offset + index_idx]]);
					}
				}

				float32 target_error = 1.0f * t_lod + 0.01f * (1 - t_lod);

				std::vector<uint32> simplified_indices;

				if(simplified_indices.size())
					mesh_preprocessor.GenerateMeshLOD(&simplified_indices, &vertices, &merged_indices, vertex_stride, merged_indices.size() / 2, target_error, true);

				OMNIFORCE_ASSERT_TAGGED(simplified_indices.size(), "Failed to generate LOD");

				if (simplified_indices.size() == merged_indices.size())
					continue;

				Scope<ClusterizedMesh> simplified_meshlets = mesh_preprocessor.GenerateMeshlets(&vertices, &simplified_indices, vertex_stride);

				uint32 group_idx = UUID() % 3240234287;
				num_newly_created_meshlets += simplified_meshlets->meshlets.size();
				for (auto& meshlet : simplified_meshlets->meshlets) {
					meshlet.vertex_offset += meshlets_data->indices.size();
					meshlet.triangle_offset += meshlets_data->local_indices.size();
					meshlet.group = group_idx;
					meshlet.lod = lod_idx;
				}
				
				new_meshlets.insert(new_meshlets.end(), simplified_meshlets->meshlets.begin(), simplified_meshlets->meshlets.end());

				meshlets_data->indices.insert(meshlets_data->indices.end(), simplified_meshlets->indices.begin(), simplified_meshlets->indices.end());
				meshlets_data->local_indices.insert(meshlets_data->local_indices.end(), simplified_meshlets->local_indices.begin(), simplified_meshlets->local_indices.end());
				meshlets_data->cull_bounds.insert(meshlets_data->cull_bounds.end(), simplified_meshlets->cull_bounds.begin(), simplified_meshlets->cull_bounds.end());

			}
			current_meshlets_offset = meshlets_data->meshlets.size();
			meshlets_data->meshlets.insert(meshlets_data->meshlets.end(), new_meshlets.begin(), new_meshlets.end());

			if(num_newly_created_meshlets == 1)
				break;

			OMNIFORCE_CORE_TRACE("Generated LOD_{}", lod_idx);
			lod_idx++;
		}

		VirtualMesh mesh = {};
		mesh.vertices = vertices;
		mesh.indices = meshlets_data->indices;
		mesh.local_indices = meshlets_data->local_indices;
		mesh.meshlets = meshlets_data->meshlets;
		mesh.meshlet_groups = mesh_cluster_groups;
		mesh.cull_bounds = meshlets_data->cull_bounds;
		mesh.vertex_stride = vertex_stride;

		return mesh;

	}

	

}