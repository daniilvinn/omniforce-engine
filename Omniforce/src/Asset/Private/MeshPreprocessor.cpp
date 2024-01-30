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

		uint64 actual_num_meshlets = meshopt_buildMeshlets((meshopt_Meshlet*)meshlets_data->meshlets.data(), meshlets_data->indices.data(), meshlets_data->local_indices.data(),
			indices.data(), indices.size(), (float*)vertices.data(), vertices.size(), 12, 64, 124, 0.5);

		meshlets_data->meshlets.resize(actual_num_meshlets);
		meshlets_data->indices.resize(actual_num_meshlets * 64);
		meshlets_data->local_indices.resize(actual_num_meshlets * 124 * 3);

		meshlets_data->cull_bounds.resize(actual_num_meshlets);
		uint32 idx = 0;
		for (auto& meshlet : meshlets_data->meshlets) {
			meshopt_Bounds bounds = meshopt_computeMeshletBounds(meshlets_data->indices.data() + meshlet.index_offset, meshlets_data->local_indices.data(), meshlet.local_index_count, (float*)vertices.data(), vertices.size(), 12);
			meshlets_data->cull_bounds[idx] = *(MeshletCullBounds*)(& bounds);
			idx++;
		}

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

	void MeshPreprocessor::OptimizeMesh(std::vector<glm::vec3>& vertices, std::vector<uint32>& indices)
	{
		std::vector<uint32> remap_table(indices.size());
		uint32 unique_vertices = meshopt_generateVertexRemap(remap_table.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), 12);

		std::vector<glm::vec3> remapped_vb(unique_vertices);
		std::vector<uint32> remapped_ib(remap_table.size());

		meshopt_remapIndexBuffer(remapped_ib.data(), indices.data(), remapped_ib.size(), remap_table.data());
		meshopt_remapVertexBuffer(remapped_vb.data(), vertices.data(), vertices.size(), 12, remap_table.data());

		meshopt_optimizeVertexCache(remapped_ib.data(), remapped_ib.data(), remapped_ib.size(), remapped_vb.size());
		meshopt_optimizeOverdraw(remapped_ib.data(), remapped_ib.data(), remapped_ib.size(), (float*)remapped_vb.data(), remapped_vb.size(), 12, 1.05f);

		memcpy(vertices.data(), remapped_vb.data(), remapped_vb.size() * sizeof(glm::vec3));
		memcpy(indices.data(), remapped_ib.data(), remapped_ib.size() * sizeof(uint32));
	}

}