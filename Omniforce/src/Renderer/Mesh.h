#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Asset/AssetBase.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/Meshlet.h>

#include <map>
#include <array>

namespace Omni {

	enum class MeshBufferKey : uint8 {
		GEOMETRY,
		ATTRIBUTES,
		MESHLETS,
		MICRO_INDICES,
		MESHLETS_CULL_DATA
	};

	struct MeshData {
		std::vector<glm::vec3> geometry;
		std::vector<byte> attributes;
		std::vector<RenderableMeshlet> meshlets;
		std::vector<byte> local_indices;
		std::vector<MeshletCullBounds> cull_data;
		Sphere& bounding_sphere;
	};

	struct MeshLODData {
		std::map<MeshBufferKey, Shared<DeviceBuffer>> buffers;
		uint32 meshlet_count = 0;
		Sphere bounding_sphere;
	};

	class OMNIFORCE_API Mesh : public AssetBase {
	public:
		inline static constexpr uint32 OMNI_MAX_MESH_LOD_COUNT = 4;
	public:
		Mesh(const MeshData& lod0, const AABB& aabb ); // create a mesh with only 1 lod
		Mesh(const std::array<MeshData, OMNI_MAX_MESH_LOD_COUNT>& lods, const AABB& aabb);
		~Mesh();

		static Shared<Mesh> Create(const MeshData& lod0, const AABB& aabb);
		static Shared<Mesh> Create(const std::array<MeshData, OMNI_MAX_MESH_LOD_COUNT>& lods, const AABB& aabb);

		void Destroy() override;

		Shared<DeviceBuffer> GetBuffer(uint32 lod_index, MeshBufferKey key) { OMNIFORCE_ASSERT_TAGGED(lod_index < OMNI_MAX_MESH_LOD_COUNT, "Exceeded max lod count"); return m_LODs[lod_index].buffers.at(key); };
		const uint32& GetMeshletCount(uint32 lod_index) const { OMNIFORCE_ASSERT_TAGGED(lod_index < OMNI_MAX_MESH_LOD_COUNT, "Exceeded max lod count"); return m_LODs[lod_index].meshlet_count; }
		const Sphere& GetBoundingSphere(uint32 lod_index) const { OMNIFORCE_ASSERT_TAGGED(lod_index < OMNI_MAX_MESH_LOD_COUNT, "Exceeded max lod count"); return m_LODs[lod_index].bounding_sphere; }
		const AABB& GetAABB() const { return m_AABB; }
		uint8 GetLODCount() const { return m_LODCount; }

	private:
		std::array<MeshLODData, OMNI_MAX_MESH_LOD_COUNT> m_LODs;
		AABB m_AABB; // of lod 0
		uint8 m_LODCount;
	};

}