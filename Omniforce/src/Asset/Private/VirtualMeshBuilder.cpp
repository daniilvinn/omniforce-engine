#include "../VirtualMeshBuilder.h"
#include "../MeshPreprocessor.h"
#include "Log/Logger.h"

#include <unordered_map>

#include <metis.h>

namespace Omni {

	std::vector<std::vector<Omni::uint32>> VirtualMeshBuilder::GroupMeshClusters(
		std::vector<RenderableMeshlet>& meshlets, 
		std::vector<uint32>& indices, 
		std::vector<uint8>& local_indices, 
		const std::vector<byte>& vertices, 
		uint32 vertex_stride
	) {
		OMNIFORCE_ASSERT_TAGGED(vertex_stride >= 12, "Vertex stride is less than 12: no vertex position data?");
		OMNIFORCE_ASSERT_TAGGED(meshlets.size() >= 8, "Less than 8 meshlets - merge them instead of partitioning");

		MeshPreprocessor mesh_preprocessor;

		std::unordered_map<MeshletEdge, std::vector<uint32>, MeshletEdgeHasher> edges_meshlets_map;
		std::unordered_map<uint32, std::vector<MeshletEdge>> meshlets_edges_map;

		std::vector<uint32> geometry_only_indices;

		// A hack to satisfity meshopt with `index_count % 3 == 0` requirement
		uint32 ib_padding = 3 - (indices.size() % 3);
		indices.resize(indices.size() + ib_padding);

		mesh_preprocessor.GenerateShadowIndexBuffer(
			&geometry_only_indices,
			&indices,
			&vertices,
			12,
			vertex_stride
		);
		indices.resize(indices.size() - ib_padding);

		// per meshlet
		for (uint32 meshlet_idx = 0; meshlet_idx < meshlets.size(); meshlet_idx++) {
			auto& meshlet = meshlets[meshlet_idx];

			auto getVertexIndex = [&](uint32 index) {
				return geometry_only_indices[local_indices[index + meshlet.triangle_offset] + meshlet.vertex_offset];
			};

			// per triangle
			for (uint32 triangle_idx = 0; triangle_idx < meshlet.triangle_count; triangle_idx++) {
				// per edge
				for (uint32 edge_idx = 0; edge_idx < 3; edge_idx++) {
					MeshletEdge edge(
						geometry_only_indices[local_indices[(edge_idx + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset],
						geometry_only_indices[local_indices[(((edge_idx + 1) % 3) + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]
					);

					auto& edge_meshlets = edges_meshlets_map[edge];
					auto& meshlet_edges = meshlets_edges_map[meshlet_idx];

					// If edge-meshlets pair already contains current meshlet idx - don't register it
					if (std::find(edge_meshlets.begin(), edge_meshlets.end(), meshlet_idx) == edge_meshlets.end())
						edge_meshlets.push_back(meshlet_idx);

					// If meshlet-edges pair already contains an edge - don't register it
					if (std::find(meshlet_edges.begin(), meshlet_edges.end(), edge) == meshlet_edges.end())
						meshlet_edges.emplace_back(edge);
				}

			}
		}

		// Remove edges non-shared edges
		std::erase_if(edges_meshlets_map, [](const auto& pair) {
			return pair.second.size() <= 1;
		});

		OMNIFORCE_ASSERT_TAGGED(edges_meshlets_map.size(), "No connections between clusters");
		OMNIFORCE_ASSERT_TAGGED(meshlets_edges_map.size(), "No cluster edges detected");

		idx_t cluster_graph_vertex_count = meshlets.size(); // Graph is built from meshlets, hence meshlet = graph vertex
		idx_t num_constaints = 1; // Default minimal value
		idx_t num_partitions = meshlets.size() / 4; // Make groups of 4 meshlets

		OMNIFORCE_ASSERT_TAGGED(num_partitions >= 2, "Num partitions must be greater or equal to 2");

		// Setup METIS options for graph edge-cut operations
		idx_t options[METIS_NOPTIONS];
		METIS_SetDefaultOptions(options);

		options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
		options[METIS_OPTION_CCORDER] = 1;

		std::vector<idx_t> partition;
		partition.resize(cluster_graph_vertex_count);

		// xadj
		std::vector<idx_t> x_adjacency;
		x_adjacency.reserve(cluster_graph_vertex_count + 1);

		// adjncy
		std::vector<idx_t> edge_adjacency;

		// weight of each edge. Weight of an edge = num shared edges between connected clusters
		std::vector<idx_t> edge_weights;

		// Per meshlet
		for (uint32 meshlet_idx = 0; meshlet_idx < meshlets.size(); meshlet_idx++) {
			uint32 first_adj_index = edge_adjacency.size();

			// Per edge
			for (const auto& edge : meshlets_edges_map[meshlet_idx]) {
				auto connections_iterator = edges_meshlets_map.find(edge);
				if (connections_iterator == edges_meshlets_map.end())
					continue;

				const auto& connections = connections_iterator->second;

				// Per connection
				for (const auto& connected_cluster : connections) {
					if (connected_cluster != meshlet_idx) {
						auto edge_iterator = std::find(edge_adjacency.begin() + first_adj_index, edge_adjacency.end(), connected_cluster);
						if (edge_iterator == edge_adjacency.end()) {
							// First encounter, register connection
							edge_adjacency.emplace_back(connected_cluster);
							edge_weights.emplace_back(1);
						}
						else {
							// Not first encounter, increment num connections
							std::ptrdiff_t pointer_difference = std::distance(edge_adjacency.begin(), edge_iterator);
							OMNIFORCE_ASSERT_TAGGED(pointer_difference >= 0, "Pointer difference less than zero");
							OMNIFORCE_ASSERT_TAGGED(pointer_difference < edge_weights.size(), "Edge weight and adjacency for have the same size");

							edge_weights[pointer_difference]++;
						}
					}
				}
			}
			x_adjacency.push_back(first_adj_index);
		}
		x_adjacency.push_back(edge_adjacency.size());

		idx_t edge_cut;
		int result = METIS_PartGraphKway(
			&cluster_graph_vertex_count,
			&num_constaints,
			x_adjacency.data(),
			edge_adjacency.data(),
			nullptr, /* vertex weights */
			nullptr, /* vertex size */
			edge_weights.data(),
			&num_partitions,
			nullptr,
			nullptr,
			options,
			&edge_cut,
			partition.data()
		);

		OMNIFORCE_ASSERT_TAGGED(result == METIS_OK, "Graph partition failed");

		// If assertion is not triggered, then we successfully performed the edge-cut and can register partitions
		std::vector<std::vector<uint32>> groups(num_partitions);

		for (uint32 i = 0; i < meshlets.size(); i++) {
			idx_t group_index = partition[i];
			groups[group_index].push_back(i);
		}

		return groups;
	}

}