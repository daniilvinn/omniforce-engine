#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Renderer/Meshlet.h>

#include <glm/glm.hpp>

namespace Omni {

	struct ClusterizedMesh {
		std::vector<RenderableMeshlet> meshlets;
		std::vector<uint32> indices;
		std::vector<byte> local_indices;
		std::vector<MeshletBounds> cull_bounds;
	};

	class OMNIFORCE_API MeshPreprocessor {
	public:
		Scope<ClusterizedMesh> GenerateMeshlets(const std::vector<byte>* vertices, const std::vector<uint32>* indices, uint32 vertex_stride);
		Bounds GenerateMeshBounds(const std::vector<glm::vec3>* points);
		void OptimizeMesh(std::vector<byte>* out_vertices, std::vector<uint32>* out_indices, const std::vector<byte>* vertices, const std::vector<uint32>* indices, uint8 vertex_stride);
		void RemapVertices(std::vector<byte>* out_vertices, const std::vector<byte>* in_vertices, uint32 vertex_stride, const std::vector<uint32>* remap_table);
		void RemapVertices(std::vector<glm::vec3>* out_vertices, const std::vector<glm::vec3>* in_vertices, const std::vector<uint32>* remap_table);
		void ConvertToLineTopology(std::vector<byte>* out_vertices, const std::vector<byte>* in_vertices, uint32 vertex_stride);
		float32 GenerateMeshLOD(std::vector<uint32>* out_indices, const std::vector<byte>* vertex_data, const std::vector<uint32>* index_data, uint32 vertex_stride, uint32 target_index_count, float32 target_error, bool lock_borders);
		void SplitVertexData(std::vector<glm::vec3>* geometry, std::vector<byte>* attributes, const std::vector<byte>* in_vertex_data, uint32 stride);
		void GenerateShadowIndexBuffer(std::vector<uint32>* out, const std::vector<uint32>* indices, const std::vector<byte>* vertices, uint32 vertex_size, uint32 vertex_stride);
	};

}