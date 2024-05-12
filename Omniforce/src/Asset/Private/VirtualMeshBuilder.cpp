#include "../VirtualMeshBuilder.h"
#include "../MeshPreprocessor.h"
#include <Log/Logger.h>
#include <Asset/MeshPreprocessor.h>

#include <unordered_map>
#include <fstream>
#include <span>

#include <metis.h>

namespace Omni {

	std::vector<std::vector<Omni::uint32>> VirtualMeshBuilder::GroupMeshClusters(
		const std::span<RenderableMeshlet>& meshlets,
		const std::span<MeshClusterGroup>& groups, 
		std::vector<uint32>& indices, 
		std::vector<uint8>& local_indices, 
		const std::vector<byte>& vertices, 
		uint32 vertex_stride
	) {
		OMNIFORCE_ASSERT_TAGGED(vertex_stride >= 12, "Vertex stride is less than 12: no vertex position data?");

		MeshPreprocessor mesh_preprocessor;

		std::unordered_map<MeshletEdge, std::vector<uint32>, MeshletEdgeHasher> edges_groups_map;
		std::unordered_map<uint32, std::vector<MeshletEdge>> groups_edges_map;

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

		// Find edges between meshlets
		
		// Per group
		uint32 num_unique_meshlets_in_groups = 0;
		uint32 group_idx = 0;
		for (auto& group : groups) {
			// per meshlet
			for (uint32 meshlet_idx = 0; meshlet_idx < group.size(); meshlet_idx++) {
				auto& meshlet = meshlets[group[meshlet_idx]];

				// per triangle
				for (uint32 triangle_idx = 0; triangle_idx < meshlet.triangle_count; triangle_idx++) {
					// per edge
					for (uint32 edge_idx = 0; edge_idx < 3; edge_idx++) {
						MeshletEdge edge(
							geometry_only_indices[local_indices[(edge_idx + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset],
							geometry_only_indices[local_indices[(((edge_idx + 1) % 3) + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]
						);

						auto& edge_groups = edges_groups_map[edge];
						auto& group_edges = groups_edges_map[group_idx];

						if (edge.first != edge.second) {
							// If edge-meshlets pair already contains current meshlet idx - don't register it
							if (std::find(edge_groups.begin(), edge_groups.end(), group_idx) == edge_groups.end())
								edge_groups.push_back(group_idx);

							// If meshlet-edges pair already contains an edge - don't register it
							if (std::find(group_edges.begin(), group_edges.end(), edge) == group_edges.end())
								group_edges.emplace_back(edge);
						}
					}
				}
			}
			num_unique_meshlets_in_groups += group.size();
			group_idx++;
		}

		// Generate a group with all groups if num groups is less than 8 (unable to partition)
		if (num_unique_meshlets_in_groups < 8) {
			std::vector<MeshClusterGroup> cluster_group(1);
			for (auto& group : groups) {
				for (auto& meshlet_index : group)
					cluster_group[0].push_back(meshlet_index);
			}

			return cluster_group;
		}

		// Remove non-shared edges
		std::erase_if(edges_groups_map, [](const auto& pair) {
			return pair.second.size() <= 1;
		});

		OMNIFORCE_ASSERT_TAGGED(edges_groups_map.size(), "No connections between clusters detected");
		OMNIFORCE_ASSERT_TAGGED(groups_edges_map.size(), "No cluster edges detected");

		// Prepare data for METIS graph cut
		idx_t cluster_graph_vertex_count = groups.size(); // Graph is built from groups, hence group = graph vertex
		idx_t num_constaints = 1; // Default minimal value
		idx_t num_partitions = num_unique_meshlets_in_groups / 4; // Make groups of 4 meshlets

		OMNIFORCE_ASSERT_TAGGED(num_partitions >= 2, "Num partitions must be greater or equal to 2");

		// Prepare xadj, adjncy and adjwgts
		std::vector<idx_t> partition;
		partition.resize(cluster_graph_vertex_count);

		// xadj
		std::vector<idx_t> x_adjacency;
		x_adjacency.reserve(cluster_graph_vertex_count + 1);

		// adjncy
		std::vector<idx_t> edge_adjacency;

		// weight of each edge. Weight of an edge = num shared edges between connected clusters
		std::vector<idx_t> edge_weights;

		// Per group
		for (uint32 group_idx = 0; group_idx < groups.size(); group_idx++) {
			uint32 first_adj_index = edge_adjacency.size();

			// Per edge
			for (const auto& edge : groups_edges_map[group_idx]) {
				auto connections_iterator = edges_groups_map.find(edge);

				if (connections_iterator == edges_groups_map.end())
					continue;

				const auto& connections = connections_iterator->second;

				// Per connection
				for (const auto& connected_cluster : connections) {
					if (connected_cluster != group_idx) {
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
							OMNIFORCE_ASSERT_TAGGED(pointer_difference < edge_weights.size(), "Edge weight and adjacency don't have the same size");

							edge_weights[pointer_difference]++;
						}
					}
				}
			}
			x_adjacency.push_back(first_adj_index);
		}
		x_adjacency.push_back(edge_adjacency.size());

		OMNIFORCE_ASSERT_TAGGED(x_adjacency.size() == cluster_graph_vertex_count + 1, "Invalid METIS graph build");

		for (auto& edge_adjacency_index : x_adjacency) {
			OMNIFORCE_ASSERT_TAGGED(edge_adjacency_index <= edge_adjacency.size(), "Out of bounds inside x_adjacency");
		}
		for (auto& vertex_index : edge_adjacency) {
			OMNIFORCE_ASSERT_TAGGED(vertex_index <= cluster_graph_vertex_count, "Out of bounds inside edge_adjacency");
		}

		// Setup METIS options for graph edge-cut operations
		idx_t options[METIS_NOPTIONS];
		METIS_SetDefaultOptions(options);

		options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
		options[METIS_OPTION_CCORDER] = 1;
		options[METIS_OPTION_NUMBERING] = 0;

		// Start METIS graph partition
		m_Mutex.lock();
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
		m_Mutex.unlock();

		OMNIFORCE_ASSERT_TAGGED(result == METIS_OK, "Graph partition failed");

		// If assertion is not triggered, then we successfully performed the edge-cut and can register partitions
		std::vector<std::vector<uint32>> partitions(num_partitions);

		for (uint32 i = 0; i < groups.size(); i++) {
			idx_t group_index = partition[i];
			for (auto& meshlet_index : groups[i]) {
				partitions[group_index].push_back(meshlet_index);
			}
		}

		return partitions;

	}

	VirtualMesh VirtualMeshBuilder::BuildClusterGraph(const std::vector<byte>& vertices, const std::vector<uint32>& indices, uint32 vertex_stride)
	{
		MeshPreprocessor mesh_preprocessor = {};

		// Clusterize initial mesh, basically build LOD 0
		Scope<ClusterizedMesh> meshlets_data = mesh_preprocessor.GenerateMeshlets(&vertices, &indices, vertex_stride);

		// All mesh's meshlet groups
		// Init with source meshlets, 1 meshlet per group
		std::vector<MeshClusterGroup> mesh_cluster_groups(meshlets_data->meshlets.size());
		for (uint32 i = 0; i < mesh_cluster_groups.size(); i++)
			mesh_cluster_groups[i].push_back(i);

		uint32 current_source_groups_offset = 0;
		uint32 current_source_meshlets_offset = 0;

		uint32 lod_idx = 0;
		while (true) {
			if (lod_idx == 1)
				break;

			std::span<MeshClusterGroup> previous_lod_groups(mesh_cluster_groups.begin() + current_source_groups_offset, mesh_cluster_groups.end());
			std::span<RenderableMeshlet> previous_lod_meshlets(meshlets_data->meshlets.begin(), meshlets_data->meshlets.end());

			std::vector<MeshClusterGroup> groups = GroupMeshClusters(
				previous_lod_meshlets,
				previous_lod_groups,
				meshlets_data->indices, 
				meshlets_data->local_indices, 
				vertices, 
				vertex_stride
			);

			current_source_groups_offset = mesh_cluster_groups.size();
			current_source_meshlets_offset = meshlets_data->meshlets.size();

			uint32 lod_cluster_count = 0;
			for (auto& group : groups) {
				if(group.size() == 0)
					continue;

				std::vector<uint32> group_indices = {};
				uint32 group_index_count = 0;

				// Preallocate just enough space
				for (auto& meshlet_idx : group)
					group_index_count += meshlets_data->meshlets[meshlet_idx].triangle_count * 3;

				group_indices.reserve(group_index_count);

				// Merge index data
				// Per meshlet in a group
				for (auto& meshlet_idx : group) {
					RenderableMeshlet& meshlet = meshlets_data->meshlets[meshlet_idx];

					// Per index
					for (uint32 i = 0; i < meshlet.triangle_count * 3; i++) {
						group_indices.push_back(FetchIndex(indices, meshlet.vertex_offset, meshlets_data->local_indices[meshlet.triangle_offset + i]));
					}

				}

				// Simplify group
				std::vector<uint32> simplified_group_indices = {};
				mesh_preprocessor.GenerateMeshLOD(
					&simplified_group_indices,
					&vertices,
					&group_indices,
					vertex_stride,
					group_indices.size() / 2,
					1000.0f,
					true
				);

				OMNIFORCE_ASSERT_TAGGED(simplified_group_indices.size(), "Failed to simplify cluster group");

				// Split group back to meshlets
				Scope<ClusterizedMesh> simplified_group_meshlets = mesh_preprocessor.GenerateMeshlets(&vertices, &simplified_group_indices, vertex_stride);
				
				for (uint32 simplified_meshlet_idx = 0; simplified_meshlet_idx < simplified_group_meshlets->meshlets.size(); simplified_meshlet_idx++) {
					RenderableMeshlet& simplified_meshlet = simplified_group_meshlets->meshlets.at(simplified_meshlet_idx);

					simplified_meshlet.vertex_offset += meshlets_data->indices.size();
					simplified_meshlet.triangle_offset += meshlets_data->local_indices.size();

				}
				
				// Update group meshlets
				group.clear();
				for (uint32 meshlet_idx = 0; meshlet_idx < simplified_group_meshlets->meshlets.size(); meshlet_idx++) {
					group.push_back(meshlet_idx + meshlets_data->meshlets.size());
				}

				meshlets_data->meshlets.insert(meshlets_data->meshlets.end(), simplified_group_meshlets->meshlets.begin(), simplified_group_meshlets->meshlets.end());
				meshlets_data->cull_bounds.insert(meshlets_data->cull_bounds.end(), simplified_group_meshlets->cull_bounds.begin(), simplified_group_meshlets->cull_bounds.end());
				meshlets_data->indices.insert(meshlets_data->indices.end(), simplified_group_meshlets->indices.begin(), simplified_group_meshlets->indices.end());
				meshlets_data->local_indices.insert(meshlets_data->local_indices.end(), simplified_group_meshlets->local_indices.begin(), simplified_group_meshlets->local_indices.end());

				for (uint32 simplified_meshlet_idx = 0; simplified_meshlet_idx < simplified_group_meshlets->meshlets.size(); simplified_meshlet_idx++) {
					RenderableMeshlet& simplified_meshlet = simplified_group_meshlets->meshlets.at(simplified_meshlet_idx);

					OMNIFORCE_ASSERT_TAGGED(simplified_meshlet.triangle_offset + simplified_meshlet.triangle_count * 3 <= meshlets_data->local_indices.size(), "OOB");

					for (uint32 local_index_idx = 0; local_index_idx < simplified_meshlet.triangle_count * 3; local_index_idx++) {
						OMNIFORCE_ASSERT_TAGGED(simplified_meshlet.triangle_offset + local_index_idx < meshlets_data->local_indices.size(), "OOB");
						uint32 local_index = meshlets_data->local_indices[simplified_meshlet.triangle_offset + local_index_idx];
						OMNIFORCE_ASSERT_TAGGED(local_index < simplified_meshlet.vertex_count, "OOB");
					}

				}

			}

			std::erase_if(groups, [](auto& value) {
				return value.size() == 0;
			});

			mesh_cluster_groups.insert(mesh_cluster_groups.end(), groups.begin(), groups.end());

			OMNIFORCE_CORE_TRACE("Generated LOD {}", lod_idx);

			if(groups.size() == 1)
				if(groups[0].size() <= 4)
					break;

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

	uint32 VirtualMeshBuilder::FetchIndex(const std::vector<uint32> indices, uint32 offset, uint8 local_index)
	{
		return indices[offset + local_index];
	}

}