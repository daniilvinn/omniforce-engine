#include "../MeshPreprocessor.h"

#include <glm/gtx/norm.hpp>
#include <meshoptimizer.h>
#include <ranges>

#include <Seb.h>
#include <Seb_point.h>

namespace Omni {

	Scope<ClusterizedMesh> MeshPreprocessor::GenerateMeshlets(const std::vector<byte>* vertices, const std::vector<uint32>* indices, uint32 vertex_stride)
	{
		Scope<ClusterizedMesh> meshlets_data = std::make_unique<ClusterizedMesh>();

		uint64 num_meshlets = meshopt_buildMeshletsBound(indices->size(), 64, 124);

		meshlets_data->meshlets.resize(num_meshlets);
		meshlets_data->indices.resize(num_meshlets * 64);
		meshlets_data->local_indices.resize(num_meshlets * 124 * 3);

		std::vector<meshopt_Meshlet> meshopt_meshlets(num_meshlets);

		uint64 actual_num_meshlets = meshopt_buildMeshlets(
			meshopt_meshlets.data(),
			meshlets_data->indices.data(), 
			meshlets_data->local_indices.data(),
			indices->data(), 
			indices->size(), 
			(float32*)vertices->data(), 
			vertices->size() / vertex_stride, 
			vertex_stride, 
			64, 
			124, 
			0.0
		);

		meshlets_data->meshlets.resize(actual_num_meshlets);
		meshlets_data->indices.resize(actual_num_meshlets * 64);
		meshlets_data->local_indices.resize(actual_num_meshlets * 124 * 3);

		meshlets_data->cull_bounds.resize(actual_num_meshlets);
		for (auto [meshlet, bounds, meshopt_meshlet] : std::views::zip(meshlets_data->meshlets, meshlets_data->cull_bounds, meshopt_meshlets)) {
			meshlet.vertex_offset = meshopt_meshlet.vertex_offset;
			meshlet.metadata.vertex_count = meshopt_meshlet.vertex_count;
			meshlet.triangle_offset = meshopt_meshlet.triangle_offset;
			meshlet.metadata.triangle_count = meshopt_meshlet.triangle_count;
			meshlet.vertex_bit_offset = 0;

			meshopt_Bounds meshopt_bounds = meshopt_computeMeshletBounds(
				&meshlets_data->indices[meshlet.vertex_offset],
				&meshlets_data->local_indices[meshlet.triangle_offset],
				meshlet.metadata.triangle_count,
				(float32*)vertices->data(), 
				vertices->size() / vertex_stride,
				vertex_stride
			);

			bounds.vis_culling_sphere =				{ glm::vec3(meshopt_bounds.center[0], meshopt_bounds.center[1], meshopt_bounds.center[2]), meshopt_bounds.radius }; // copy bounding sphere
			bounds.cone_apex =						glm::vec3(meshopt_bounds.cone_apex[0], meshopt_bounds.cone_apex[1], meshopt_bounds.cone_apex[2]);					// copy cone apex
			bounds.cone_axis =						glm::i8vec3(meshopt_bounds.cone_axis_s8[0], meshopt_bounds.cone_axis_s8[1], meshopt_bounds.cone_axis_s8[2]);		// copy s8 cone axis
			bounds.cone_cutoff =					meshopt_bounds.cone_cutoff_s8;																						// copy s8 cone cutoff
			bounds.lod_culling.sphere =				bounds.vis_culling_sphere;
			bounds.lod_culling.parent_sphere =		Sphere(glm::vec3(0.0f, 0.0f, 0.0f), +INFINITY);
			bounds.lod_culling.error =				0.0f;
			bounds.lod_culling.parent_error =		+INFINITY;
		}

		meshlets_data->indices.resize(meshlets_data->meshlets[meshlets_data->meshlets.size() - 1].vertex_offset + meshlets_data->meshlets[meshlets_data->meshlets.size() - 1].metadata.vertex_count);
		meshlets_data->local_indices.resize(meshlets_data->meshlets[meshlets_data->meshlets.size() - 1].triangle_offset + meshlets_data->meshlets[meshlets_data->meshlets.size() - 1].metadata.triangle_count * 3);

		return meshlets_data;
	}

	Bounds MeshPreprocessor::GenerateMeshBounds(const std::vector<glm::vec3>* points)
	{
		Bounds bounds = {};
		bounds.aabb.min = glm::vec3(FLT_MAX);
		bounds.aabb.max = glm::vec3(FLT_MIN);

		for (auto& point : *points) {
			for (int i = 0; i < 3; i++) {
				bounds.aabb.min[i] = glm::min(bounds.aabb.min[i], point[i]);
				bounds.aabb.max[i] = glm::max(bounds.aabb.max[i], point[i]);
			}
		}

#if 0
		namespace SEB = SEB_NAMESPACE;

		std::vector<SEB::Point<float32>> seb_points;
		std::vector<float32> vertex_points(3);
		for (auto& point : *points) {
			for (uint32 i = 0; i < 3; i++) {
				vertex_points[i] = point[i];
			}
			seb_points.push_back(SEB::Point<float32>(3, vertex_points.begin()));
		}

		SEB::Smallest_enclosing_ball<float32> seb(3, seb_points);
		auto center_position_iterator = seb.center_begin();

		for (uint32 i = 0; i < 3; i++) {
			bounds.sphere.center[i] = center_position_iterator[i];
		}
		bounds.sphere.radius = seb.radius();
#endif
		bounds.sphere.center = { 
			(bounds.aabb.min.x + bounds.aabb.max.x) * 0.5f, 
			(bounds.aabb.min.y + bounds.aabb.max.y) * 0.5f,
			(bounds.aabb.min.z + bounds.aabb.max.z) * 0.5f,
		};
		
		bounds.sphere.radius = glm::sqrt(
			glm::pow(bounds.aabb.max.x - bounds.aabb.min.x, 2) +
			glm::pow(bounds.aabb.max.y - bounds.aabb.min.y, 2) +
			glm::pow(bounds.aabb.max.z - bounds.aabb.min.z, 2)
		) * 0.5f;

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

	void MeshPreprocessor::RemapVertices(std::vector<glm::vec3>* out_vertices, const std::vector<glm::vec3>* in_vertices, const std::vector<uint32>* remap_table)
	{
		out_vertices->resize(remap_table->size());
		for (uint32 i = 0; i < remap_table->size(); i++)
			out_vertices[i] = in_vertices[remap_table->at(i)];
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

	Omni::float32 MeshPreprocessor::GenerateMeshLOD(std::vector<uint32>* out_indices, const std::vector<byte>* vertex_data, const std::vector<uint32>* index_data, uint32 vertex_stride, uint32 target_index_count, float32 target_error, bool lock_borders) {
		out_indices->resize(index_data->size());
		float32 result_error = 0.0f;
		out_indices->resize(meshopt_simplify(
			out_indices->data(),
			index_data->data(),
			index_data->size(),
			(float32*)vertex_data->data(),
			vertex_data->size() / vertex_stride,
			vertex_stride,
			target_index_count,
			target_error,
			lock_borders ? meshopt_SimplifyLockBorder : 0,
			&result_error
		));
		return result_error;
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

	void MeshPreprocessor::GenerateShadowIndexBuffer(std::vector<uint32>* out, const std::vector<uint32>* indices, const std::vector<byte>* vertices, uint32 vertex_size, uint32 vertex_stride)
	{
		out->resize(indices->size());
		meshopt_generateShadowIndexBuffer(
			out->data(),
			indices->data(),
			indices->size(),
			vertices->data(),
			vertices->size() / vertex_stride,
			vertex_size,
			vertex_stride
		);
	}

}