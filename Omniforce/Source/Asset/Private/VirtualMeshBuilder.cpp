#include <Foundation/Common.h>
#include <Asset/VirtualMeshBuilder.h>

#include <Foundation/RandomNumberGenerator.h>
#include <Asset/MeshPreprocessor.h>
#include <Core/KDTree.h>

#include <unordered_map>
#include <fstream>
#include <span>
#include <ranges>

#include <metis.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/packing.hpp>
#include <meshoptimizer.h>

namespace Omni {

	VirtualMesh VirtualMeshBuilder::BuildClusterGraph(const std::vector<byte>& vertices, const std::vector<uint32>& indices, uint32 vertex_stride, const VertexAttributeMetadataTable& vertex_metadata)
	{
		Timer timer;
		MeshPreprocessor mesh_preprocessor = {};

		// Clusterize initial mesh, basically build LOD 0
		Ptr<ClusterizedMesh> meshlets_data = mesh_preprocessor.GenerateMeshlets(&vertices, &indices, vertex_stride);

		// All mesh's meshlet groups
		// Init with source meshlets, 1 meshlet per group
		std::vector<MeshClusterGroup> mesh_cluster_groups(meshlets_data->meshlets.size());
		for (uint32 i = 0; i < mesh_cluster_groups.size(); i++)
			mesh_cluster_groups[i].push_back(i);

		// Global data
		uint32 lod_idx = 1; // initialized with 1 because we already have LOD 0 (source mesh)
		float32 simplify_scale = meshopt_simplifyScale((float32*)vertices.data(), vertices.size() / vertex_stride, vertex_stride);
		// Compute max lod count. I expect each lod level to have 2 times less indices than
		// previous level. Removed 5 levels because a single meshlet can hold up to 2^7 triangles + 2 lods
		// in case if some group won't be able to half the index count. Clamped for 
		// convenience.
		// So if a `max_lod` is X, it's value is:
		// x = ceil(log2(c)) - 7 + 2, where `c` is index count of source mesh
		const uint32 max_lod = glm::clamp((uint32)glm::ceil(glm::log2(float32(indices.size()))) - 5, 1u, 25u);

		// Init with source meshlets
		std::vector<uint32> previous_lod_meshlets(meshlets_data->meshlets.size());
		for (uint32 i = 0; i < previous_lod_meshlets.size(); i++)
			previous_lod_meshlets[i] = i;

		OMNIFORCE_CORE_INFO("Virtual mesh generation started. Max LOD count: {}", max_lod);

		// Use shared mutex for parallel group processing.
		// Acquire shared lock on reads, but a unique lock on writes so threads can concurrently read data
		std::shared_mutex mtx;
		
		while (lod_idx < max_lod) {
			LODGenerationPassStatistics stats = {};
			stats.input_meshlet_count = previous_lod_meshlets.size();

			float32 t_lod = float32(lod_idx) / (float32)max_lod;

			// 1. Find edge vertices (not indices)
			// 2. Compute average vertex distance for current source meshlets
			// 3. Generate welded remap table
			// 4. Clear index array of current meshlets, so next meshlets can properly fill new data
			std::vector<bool> edge_vertices_map = GenerateEdgeMap(meshlets_data->meshlets, previous_lod_meshlets, vertices, meshlets_data->indices, meshlets_data->local_indices, vertex_stride);

			// Evaluate unique indices to be welded
			rh::unordered_flat_set<uint32> unique_indices;

			// Fetch vertices to be welded. We can only use those vertices which are used in previous LOD level
			// Declare a mesh surface area, used further to compute minimal distance for vertices to be welded
			float64 total_surface_area = 0.0;
			for (auto& meshlet_idx : previous_lod_meshlets) {
				RenderableMeshlet& meshlet = meshlets_data->meshlets[meshlet_idx];

				glm::vec3 triangle_points[3] = {};
				// Fetch index data
				for (uint32 index_idx = 0; index_idx < meshlet.metadata.triangle_count * 3; index_idx++) {
					unique_indices.emplace(meshlets_data->indices[meshlet.vertex_offset + meshlets_data->local_indices[meshlet.triangle_offset + index_idx]]);

					// Fetch triangle points
					triangle_points[index_idx % 3] = Utils::FetchVertexFromBuffer(vertices, meshlets_data->indices[meshlet.vertex_offset + meshlets_data->local_indices[meshlet.triangle_offset + index_idx]], vertex_stride);

					// Compute triangle area and add it to the overall mesh surface area
					if (index_idx % 3 == 2) {
						total_surface_area += 0.5f * glm::length(glm::cross(triangle_points[1] - triangle_points[0], triangle_points[2] - triangle_points[0]));
					}
				}
			}

			std::vector<std::pair<glm::vec3, uint32>> vertices_to_weld;
			vertices_to_weld.reserve(unique_indices.size());

			for (auto& index : unique_indices)
				vertices_to_weld.push_back({ Utils::FetchVertexFromBuffer(vertices, index, vertex_stride), index });

			stats.input_vertex_count = vertices_to_weld.size();

			// Build acceleration structure to speed up vertex neighbors look up
			KDTree kd_tree;
			kd_tree.BuildFromPointSet(vertices_to_weld);

			// Compute average vertex density
			float32 vertex_density = (float64)indices.size() / total_surface_area;
			float32 min_vertex_distance = glm::sqrt(simplify_scale / vertex_density) * t_lod * 0.3f;
			float32 min_uv_distance = t_lod * 0.5f + (1.0f - t_lod) * 1.0f / 256.0f;

			stats.min_welder_vertex_distance = min_vertex_distance;
			stats.mesh_scale = simplify_scale;

			// Weld close enough vertices together
			std::vector<uint32> welder_remap_table = GenerateVertexWelderRemapTable(vertices, vertex_stride, kd_tree, unique_indices, edge_vertices_map, min_vertex_distance, min_uv_distance, vertex_metadata, stats);

			// Generate meshlet groups
			auto groups = GroupMeshClusters(meshlets_data->meshlets, previous_lod_meshlets, welder_remap_table, meshlets_data->indices, meshlets_data->local_indices, vertices, vertex_stride);
			stats.group_count = groups.size();

			// Remove empty groups
			std::erase_if(groups, [](const auto& group) {
				return group.size() == 0;
			});

			// Clear current meshlets indices, so we can register new ones
			previous_lod_meshlets.clear();

			// Process groups, also detect edges for next LOD generation pass
			uint32 num_newly_created_meshlets = 0;

			// Launch parallel processing of generated groups
			#pragma omp parallel for
			for (int32 group_idx = 0; group_idx < groups.size(); group_idx++) {
				const std::vector<uint32>& group = groups[group_idx];

				// Merge meshlets
				// Prepare storage

				// Group-local geometry data
				std::vector<uint32> group_to_mesh_space_vertex_remap;
				std::unordered_map<uint32, uint32> mesh_to_group_space_vertex_remap;
				std::vector<byte> group_local_vbo;

				// Other data
				std::vector<uint32> merged_indices;
				uint32 num_merged_indices = 0;

				mtx.lock_shared();
				for (auto& meshlet_idx : group)
					num_merged_indices += meshlets_data->meshlets[meshlet_idx].metadata.triangle_count * 3;
				mtx.unlock_shared();

				merged_indices.reserve(num_merged_indices);

				uint32 vbo_vertex_count = 0;

				// Allocate worst case memory
				group_local_vbo.resize(num_merged_indices * vertex_stride);

				// Merge
				uint32 test = 0;
				for (auto& meshlet_idx : group) {
					RenderableMeshlet& meshlet = meshlets_data->meshlets[meshlet_idx];
					uint32 triangle_indices[3] = {};
					for (uint32 index_idx = 0; index_idx < meshlet.metadata.triangle_count * 3; index_idx++) {
						mtx.lock_shared();
						triangle_indices[index_idx % 3] = welder_remap_table[meshlets_data->indices[meshlet.vertex_offset + meshlets_data->local_indices[meshlet.triangle_offset + index_idx]]];
						mtx.unlock_shared();

						if (index_idx % 3 == 2) {// last index of triangle was registered
							const bool is_triangle_degenerate = (triangle_indices[0] == triangle_indices[1] || triangle_indices[0] == triangle_indices[2] || triangle_indices[1] == triangle_indices[2]);

							// Check if triangle is degenerate
							// https://github.com/jglrxavpok/Carrot/blob/8f8bfe22c0a68cc55e74f04543c611f1120e06e5/asset_tools/fertilizer/gltf/GLTFProcessing.cpp
							if (!is_triangle_degenerate) {
								for (uint32 i = 0; i < 3; i++) {
									auto [iterator, was_new] = mesh_to_group_space_vertex_remap.try_emplace(triangle_indices[i]);
									test++;

									if (was_new) {
										iterator->second = vbo_vertex_count;
										memcpy(group_local_vbo.data() + (vertex_stride * vbo_vertex_count), vertices.data() + (vertex_stride * iterator->second), vertex_stride);
										vbo_vertex_count++;
									}
									merged_indices.push_back(iterator->second);
								}
							}
						}
					}
				}

				// Shrink to fit
				group_local_vbo.resize(vbo_vertex_count * vertex_stride);

				// Skip if no indices after merging (maybe all triangles are degenerate?)
				if (merged_indices.size() == 0)
					continue;
				
				// Create remap table to remap indices back to mesh space from group space
				group_to_mesh_space_vertex_remap = std::vector<uint32>(group_local_vbo.size() / vertex_stride, ~0ull);

				for (const auto& [mesh_space_idx, group_space_idx] : mesh_to_group_space_vertex_remap) {
					OMNIFORCE_ASSERT_TAGGED(group_space_idx < group_to_mesh_space_vertex_remap.size(), "Mismatched sizes");
					group_to_mesh_space_vertex_remap[group_space_idx] = mesh_space_idx;
				}

				// Setup simplification parameters
				const float32 target_error = 0.9f * t_lod + 0.01f * (1 - t_lod);
				const float32 simplification_rate = 0.5f;
				std::vector<uint32> simplified_group_indices;

				// Generate LOD
				bool lod_generation_failed = false;
				float32 result_error = 0.0f;
				result_error = mesh_preprocessor.GenerateMeshLOD(&simplified_group_indices, &group_local_vbo, &merged_indices, vertex_stride, merged_indices.size() * simplification_rate, target_error, true);

				OMNIFORCE_ASSERT(simplified_group_indices.size());

				// Failed to generate LOD for a given group. Re register meshlets and skip the group
				if (simplified_group_indices.size() == merged_indices.size()) {

					lod_generation_failed = true;
					for (const auto& meshlet_idx : group)
						previous_lod_meshlets.push_back(meshlet_idx);

					stats.group_simplification_failure_count++;

					continue;
				}

				// Remap indices back to mesh space
				for (auto& index : simplified_group_indices) {
					index = group_to_mesh_space_vertex_remap[index];
				}

				// Compute LOD culling bounding sphere for current simplified (!) group
				AABB group_aabb = { glm::vec3(+INFINITY), glm::vec3(-INFINITY)};

				for (const auto& index : simplified_group_indices) {
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
				mtx.lock_shared();
				for (const auto& child_meshlet_index : group) {
					max_children_error = std::max(meshlets_data->cull_bounds[child_meshlet_index].lod_culling.error, max_children_error);
				}
				mtx.unlock_shared();

				mesh_space_error += max_children_error;
				mtx.lock_shared();
				for (const auto& child_meshlet_index : group) {
					meshlets_data->cull_bounds[child_meshlet_index].lod_culling.parent_sphere = simplified_group_bounding_sphere;
					meshlets_data->cull_bounds[child_meshlet_index].lod_culling.parent_error = mesh_space_error;
				}
				mtx.unlock_shared();

				// Split back
				Ptr<ClusterizedMesh> simplified_meshlets = mesh_preprocessor.GenerateMeshlets(&vertices, &simplified_group_indices, vertex_stride);

				for (auto& bounds : simplified_meshlets->cull_bounds) {
					bounds.lod_culling.error = mesh_space_error;
					bounds.lod_culling.sphere = simplified_group_bounding_sphere;
				}

				// Acquire a unique(!!!) lock, so data patching is performed exclusively, so new meshlets acquire correct offsets within a buffer
				mtx.lock();

				// Patch data
				num_newly_created_meshlets += simplified_meshlets->meshlets.size();
				for (auto& meshlet : simplified_meshlets->meshlets) {
					meshlet.vertex_offset += meshlets_data->indices.size();
					meshlet.triangle_offset += meshlets_data->local_indices.size();
				}

				// Register new meshlets for next LOD generation pass
				for (uint32 i = 0; i < simplified_meshlets->meshlets.size(); i++)
					previous_lod_meshlets.push_back(meshlets_data->meshlets.size() + i);

				// Push group's to buffers
				meshlets_data->meshlets.insert(meshlets_data->meshlets.end(), simplified_meshlets->meshlets.begin(), simplified_meshlets->meshlets.end());
				meshlets_data->indices.insert(meshlets_data->indices.end(), simplified_meshlets->indices.begin(), simplified_meshlets->indices.end());
				meshlets_data->local_indices.insert(meshlets_data->local_indices.end(), simplified_meshlets->local_indices.begin(), simplified_meshlets->local_indices.end());
				meshlets_data->cull_bounds.insert(meshlets_data->cull_bounds.end(), simplified_meshlets->cull_bounds.begin(), simplified_meshlets->cull_bounds.end());
				mtx.unlock();

				// Update statistics
				stats.output_meshlet_count += simplified_meshlets->meshlets.size();
			}

			// Dump pass statistics
			OMNIFORCE_CORE_TRACE("Virtual mesh generation pass #{} finished. Statistics:", lod_idx);
			OMNIFORCE_CORE_TRACE("\tInput meshlet count: {}", stats.input_meshlet_count);
			OMNIFORCE_CORE_TRACE("\tOutput meshlet count: {}", stats.output_meshlet_count.load());
			OMNIFORCE_CORE_TRACE("\tGroup count: {}", stats.group_count);
			OMNIFORCE_CORE_TRACE("\tInput vertex count: {}", stats.input_vertex_count);
			OMNIFORCE_CORE_TRACE("\tWelded vertex count: {}", stats.welded_vertex_count);
			OMNIFORCE_CORE_TRACE("\tLocked vertex count: {}", stats.locked_vertex_count);
			OMNIFORCE_CORE_TRACE("\tGroup simplification failure count: {}", stats.group_simplification_failure_count.load());
			OMNIFORCE_CORE_TRACE("\tDegenerate triangle count: {}", stats.degenerate_triangles_erased.load());
			OMNIFORCE_CORE_TRACE("\tMin. welder vertex distance: {}", stats.min_welder_vertex_distance);
			OMNIFORCE_CORE_TRACE("\tMesh scale: {}", stats.mesh_scale);

			// If only 1 meshlet was created, finish mesh building - nothing to simplify further
			if (num_newly_created_meshlets == 1)
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

		OMNIFORCE_CORE_TRACE("Mesh building finished. Time taken: {}s. Final triangle count (including all LOD levels): {}", timer.ElapsedMilliseconds() / 1000.0f, mesh.local_indices.size() / 3);

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
		if (meshlet_indices.size() < 12)
			return { {meshlet_indices.begin(), meshlet_indices.end() } };

		// meshlets represented by their index into 'meshlets'
		std::unordered_map<MeshletEdge, std::vector<uint32>> edges_meshlets_map;
		std::unordered_map<uint32, std::vector<MeshletEdge>> meshlets_edges_map;
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
		idx_t num_partitions = meshlet_indices.size() / 6; // Group by 4 meshlets

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
						auto iterator = std::find(edge_adjacency.begin() + edge_adjacency_range_begin_iterator, edge_adjacency.end(), meshlet);
						if (iterator == edge_adjacency.end()) {
							// In case of first encounter, register this edge
							edge_adjacency.emplace_back(meshlet);
							edge_weights.emplace_back(1);
						}
						else {
							// In other case, just increment edge count
							std::ptrdiff_t pointer_distance = std::distance(edge_adjacency.begin(), iterator);

							OMNIFORCE_ASSERT_TAGGED(pointer_distance >= 0, "Pointer distance is less than zero");
							OMNIFORCE_ASSERT_TAGGED(pointer_distance < edge_weights.size(), "Edge weights and edge adjacency must have the same length");

							edge_weights[pointer_distance]++;
						}
					}
				}
			}
			x_adjacency.push_back(edge_adjacency_range_begin_iterator);
		}
		// Add last value
		x_adjacency.push_back(edge_adjacency.size());

		// Sanity check
		OMNIFORCE_ASSERT_TAGGED(x_adjacency.size() == meshlet_indices.size() + 1, "unexpected count of vertices for METIS graph: invalid xadj");
		OMNIFORCE_ASSERT_TAGGED(edge_adjacency.size() == edge_weights.size(), "Failed during METIS CSR graph generation: invalid adjncy / edgwgts");

		// Launch partition
		idx_t final_cut_cost = 0; // final cost of the cut found by METIS
		int result = -1;
		{
#ifndef METIS_HAS_THREADLOCAL
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

		// Fill the resulting data structure
		std::vector<MeshClusterGroup> groups;
		groups.resize(num_partitions);
		for (uint32 i = 0; i < meshlet_indices.size(); i++) {
			idx_t partitionNumber = partition[i];
			groups[partitionNumber].push_back(meshlet_indices[i]);
		}
		return groups;

	}

	// TODO: take UVs into account
	// Assumes that positions are encoded in first 12 bytes of vertices
	std::vector<Omni::uint32> VirtualMeshBuilder::GenerateVertexWelderRemapTable(
		const std::vector<byte>& vertices, 
		uint32 vertex_stride, 
		const KDTree& kd_tree, 
		const rh::unordered_flat_set<uint32>& lod_indices, 
		const std::vector<bool>& edge_vertex_map, 
		float32 min_vertex_distance, 
		float32 min_uv_distance, 
		const VertexAttributeMetadataTable& vertex_metadata, 
		LODGenerationPassStatistics& stats
	)
	{
		std::vector<uint32> remap_table(vertices.size() / vertex_stride);

		// Init remap table
		std::iota(remap_table.begin(), remap_table.end(), 0);

		// Init UVs offsets
		bool has_uvs = false;
		std::vector<uint8> uv_channels_offsets;
		for (const auto& metadata_entry : vertex_metadata) {
			if (metadata_entry.first.find("TEXCOORD") != std::string::npos) {
				has_uvs = true;
				uv_channels_offsets.push_back(metadata_entry.second);
			}
		}
	

		for (const auto& index : lod_indices) {
			if (edge_vertex_map[index]) {
				stats.locked_vertex_count++;
				continue;
			}

			// Fetch current vertex UVs
			const glm::vec3 current_vertex_position = Utils::FetchVertexFromBuffer(vertices, index, vertex_stride);
			std::vector<glm::vec2> current_vertex_uvs;

			// Init current vertex UVs
			for (const auto& uv_channel_offset : uv_channels_offsets) {
				current_vertex_uvs.push_back(glm::unpackHalf(Utils::FetchDataFromBuffer<glm::u16vec2>(vertices, index, uv_channel_offset, vertex_stride)));
			}

			float32 min_distance_sq = min_vertex_distance * min_vertex_distance;
			float32 min_uv_distance_sq = min_uv_distance * min_uv_distance;

			const std::vector<uint32> neighbour_indices = kd_tree.ClosestPointSet(current_vertex_position, min_vertex_distance, index);

			// Check neighbours
			uint32 replacement = index;
			for (const auto& neighbour : neighbour_indices) {
				const glm::vec3 neighbour_vertex_position = Utils::FetchVertexFromBuffer(vertices, remap_table[neighbour], vertex_stride);

				const glm::vec2 neighbour_vertex_uvs = Utils::FetchVertexFromBuffer(vertices, remap_table[neighbour], vertex_stride);

				const float32 vertex_distance_squared = glm::distance2(current_vertex_position, neighbour_vertex_position);
				if (vertex_distance_squared < min_distance_sq) {
					// Check UV distances
					bool uv_test_passed = true;
					for (uint32 uv_index = 0; const auto& uv_channel_offset : uv_channels_offsets) {
						glm::vec2 uv = glm::unpackHalf(Utils::FetchDataFromBuffer<glm::u16vec2>(vertices, remap_table[neighbour], uv_channel_offset, vertex_stride));

						float32 uv_distance_sq = glm::distance2(uv, current_vertex_uvs[uv_index]);
						if (uv_distance_sq <= min_uv_distance_sq) {
							min_distance_sq = uv_distance_sq;
						}
						else {
							uv_test_passed = false;
							break;
						}

						uv_index++;
					}

					if (uv_test_passed) {
						replacement = neighbour;
						min_distance_sq = vertex_distance_squared;
					}

				}
			}

			OMNIFORCE_ASSERT(lod_indices.contains(remap_table[replacement]));
			remap_table[index] = remap_table[replacement];

			// If a vertex was welded with itself, we don't recognize it as "vertex was welded"
			if(neighbour_indices.size())
				stats.welded_vertex_count++;
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
	
		for (const auto& meshlet_idx : current_meshlets) {
			const auto& meshlet = meshlets[meshlet_idx];

			const uint32 triangle_count = meshlet.metadata.triangle_count;
			
			for (uint32 triangle_idx = 0; triangle_idx < triangle_count; triangle_idx++) {
				
				for (uint32 i = 0; i < 3; i++) {
					// Use remap table which "eliminates" the attributes, because vertices might have different attributes (hence indices as well) but the same position - they must be locked
					MeshletEdge edge(
						geometry_only_indices[local_indices[(i + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset],
						geometry_only_indices[local_indices[(((i + 1) % 3) + triangle_idx * 3) + meshlet.triangle_offset] + meshlet.vertex_offset]
					);
					if (edge.first != edge.second) {
						edges[edge].emplace(meshlet_idx);
					}
				}
			}
		}

		// Fill the data
		for (const auto& [edge, meshlets] : edges) {
			if (meshlets.size() > 1) {
				result[edge.first] = true;
				result[edge.second] = true;
			}
		}

		return result;
	}

}