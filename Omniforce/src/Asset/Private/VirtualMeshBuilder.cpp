#include "../VirtualMeshBuilder.h"
#include "../MeshPreprocessor.h"
#include <Log/Logger.h>
#include <Asset/MeshPreprocessor.h>
#include <Core/RandomNumberGenerator.h>
#include <Core/KDTree.h>

#include <unordered_map>
#include <fstream>
#include <span>
#include <ranges>

#include <metis.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <meshoptimizer.h>

namespace Omni {

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

		// Global data
		uint32 lod_idx = 1; // initialized with 1 because we already have LOD 0 (source mesh)
		float32 simplify_scale = meshopt_simplifyScale((float32*)vertices.data(), vertices.size() / vertex_stride, vertex_stride);
		// Compute max lod count. I expect each lod level to have 2 times less indices than
		// previous level. Added 2 if some levels won't be able to half index count
		const uint32 max_lod = glm::ceil(glm::log2(float32(indices.size())));
		//const uint32 max_lod = 5;

		std::vector<uint32> previous_lod_indices = indices;
		std::vector<uint32> current_meshlets(meshlets_data->meshlets.size());
		for (uint32 i = 0; i < current_meshlets.size(); i++)
			current_meshlets[i] = i;

		while (lod_idx < max_lod) {
			float32 t_lod = float32(lod_idx) / (float32)max_lod;
			float32 min_vertex_distance = (t_lod * 0.1f + (1.0f - t_lod) * 0.01f) * simplify_scale;

			// 1. Find edge vertices (not indices)
			// 2. Compute average vertex distance for current source meshlets
			// 3. Generate welded remap table
			// 4. Clear index array of current meshlets, so next meshlets can properly fill new data
			std::vector<bool> edge_vertices_map = GenerateEdgeMap(meshlets_data->meshlets, current_meshlets, vertices, meshlets_data->indices, meshlets_data->local_indices, vertex_stride);
			float32 average_vertex_distance_sq = ComputeAverageVertexDistanceSquared(vertices, previous_lod_indices, vertex_stride);

			// Evaluate unique indices to be welded
			rh::unordered_flat_set<uint32> unique_indices;
			for (auto& index : previous_lod_indices)
				unique_indices.emplace(index);

			std::vector<std::pair<glm::vec3, uint32>> vertices_to_weld;
			vertices_to_weld.reserve(unique_indices.size());

			for (auto& index : unique_indices)
				vertices_to_weld.push_back({ *(glm::vec3*)&vertices[index * vertex_stride], index });

			KDTree kd_tree;
			kd_tree.BuildFromPointSet(vertices_to_weld);

			// Weld close enough vertices together
			std::vector<uint32> welder_remap_table = GenerateVertexWelderRemapTable(vertices, vertex_stride, kd_tree, unique_indices, edge_vertices_map, min_vertex_distance);
			previous_lod_indices.clear();

			// Generate meshlet groups
			auto groups = GroupMeshClusters(meshlets_data->meshlets, current_meshlets, welder_remap_table, meshlets_data->indices, meshlets_data->local_indices, vertices, vertex_stride);

			// Remove empty groups
			std::erase_if(groups, [](const auto& group) {
				return group.size() == 0;
			});

			// Clear current meshlets indices, so we can register new ones
			current_meshlets.clear();

			// Process groups, also detect edges for next LOD generation pass
			uint32 num_newly_created_meshlets = 0;

			for (const auto& group : groups) {
				// Merge meshlets
				// Prepare storage
				std::vector<uint32> merged_indices;
				uint32 num_merged_indices = 0;

				for (auto& meshlet_idx : group)
					num_merged_indices += meshlets_data->meshlets[meshlet_idx].metadata.triangle_count * 3;

				merged_indices.reserve(num_merged_indices);

				// Merge
				for (auto& meshlet_idx : group) {
					RenderableMeshlet& meshlet = meshlets_data->meshlets[meshlet_idx];
					uint32 triangle_indices[3] = {};
					for (uint32 index_idx = 0; index_idx < meshlet.metadata.triangle_count * 3; index_idx++) {
						triangle_indices[index_idx % 3] = welder_remap_table[meshlets_data->indices[meshlet.vertex_offset + meshlets_data->local_indices[meshlet.triangle_offset + index_idx]]];

						if (index_idx % 3 == 2) {// last index of triangle was registered
							bool is_triangle_degenerate = (triangle_indices[0] == triangle_indices[1] || triangle_indices[0] == triangle_indices[2] || triangle_indices[1] == triangle_indices[2]);

							// Check if triangle is degenerate
							if (!is_triangle_degenerate) {
								merged_indices.push_back(triangle_indices[0]);
								merged_indices.push_back(triangle_indices[1]);
								merged_indices.push_back(triangle_indices[2]);
							}
						}
					}
				}

				// Skip if no indices after merging (maybe all triangles are degenerate?)
				if (merged_indices.size() == 0)
					continue;
				previous_lod_indices.insert(previous_lod_indices.end(), merged_indices.begin(), merged_indices.end());

				// Setup simplification parameters
				float32 target_error = 0.9f * t_lod + 0.01f * (1 - t_lod);
				float32 simplification_rate = 0.5f;
				std::vector<uint32> simplified_indices;

				// Generate LOD
				bool lod_generation_failed = false;
				float32 result_error = 0.0f;
				while (!simplified_indices.size() || simplified_indices.size() == merged_indices.size()) {
					result_error = mesh_preprocessor.GenerateMeshLOD(&simplified_indices, &vertices, &merged_indices, vertex_stride, merged_indices.size() * simplification_rate, target_error, true);
					target_error += 0.05f;
					simplification_rate += 0.05f;

					OMNIFORCE_ASSERT(simplified_indices.size());

					// Failed to generate LOD for a given group. Re register meshlets and skip the group
					if (simplification_rate >= 1.0f && (simplified_indices.size() == merged_indices.size())) {
						OMNIFORCE_CORE_WARNING("Failed to generate LOD, registering source clusters");

						lod_generation_failed = true;
						for (const auto& meshlet_idx : group)
							current_meshlets.push_back(meshlet_idx);

						break;
					}
				}

				if (lod_generation_failed)
					continue;

				// Compute LOD culling bounding sphere for current simplified (!) group
				AABB group_aabb = { glm::vec3(+INFINITY), glm::vec3(-INFINITY)};

				for (const auto& index : simplified_indices) {
					const glm::vec3 vertex = Utils::FetchVertexFromBuffer(vertices, index, vertex_stride);

					group_aabb.max = glm::max(group_aabb.max, vertex);
					group_aabb.min = glm::min(group_aabb.min, vertex);
				}

				Sphere simplified_group_bounding_sphere = Utils::SphereFromAABB(group_aabb);

				// Compute error in mesh scale
				const float32 local_mesh_scale = meshopt_simplifyScale((float32*)vertices.data(), vertices.size() / vertex_stride, vertex_stride);
				float32 mesh_space_error = result_error * local_mesh_scale;

				// Find biggest error of children clusters
				float32 max_children_error = 0.0f;
				for (const auto& child_meshlet_index : group) {
					max_children_error = std::max(meshlets_data->cull_bounds[child_meshlet_index].lod_culling.error, max_children_error);
				}

				mesh_space_error += max_children_error;
				for (const auto& child_meshlet_index : group) {
					meshlets_data->cull_bounds[child_meshlet_index].lod_culling.parent_sphere = simplified_group_bounding_sphere;
					meshlets_data->cull_bounds[child_meshlet_index].lod_culling.parent_error = mesh_space_error;
				}

				// Split back
				Scope<ClusterizedMesh> simplified_meshlets = mesh_preprocessor.GenerateMeshlets(&vertices, &simplified_indices, vertex_stride);

				for (auto& bounds : simplified_meshlets->cull_bounds) {
					bounds.lod_culling.error = mesh_space_error;
					bounds.lod_culling.sphere = simplified_group_bounding_sphere;
				}

				for (const auto& simplified_bounds : simplified_meshlets->cull_bounds) {
					float32 source_max_error = 0.0f;
					for (const auto& source_meshlet_index : group) {
						source_max_error = std::max(source_max_error, meshlets_data->cull_bounds[source_meshlet_index].lod_culling.error);
					}
					OMNIFORCE_ASSERT_TAGGED(simplified_bounds.lod_culling.error >= source_max_error, "Invalid cluster group error evaluation during mesh build");
				}

				uint32 group_idx = RandomEngine::Generate<uint32>();
				num_newly_created_meshlets += simplified_meshlets->meshlets.size();
				for (auto& meshlet : simplified_meshlets->meshlets) {
					meshlet.vertex_offset += meshlets_data->indices.size();
					meshlet.triangle_offset += meshlets_data->local_indices.size();
				}
				for (uint32 i = 0; i < simplified_meshlets->meshlets.size(); i++)
					current_meshlets.push_back(meshlets_data->meshlets.size() + i);

				meshlets_data->meshlets.insert(meshlets_data->meshlets.end(), simplified_meshlets->meshlets.begin(), simplified_meshlets->meshlets.end());
				meshlets_data->indices.insert(meshlets_data->indices.end(), simplified_meshlets->indices.begin(), simplified_meshlets->indices.end());
				meshlets_data->local_indices.insert(meshlets_data->local_indices.end(), simplified_meshlets->local_indices.begin(), simplified_meshlets->local_indices.end());
				meshlets_data->cull_bounds.insert(meshlets_data->cull_bounds.end(), simplified_meshlets->cull_bounds.begin(), simplified_meshlets->cull_bounds.end());

			}

			if (num_newly_created_meshlets == 1)
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

		OMNIFORCE_CORE_TRACE("Mesh building finished. Final triangle count (including all LOD levels): {}", mesh.local_indices.size() / 3);

		return mesh;

	}

	std::vector<Omni::MeshClusterGroup> VirtualMeshBuilder::GroupMeshClusters(
		std::span<RenderableMeshlet> meshlets,
		std::span<uint32> meshlet_indices,
		const std::vector<uint32>& welder_remap_table,
		std::vector<uint32>& indices,
		std::vector<uint8>& local_indices,
		const std::vector<byte>& vertices,
		uint32 vertex_stride
	) {
		OMNIFORCE_ASSERT_TAGGED(vertex_stride >= 12, "Vertex stride is less than 12: no vertex position data?");

		// Early out if there is less than 8 meshlets (unable to partition)
		if (meshlet_indices.size() < 8)
			return { {meshlet_indices.begin(), meshlet_indices.end() } };

		// meshlets represented by their index into 'meshlets'
		std::unordered_map<MeshletEdge, std::vector<uint32>> edges_meshlets_map;
		std::unordered_map<uint32, std::vector<MeshletEdge>> meshlets_edges_map; // probably could be a vector
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
		for (uint32 meshlet_idx = 0; meshlet_idx < meshlet_indices.size(); meshlet_idx++) {
			const auto& meshlet = meshlets[meshlet_indices[meshlet_idx]];

			const uint32 triangle_count = meshlet.metadata.triangle_count;
			// for each triangle of the cluster
			for (uint32 triangle_idx = 0; triangle_idx < triangle_count; triangle_idx++) {
				// for each edge of the triangle
				for (uint32 i = 0; i < 3; i++) {
					MeshletEdge edge(
						welder_remap_table[geometry_only_indices[local_indices[(i + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]],
						welder_remap_table[geometry_only_indices[local_indices[(((i + 1) % 3) + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]]
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
			MeshClusterGroup group;
			for (uint32 i = 0; i < meshlet_indices.size(); i++) {
				group.push_back(i);
			}

			return { group };
		}

		idx_t graph_vertex_count = meshlet_indices.size(); // graph is built from meshlets, hence graph vertex = meshlet
		idx_t num_constrains = 1;
		idx_t num_partitions = meshlet_indices.size() / 4; // Group by 4 meshlets

		OMNIFORCE_ASSERT_TAGGED(num_partitions > 1, "Invalid partition count");

		idx_t metis_options[METIS_NOPTIONS];
		METIS_SetDefaultOptions(metis_options);

		// edge-cut
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

		for (uint32 meshlet_idx = 0; meshlet_idx < meshlet_indices.size(); meshlet_idx++) {
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

		OMNIFORCE_ASSERT_TAGGED(x_adjacency.size() == meshlet_indices.size() + 1, "unexpected count of vertices for METIS graph: invalid xadj");
		OMNIFORCE_ASSERT_TAGGED(edge_adjacency.size() == edge_weights.size(), "Failed during METIS CSR graph generation: invalid adjncy / edgwgts");

		idx_t final_cut_cost; // final cost of the cut found by METIS
		int result = -1;
		{
#ifndef BLABLA
			std::lock_guard lock(m_Mutex);
#endif
			result = METIS_PartGraphKway(
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
		}

		OMNIFORCE_ASSERT_TAGGED(result == METIS_OK, "Graph partitioning failed!");

		// ===== Group meshlets together
		std::vector<MeshClusterGroup> groups;
		groups.resize(num_partitions);
		for (uint32 i = 0; i < meshlet_indices.size(); i++) {
			idx_t partitionNumber = partition[i];
			groups[partitionNumber].push_back(meshlet_indices[i]);
		}
		return groups;

	}

	float32 VirtualMeshBuilder::ComputeAverageVertexDistanceSquared(const std::vector<byte> vertices, const std::vector<uint32>& indices, uint32 vertex_stride)
	{
		// Law of large numbers:
		// In probability theory, the law of large numbers (LLN) is a mathematical theorem that 
		// states that the average of the results obtained from a large number of independent 
		// and identical random samples converges to the true value, if it exists.

		// Sample 1024 random triangles and compute distance between its vertices. Dramatically speeds up vertex density computation for high-fidelity meshes
		// with large vertex and index counts
		OMNIFORCE_ASSERT_TAGGED(indices.size(), "No indices was provided");
		OMNIFORCE_ASSERT_TAGGED(vertices.size(), "No vertex data was provided");

		const uint32 NUM_SAMPLES = 1024;
		const uint32 triangle_count = indices.size() / 3;

		float64 average_distance_sq = 0.0f;

		for (uint32 i = 0; i < NUM_SAMPLES; i++) {
			uint32 triangle_sample_id = RandomEngine::Generate<uint32>(0, triangle_count - 1);
			uint32 i0 = indices[triangle_sample_id + 0];
			uint32 i1 = indices[triangle_sample_id + 1];
			uint32 i2 = indices[triangle_sample_id + 2];

			glm::vec3* v0, * v1, * v2;
			v0 = (glm::vec3*)&vertices[i0 * vertex_stride];
			v1 = (glm::vec3*)&vertices[i1 * vertex_stride];
			v2 = (glm::vec3*)&vertices[i2 * vertex_stride];

			average_distance_sq += glm::distance2(*v0, *v1);
			average_distance_sq += glm::distance2(*v1, *v2);
			average_distance_sq += glm::distance2(*v0, *v2);
		}

		return average_distance_sq / (NUM_SAMPLES * 3);
	}

	// TODO: take UVs into account
	// Assumes that positions are encoded in first 12 bytes of vertices
	std::vector<Omni::uint32> VirtualMeshBuilder::GenerateVertexWelderRemapTable(const std::vector<byte>& vertices, uint32 vertex_stride, const KDTree& kd_tree, const rh::unordered_flat_set<uint32>& lod_indices, const std::vector<bool>& edge_vertex_map, float32 min_vertex_distance)
	{
		std::vector<uint32> remap_table(vertices.size() / vertex_stride);

		// Init remap table
		for (uint32 i = 0; i < remap_table.size(); i++)
			remap_table[i] = i;

		for (const auto& index : lod_indices) {
			if (edge_vertex_map[index])
				continue;

			float32 min_distance_sq = min_vertex_distance * min_vertex_distance;
			const glm::vec3 current_vertex = Utils::FetchVertexFromBuffer(vertices, index, vertex_stride);

			const std::vector<uint32> neighbour_indices = kd_tree.ClosestPointSet(current_vertex, min_vertex_distance, index);

			uint32 replacement = index;
			for (const auto& neighbour : neighbour_indices) {
				const glm::vec3 neighbour_vertex = Utils::FetchVertexFromBuffer(vertices, neighbour, vertex_stride);

				const float32 vertex_distance_squared = glm::distance2(current_vertex, neighbour_vertex);
				if (vertex_distance_squared < min_distance_sq) {
					replacement = neighbour;
					min_distance_sq = vertex_distance_squared;
				}
			}

			OMNIFORCE_ASSERT(lod_indices.contains(remap_table[replacement]));
			remap_table[index] = remap_table[replacement];
		}

		return remap_table;
	}

	std::vector<bool> VirtualMeshBuilder::GenerateEdgeMap(
		std::span<RenderableMeshlet> meshlets,
		const std::span<uint32> current_meshlets,
		const std::vector<byte>& vertices,
		std::vector<uint32>& indices,
		const std::vector<uint8>& local_indices,
		uint32 vertex_stride
	)
	{
		std::vector<bool> result(vertices.size() / vertex_stride, false);
		rh::unordered_map<MeshletEdge, rh::unordered_set<uint32>> edges;

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
		
		// for each meshlet
		for (const auto& meshletIndex : current_meshlets) {
			const auto& meshlet = meshlets[meshletIndex];

			const uint32 triangleCount = meshlet.metadata.triangle_count / 3;
			// for each triangle of the meshlet
			for (uint32 triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++) {
				// for each edge of the triangle
				for (uint32 i = 0; i < 3; i++) {
					MeshletEdge edge(
						geometry_only_indices[indices[local_indices[(i + triangleIndex * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]],
						geometry_only_indices[indices[local_indices[(((i + 1) % 3) + triangleIndex * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]]
					);
					if (edge.first != edge.second) {
						edges[edge].emplace(meshletIndex);
					}
				}
			}
		}

		for (const auto& [edge, meshlets] : edges) {
			if (meshlets.size() == 1) {
				result[edge.first] = true;
				result[edge.second] = true;
			}
		}

		return result;
	}

}