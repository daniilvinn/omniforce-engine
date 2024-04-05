#include "../MeshPreprocessor.h"

#include <glm/gtx/norm.hpp>
#include <meshoptimizer.h>

namespace Omni {

	GeneratedMeshlets* MeshPreprocessor::GenerateMeshlets(const std::vector<byte>* vertices, const std::vector<uint32>* indices, uint32 vertex_stride)
	{
		GeneratedMeshlets* meshlets_data = new GeneratedMeshlets;

		uint64 num_meshlets = meshopt_buildMeshletsBound(indices->size(), 64, 124);

		meshlets_data->meshlets.resize(num_meshlets);
		meshlets_data->indices.resize(num_meshlets * 64);
		meshlets_data->local_indices.resize(num_meshlets * 124 * 3);

		meshopt_Meshlet* meshlets = new meshopt_Meshlet[num_meshlets];

		uint64 actual_num_meshlets = meshopt_buildMeshlets(
			meshlets, 
			meshlets_data->indices.data(), 
			meshlets_data->local_indices.data(),
			indices->data(), 
			indices->size(), 
			(float32*)vertices->data(), 
			vertices->size() / vertex_stride, 
			vertex_stride, 
			64, 
			124, 
			0.5
		);

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
				(float32*)vertices->data(), 
				vertices->size() / vertex_stride,
				vertex_stride
			);

			memcpy(&meshlets_data->cull_bounds[idx], &bounds, sizeof MeshletCullBounds);

			idx++;
		}

		delete[] meshlets;

		return meshlets_data;
	}

	Bounds MeshPreprocessor::GenerateMeshBounds(const std::vector<glm::vec3>* points)
	{
		Bounds bounds = {};

		glm::vec3 farthest_vertex[2] = { points->at(0), points->at(1) };
		glm::vec3 averaged_vertex_position(0.0f);
		float32 radius[2] = { 0.0f, 0.0f };

		for (auto& point : *points) {
			averaged_vertex_position += point;
			for (int i = 0; i < 3; i++) {
				bounds.aabb.min[i] = glm::min(bounds.aabb.min[i], point[i]);
				bounds.aabb.max[i] = glm::max(bounds.aabb.max[i], point[i]);
			}
		}

		averaged_vertex_position /= points->size();
		glm::vec3 aabb_centroid = 0.5f * (bounds.aabb.min + bounds.aabb.max);

		// Second pass - find farthest vertices for both averaged vertex position and AABB centroid
		for (auto& point : *points) {
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

	void MeshPreprocessor::OptimizeMesh(std::vector<byte>* out_vertices, std::vector<uint32>* out_indices, const std::vector<byte>* vertices, const std::vector<uint32>* indices, uint8 vertex_stride)
	{
		std::vector<uint32> remap_table(vertices->size() / vertex_stride);
		uint32 unique_vertices = meshopt_generateVertexRemap(remap_table.data(), indices->data(), indices->size(), vertices->data(), vertices->size() / vertex_stride, vertex_stride);

		out_indices->resize(indices->size());
		out_vertices->resize(unique_vertices * vertex_stride);

		meshopt_remapIndexBuffer(out_indices->data(), indices->data(), out_indices->size(), remap_table.data());
		meshopt_remapVertexBuffer(out_vertices->data(), vertices->data(), vertices->size() / vertex_stride, vertex_stride, remap_table.data());

		meshopt_optimizeVertexCache(out_indices->data(), out_indices->data(), out_indices->size(), out_vertices->size() / vertex_stride);
		meshopt_optimizeOverdraw(out_indices->data(), out_indices->data(), out_indices->size(), (float32*)out_vertices->data(), out_vertices->size() / vertex_stride, vertex_stride, 1.05f);
		meshopt_optimizeVertexFetch(out_vertices->data(), out_indices->data(), out_indices->size(), out_vertices->data(), out_vertices->size() / vertex_stride, vertex_stride);
	}

	void MeshPreprocessor::RemapVertices(std::vector<byte>* out_vertices, const std::vector<byte>* in_vertices, uint32 vertex_stride, const std::vector<uint32>* remap_table)
	{
		for (uint32 i = 0; i < remap_table->size(); i++)
			memcpy(out_vertices->data() + vertex_stride * i, in_vertices->data() + vertex_stride * remap_table->at(i), vertex_stride);
	}

	void MeshPreprocessor::ConvertToLineTopology(std::vector<byte>* out_vertices, const std::vector<byte>* in_vertices, uint32 vertex_stride)
	{
		uint32 num_iterations = in_vertices->size() / vertex_stride;
		for (size_t i = 0; i < num_iterations; i += 3) {
			const byte* v0 = in_vertices->data() + vertex_stride * i;
			const byte* v1 = in_vertices->data() + vertex_stride * (i + 1);
			const byte* v2 = in_vertices->data() + vertex_stride * (i + 2);

			memcpy(out_vertices->data() + vertex_stride * (i * 2 + 0), v0, vertex_stride);
			memcpy(out_vertices->data() + vertex_stride * (i * 2 + 1), v1, vertex_stride);

			memcpy(out_vertices->data() + vertex_stride * (i * 2 + 2), v1, vertex_stride);
			memcpy(out_vertices->data() + vertex_stride * (i * 2 + 3), v2, vertex_stride);

			memcpy(out_vertices->data() + vertex_stride * (i * 2 + 4), v2, vertex_stride);
			memcpy(out_vertices->data() + vertex_stride * (i * 2 + 5), v0, vertex_stride);
		}
	}

	void MeshPreprocessor::GenerateMeshLOD(std::vector<uint32>* out_indices, const std::vector<byte>* vertex_data, const std::vector<uint32>* index_data, uint32 vertex_stride, uint32 target_index_count, float32 target_error) {
		out_indices->resize(index_data->size());
		out_indices->resize(meshopt_simplify(
			out_indices->data(),
			index_data->data(),
			index_data->size(),
			(float32*)vertex_data->data(),
			vertex_data->size() / vertex_stride,
			vertex_stride,
			target_index_count,
			target_error
		));
	}

	void MeshPreprocessor::SplitVertexData(std::vector<glm::vec3>* geometry, std::vector<byte>* attributes, const std::vector<byte>* in_vertex_data, uint32 vertex_stride)
	{
		uint32 num_iterations = in_vertex_data->size() / vertex_stride;
		uint32 deinterleaved_stride = vertex_stride - sizeof glm::vec3;

		for (int i = 0; i < num_iterations; i++) {
			memcpy(geometry->data() + i, in_vertex_data->data() + i * vertex_stride, sizeof(glm::vec3));
			memcpy(attributes->data() + i * deinterleaved_stride, in_vertex_data->data() + i * vertex_stride + sizeof(glm::vec3), deinterleaved_stride);
		}
	}

}