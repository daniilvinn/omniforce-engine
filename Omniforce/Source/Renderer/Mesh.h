#pragma once

#include <Foundation/Common.h>

#include <Asset/AssetBase.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/Meshlet.h>
#include <Core/BitStream.h>

#include <map>

namespace Omni {

	enum class MeshBufferKey : uint8 {
		GEOMETRY,
		ATTRIBUTES,
		MESHLETS,
		MICRO_INDICES,
		MESHLETS_CULL_DATA
	};

	struct MeshData {
		Ptr<BitStream> geometry;
		std::vector<byte> attributes;
		std::vector<RenderableMeshlet> meshlets;
		std::vector<byte> local_indices;
		std::vector<MeshletBounds> cull_data;
		int32 quantization_grid_size;
		Sphere bounding_sphere;
	};

	class OMNIFORCE_API Mesh : public AssetBase {
	public:
		inline static constexpr uint32 OMNI_MAX_MESH_LOD_COUNT = 4;
	public:
		Mesh(IAllocator* allocator, const MeshData& lod0, const AABB& aabb);
		~Mesh();

		static Ref<Mesh> Create(IAllocator* allocator, const MeshData& lod0, const AABB& aabb);

		void Destroy() override;

		Ref<DeviceBuffer> GetBuffer(MeshBufferKey key) { return m_Buffers[key]; };
		const uint32& GetMeshletCount() const { return m_MeshletCount; }
		const int32& GetQuantizationGridSize() const { return m_QuantizationGridSize; }
		const Sphere& GetBoundingSphere() const { return m_BoundingSphere; }
		const AABB& GetAABB() const { return m_AABB; }

	private:
		std::map<MeshBufferKey, Ref<DeviceBuffer>> m_Buffers;
		uint32 m_MeshletCount = 0;
		int32 m_QuantizationGridSize = 0;
		Sphere m_BoundingSphere = {};
		AABB m_AABB = {}; // of lod 0
	};

}