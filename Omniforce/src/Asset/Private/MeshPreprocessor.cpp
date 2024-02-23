#include "../MeshPreprocessor.h"

#include <glm/gtx/norm.hpp>
#include <meshoptimizer.h>

namespace Omni {

	GeneratedMeshlets* MeshPreprocessor::GenerateMeshlets(const std::vector<glm::vec3>& vertices, const std::vector<uint32>& indices)
	{
		GeneratedMeshlets* meshlets_data = new GeneratedMeshlets;

		uint64 num_meshlets = meshopt_buildMeshletsBound(indices.size(), 64, 124);

		meshlets_data->meshlets.resize(num_meshlets);
		meshlets_data->indices.resize(num_meshlets * 64);
		meshlets_data->local_indices.resize(num_meshlets * 124 * 3);

		meshopt_Meshlet* meshlets = new meshopt_Meshlet[num_meshlets];

		uint64 actual_num_meshlets = meshopt_buildMeshlets(meshlets, meshlets_data->indices.data(), meshlets_data->local_indices.data(),
			indices.data(), indices.size(), (float32*)vertices.data(), vertices.size(), 12, 64, 124, 0.5);

		meshlets_data->meshlets.resize(actual_num_meshlets);
		meshlets_data->indices.resize(actual_num_meshlets * 64);
		meshlets_data->local_indices.resize(actual_num_meshlets * 124 * 3);

		meshlets_data->cull_bounds.resize(actual_num_meshlets);
		uint32 idx = 0;
		for (auto& meshlet : meshlets_data->meshlets) {
			meshlet.index_offset = meshlets[idx].vertex_offset;
			meshlet.triangles_count = meshlets[idx].triangle_count;
			meshlet.local_index_offset = meshlets[idx].triangle_offset;

			std::vector<uint32> indices(64);
			for (auto& index : indices) {
				index = *(meshlets_data->indices.data() + meshlets[idx].vertex_offset) - meshlets[idx].vertex_offset;
			}

			// TODO bug here, passing invalid data
			meshopt_Bounds bounds = meshopt_computeMeshletBounds(
				&meshlets_data->indices[meshlet.index_offset],
				&meshlets_data->local_indices[meshlet.local_index_offset],
				meshlet.triangles_count, 
				(float*)vertices.data(), 
				vertices.size(),
				12
			);

			memcpy(&meshlets_data->cull_bounds[idx], &bounds, sizeof MeshletCullBounds);

			idx++;
		}

		delete[] meshlets;

		return meshlets_data;
	}

	Sphere MeshPreprocessor::GenerateBoundingSphere(const std::vector<glm::vec3>& points)
	{
		Sphere sphere = {};
		AABB aabb = {};

		glm::vec3 farthest_vertex[2] = { points[0], points[0] };
		glm::vec3 averaged_vertex_position(0.0f);
		float radius[2] = { 0.0f, 0.0f };

		for (auto& point : points) {
			averaged_vertex_position += point;
			for (int i = 0; i < 3; i++) {
				aabb.min[i] = glm::min(aabb.min[i], point[i]);
				aabb.max[i] = glm::max(aabb.max[i], point[i]);
			}
		}

		averaged_vertex_position /= points.size();
		glm::vec3 aabb_centroid = 0.5f * (aabb.min + aabb.max);

		// Second pass - find farthest vertices for both averaged vertex position and AABB centroid
		for (auto& point : points) {
			if (glm::distance2(averaged_vertex_position, point) > glm::distance2(averaged_vertex_position, farthest_vertex[0]))
				farthest_vertex[0] = point;
			if (glm::distance2(aabb_centroid, point) > glm::distance2(aabb_centroid, farthest_vertex[1]))
				farthest_vertex[1] = point;
		}

		float averaged_vertex_to_farthest_distance = glm::distance(farthest_vertex[0], averaged_vertex_position);
		float aabb_centroid_to_farthest_distance = glm::distance(farthest_vertex[1], aabb_centroid);

		sphere.center = averaged_vertex_to_farthest_distance < aabb_centroid_to_farthest_distance ? averaged_vertex_position : aabb_centroid;
		sphere.radius = glm::min(averaged_vertex_to_farthest_distance, aabb_centroid_to_farthest_distance);

		return sphere;
	}

	void MeshPreprocessor::OptimizeMesh(std::vector<byte>& vertices, std::vector<uint32>& indices, uint8 vertex_stride)
	{
		std::vector<uint32> remap_table(indices.size());
		uint32 unique_vertices = meshopt_generateVertexRemap(remap_table.data(), indices.data(), indices.size(), vertices.data(), vertices.size() / vertex_stride, vertex_stride);

		std::vector<uint32> remapped_ib(remap_table.size());
		std::vector<byte> remapped_vb(unique_vertices * vertex_stride);

		meshopt_remapIndexBuffer(remapped_ib.data(), indices.data(), remapped_ib.size(), remap_table.data());
		meshopt_remapVertexBuffer(remapped_vb.data(), vertices.data(), vertices.size() / vertex_stride, vertex_stride, remap_table.data());

		meshopt_optimizeVertexCache(remapped_ib.data(), remapped_ib.data(), remapped_ib.size(), remapped_vb.size() / vertex_stride);
		meshopt_optimizeOverdraw(remapped_ib.data(), remapped_ib.data(), remapped_ib.size(), (float*)remapped_vb.data(), remapped_vb.size() / vertex_stride, vertex_stride, 1.05f);
		meshopt_optimizeVertexFetch(remapped_vb.data(), remapped_ib.data(), remapped_ib.size(), remapped_vb.data(), remapped_vb.size() / vertex_stride, vertex_stride);

		vertices.resize(remapped_vb.size());
		memcpy(vertices.data(), remapped_vb.data(), remapped_vb.size());
		memcpy(indices.data(), remapped_ib.data(), remapped_ib.size() * sizeof(uint32));
	}

}