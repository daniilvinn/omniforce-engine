#include "../VirtualMeshBuilder.h"
#include "../MeshPreprocessor.h"
#include <Log/Logger.h>
#include <Asset/MeshPreprocessor.h>
#include <Core/RandomNumberGenerator.h>
#include <Core/KDTree.h>

#include <unordered_map>
#include <fstream>
#include <span>

#include <metis.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <meshoptimizer.h>

namespace Omni {

	std::vector<Omni::MeshClusterGroup> VirtualMeshBuilder::GroupMeshClusters(
		const std::span<RenderableMeshlet>& meshlets,
		const std::vector<uint32>& welder_remap_table,
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
		for (uint32 meshlet_idx = 0; meshlet_idx < meshlets.size(); meshlet_idx++) {
			const auto& meshlet = meshlets[meshlet_idx];

			const uint32 triangle_count = meshlet.triangle_count;
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
			for (uint32 i = 0; i < meshlets.size(); i++) {
				group.push_back(i);
			}

			return { group };
		}

		idx_t graph_vertex_count = meshlets.size(); // graph is built from meshlets, hence graph vertex = meshlet
		idx_t num_constrains = 1;
		idx_t num_partitions = meshlets.size() / 4; // Group by 4 meshlets

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

		m_Mutex.lock();
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
		m_Mutex.unlock();

		OMNIFORCE_ASSERT_TAGGED(result == METIS_OK, "Graph partitioning failed!");

		// ===== Group meshlets together
		std::vector<MeshClusterGroup> groups;
		groups.resize(num_partitions);
		for (uint32 i = 0; i < meshlets.size(); i++) {
			idx_t partitionNumber = partition[i];
			groups[partitionNumber].push_back(i);
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

			glm::vec3* v0, *v1, *v2;
			v0 = (glm::vec3*)&vertices[i0 * vertex_stride];
			v1 = (glm::vec3*)&vertices[i1 * vertex_stride];
			v2 = (glm::vec3*)&vertices[i2 * vertex_stride];

			average_distance_sq += glm::distance2(*v0, *v1);
			average_distance_sq += glm::distance2(*v1, *v2);
			average_distance_sq += glm::distance2(*v0, *v2);
		}

		return average_distance_sq / (NUM_SAMPLES * 3);
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

		for (uint32 i = 0; i < meshlets_data->meshlets.size(); i++) {
			meshlets_data->meshlets[i].group = i;
		}

		// Global data
		uint32 lod_idx = 1;
		uint32 current_meshlets_offset = 0;
		float32 simplify_scale = meshopt_simplifyScale((float32*)vertices.data(), vertices.size() / vertex_stride, vertex_stride);
		// Compute max lod count. I expect each lod level to have 2 times less indices than
		// previous level. Added 2 if some levels won't be able to half index count
		const uint32 max_lod = glm::ceil(glm::log2(float32(indices.size()))); 
		//const uint32 max_lod = 5;

		std::vector<uint32> previous_lod_indices = indices;

		while (lod_idx < max_lod) {
			// Create a span of meshlets which are currently used as source meshlets for simplification. Also setup t_lod variable
			std::span<RenderableMeshlet> meshlets_to_group(meshlets_data->meshlets.begin() + current_meshlets_offset, meshlets_data->meshlets.end());
			float32 t_lod = float32(lod_idx) / (float32)max_lod;
			float32 min_vertex_distance = (t_lod * 0.1f + (1.0f - t_lod) * 0.01f) * simplify_scale;

			// 1. Find edge vertices (not indices)
			// 2. Compute average vertex distance for current source meshlets
			// 3. Generate welded remap table
			// 4. Clear index array of current meshlets, so next meshlets can properly fill new data
			std::vector<bool> edge_vertices_map = GenerateEdgeMap(meshlets_to_group, meshlets_data->indices, meshlets_data->local_indices, vertices.size() / vertex_stride);
			float32 average_vertex_distance_sq = ComputeAverageVertexDistanceSquared(vertices, previous_lod_indices, vertex_stride);

			// Build kd-tree
			KDTree current_lod_kd_tree = {};
			for (auto& index : previous_lod_indices)
				current_lod_kd_tree.AddPoint(*(glm::vec3*)&vertices[index * vertex_stride], index);

			std::vector<uint32> welder_remap_table = GenerateVertexWelderRemapTable(vertices, current_lod_kd_tree, vertex_stride, previous_lod_indices, edge_vertices_map, min_vertex_distance);
			previous_lod_indices.clear();

			// Generate meshlet groups
			auto groups = GroupMeshClusters(meshlets_to_group, welder_remap_table, meshlets_data->indices, meshlets_data->local_indices, vertices, vertex_stride);

			// Remove empty groups
			std::erase_if(groups, [](const auto& group) {
				return group.size() == 0;
			});

			// Process groups, also detect edges for next LOD generation pass
			uint32 num_newly_created_meshlets = 0;
			std::vector<RenderableMeshlet> new_meshlets;


			for (const auto& group : groups) {
				// Merge meshlets
				// Prepare storage
				std::vector<uint32> merged_indices;
				uint32 num_merged_indices = 0;

				for (auto& meshlet_idx : group)
					num_merged_indices += meshlets_data->meshlets[meshlet_idx].triangle_count * 3;

				merged_indices.reserve(num_merged_indices);

				// Merge
				for (auto& meshlet_idx : group) {
					RenderableMeshlet& meshlet = meshlets_data->meshlets[meshlet_idx + current_meshlets_offset];
					uint32 triangle_indices[3] = {};
					for (uint32 index_idx = 0; index_idx < meshlet.triangle_count * 3; index_idx++) {
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

				if (lod_idx == max_lod - 2)
					int bp = 5;

				// Skip if no indices after merging (maybe all triangles are degenerate?)
				if(merged_indices.size() == 0)
					continue;
				previous_lod_indices.insert(previous_lod_indices.end(), merged_indices.begin(), merged_indices.end());

				// Setup simplification parameters
				float32 target_error = 0.9f * t_lod + 0.01f * (1 - t_lod);
				float32 simplification_rate = 0.5f;
				std::vector<uint32> simplified_indices;

				// Generate LOD
				while (!simplified_indices.size() || simplified_indices.size() == merged_indices.size()) {
					mesh_preprocessor.GenerateMeshLOD(&simplified_indices, &vertices, &merged_indices, vertex_stride, merged_indices.size() * simplification_rate, target_error, true);
					target_error += 0.05f;
					simplification_rate += 0.05f;

					OMNIFORCE_ASSERT(simplified_indices.size());

					if (simplification_rate >= 1.0f && (simplified_indices.size() == merged_indices.size())) {
						OMNIFORCE_CORE_WARNING("Failed to generate LOD, registering source clusters");
						break;
					}
				}

				// Split back
				Scope<ClusterizedMesh> simplified_meshlets = mesh_preprocessor.GenerateMeshlets(&vertices, &simplified_indices, vertex_stride);

				uint32 group_idx = RandomEngine::Generate<uint32>();
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

		OMNIFORCE_CORE_TRACE("Mesh building finished. Final triangle count (including all LOD levels): {}", mesh.local_indices.size() / 3);

		return mesh;

	}

	// TODO: take UVs into account
	// Assumes that positions are encoded in first 12 bytes of vertices
	std::vector<uint32> VirtualMeshBuilder::GenerateVertexWelderRemapTable(const std::vector<byte>& vertices, const KDTree& kd_tree, uint32 vertex_stride, std::vector<uint32> lod_indices, const std::vector<bool>& edge_vertex_map, float32 min_vertex_distance_sq)
	{
		std::vector<uint32> remap_table(vertices.size() / vertex_stride);

		// Init remap table
		for (uint32 i = 0; i < remap_table.size(); i++)
			remap_table[i] = i;

		const uint32 vertex_count = vertices.size() / vertex_stride;
		remap_table.resize(vertex_count, uint32(-1)); // underflow on purpose

		for(auto& index : lod_indices) {
			// If it is edge vertex, we can't use it in welding process, therefore we just skip it
			if (edge_vertex_map[index])
				continue;

			float32 min_current_vertex_distance = min_vertex_distance_sq;

			const glm::vec3* vertex = (glm::vec3*)&vertices[index * vertex_stride];
			uint32 replacement = -1;

			// find neighbor here
			auto potential_replacements = kd_tree.FindNearestPoints(*vertex, min_current_vertex_distance, index);
			float32 distance_to_potential_vertex_squared = min_vertex_distance_sq * min_vertex_distance_sq;

			for (auto& potential_replacement : potential_replacements) {
				glm::vec3* potential_vertex_data = (glm::vec3*)&vertices[remap_table[potential_replacement] * vertex_stride];
				float32 dist_squared = glm::distance2(*vertex, *potential_vertex_data);

				if (distance_to_potential_vertex_squared > dist_squared) {
  					//replacement = potential_replacement;
					distance_to_potential_vertex_squared = dist_squared;
				}
			}

			if(replacement != -1)
				remap_table[index] = replacement;
			
		}

		// Sanity check, debug only
		if (OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG) {
			float32 max_distance = 0.0f;

			for (uint32 i = 0; i < vertex_count; i++) {
				glm::vec3* original_vertex = (glm::vec3*)&vertices[i * vertex_stride];
				glm::vec3* new_vertex = (glm::vec3*)&vertices[remap_table[i] * vertex_stride];

				OMNIFORCE_ASSERT_TAGGED(glm::distance(*original_vertex, *new_vertex) <= min_vertex_distance_sq, "Invalid welder remap table calculation");
			}
		}

		return remap_table;
	}

	std::vector<bool> VirtualMeshBuilder::GenerateEdgeMap(const std::span<RenderableMeshlet>& meshlets, const std::vector<uint32>& indices, const std::vector<uint8>& local_indices, uint32 vertex_count)
	{
		std::vector<bool> result(vertex_count, false);
		rh::unordered_map<MeshletEdge, uint32> edges;

		// for each cluster
		for (uint32 meshlet_idx = 0; meshlet_idx < meshlets.size(); meshlet_idx++) {
			const auto& meshlet = meshlets[meshlet_idx];

			const uint32 triangle_count = meshlet.triangle_count;
			// for each triangle of the cluster
			for (uint32 triangle_idx = 0; triangle_idx < triangle_count; triangle_idx++) {
				// for each edge of the triangle
				for (uint32 i = 0; i < 3; i++) {
					MeshletEdge edge(
						indices[local_indices[(i + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset],
						indices[local_indices[(((i + 1) % 3) + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]
					);

					edges[edge]++;

				}
			}
		}

		for (const auto& [edge, num_connections] : edges) {
			if (num_connections == 1) {
				result[edge.first] = true;
				result[edge.second] = true;
			}
		}

		return result;
	}

}