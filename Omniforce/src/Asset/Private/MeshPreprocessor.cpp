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
			memcpy(&meshlet, &meshlets[idx], sizeof(RenderableMeshlet));

			meshopt_Bounds bounds = meshopt_computeMeshletBounds(
				&meshlets_data->indices[meshlet.vertex_offset],
				&meshlets_data->local_indices[meshlet.triangle_offset],
				meshlet.triangles_count, 
				(float32*)vertices.data(), 
				vertices.size(),
				12
			);

			memcpy(&meshlets_data->cull_bounds[idx], &bounds, sizeof MeshletCullBounds);

			idx++;
		}

		delete[] meshlets;

		return meshlets_data;
	}

	Omni::Bounds MeshPreprocessor::GenerateMeshBounds(const std::vector<glm::vec3>& points)
	{
		Bounds bounds;

		glm::vec3 farthest_vertex[2] = { points[0], points[0] };
		glm::vec3 averaged_vertex_position(0.0f);
		float32 radius[2] = { 0.0f, 0.0f };

		for (auto& point : points) {
			averaged_vertex_position += point;
			for (int i = 0; i < 3; i++) {
				bounds.aabb.min[i] = glm::min(bounds.aabb.min[i], point[i]);
				bounds.aabb.max[i] = glm::max(bounds.aabb.max[i], point[i]);
			}
		}

		averaged_vertex_position /= points.size();
		glm::vec3 aabb_centroid = 0.5f * (bounds.aabb.min + bounds.aabb.max);

		// Second pass - find farthest vertices for both averaged vertex position and AABB centroid
		for (auto& point : points) {
			if (glm::distance2(averaged_vertex_position, point) > glm::distance2(averaged_vertex_position, farthest_vertex[0]))
				farthest_vertex[0] = point;
			if (glm::distance2(aabb_centroid, point) > glm::distance2(aabb_centroid, farthest_vertex[1]))
				farthest_vertex[1] = point;
		}

		float32 averaged_vertex_to_farthest_distance = glm::distance(farthest_vertex[0], averaged_vertex_position);
		float32 aabb_centroid_to_farthest_distance = glm::distance(farthest_vertex[1], aabb_centroid);

		bounds.sphere.center = averaged_vertex_to_farthest_distance < aabb_centroid_to_farthest_distance ? averaged_vertex_position : aabb_centroid;
		bounds.sphere.radius = glm::min(averaged_vertex_to_farthest_distance, aabb_centroid_to_farthest_distance);

		return bounds;
	}

	void MeshPreprocessor::OptimizeMesh(std::vector<byte>* out_vertices, std::vector<uint32>* out_indices, const std::vector<byte>& vertices, const std::vector<uint32>& indices, uint8 vertex_stride)
	{
		std::vector<uint32> remap_table(indices.size());
		uint32 unique_vertices = meshopt_generateVertexRemap(remap_table.data(), indices.data(), indices.size(), vertices.data(), vertices.size() / vertex_stride, vertex_stride);

		out_indices->resize(remap_table.size());
		out_vertices->resize(unique_vertices * vertex_stride);

		meshopt_remapIndexBuffer(out_indices->data(), indices.data(), out_indices->size(), remap_table.data());
		meshopt_remapVertexBuffer(out_vertices->data(), vertices.data(), vertices.size() / vertex_stride, vertex_stride, remap_table.data());

		meshopt_optimizeVertexCache(out_indices->data(), out_indices->data(), out_indices->size(), out_vertices->size() / vertex_stride);
		meshopt_optimizeOverdraw(out_indices->data(), out_indices->data(), out_indices->size(), (float32*)out_vertices->data(), out_vertices->size() / vertex_stride, vertex_stride, 1.05f);
		meshopt_optimizeVertexFetch(out_vertices->data(), out_indices->data(), out_indices->size(), out_vertices->data(), out_vertices->size() / vertex_stride, vertex_stride);
	}

	std::vector<glm::vec3> MeshPreprocessor::RemapVertices(const std::vector<glm::vec3>& src_vertices, const std::vector<uint32> remap_table)
	{
		std::vector<glm::vec3> result;
		result.reserve(remap_table.size());

		for (auto& index : remap_table)
			result.push_back(src_vertices[index]);

		return result;
	}

	std::vector<glm::vec3> MeshPreprocessor::ConvertToLineTopology(const std::vector<glm::vec3>& vertices)
	{
		std::vector<glm::vec3> linesVertices;
		linesVertices.reserve(vertices.size() * 1.4);

		for (size_t i = 0; i < vertices.size(); i += 3) {
			glm::vec3 v0 = vertices[i];
			glm::vec3 v1 = vertices[i + 1];
			glm::vec3 v2 = vertices[i + 2];

			linesVertices.push_back(v0);
			linesVertices.push_back(v1);

			linesVertices.push_back(v1);
			linesVertices.push_back(v2);

			linesVertices.push_back(v2);
			linesVertices.push_back(v0);
		}

		linesVertices.shrink_to_fit();

		return linesVertices;
	}

	std::vector<uint32> MeshPreprocessor::GenerateLOD(const std::vector<byte>& vertices, uint32 vertex_stride, const std::vector<uint32>& indices, uint32 target_index_count, float32 target_error, bool lock_borders)
	{
		std::vector<uint32> result(indices.size());

		float32 out_error = 0.0f;

		uint32 final_index_count = meshopt_simplify(
			result.data(),
			indices.data(),
			indices.size(),
			(float32*)vertices.data(),
			vertices.size() / vertex_stride,
			vertex_stride,
			target_index_count,
			target_error, 
			lock_borders ? meshopt_SimplifyLockBorder : 0, 
			&out_error
		);

		result.resize(final_index_count);

		return result;
	}

	void MeshPreprocessor::SplitVertexData(std::vector<glm::vec3>* geometry, std::vector<byte>* attributes, const std::vector<byte>& in_vertex_data, uint32 stride)
	{
		uint32 num_iterations = in_vertex_data.size() / stride;
		uint32 deinterleaved_stride = stride - sizeof glm::vec3;
		// split data back
		for (int i = 0; i < num_iterations; i++) {
			memcpy(geometry->data() + i, in_vertex_data.data() + i * stride, sizeof(glm::vec3));
			memcpy(attributes->data() + i * deinterleaved_stride, in_vertex_data.data() + i * stride + sizeof(glm::vec3), deinterleaved_stride);
		}
	}

}