#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Asset/AssetBase.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/Meshlet.h>
#include <Core/BitStream.h>

#include <Log/Logger.h>

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
		Scope<BitStream> geometry;
		std::vector<byte> attributes;
		std::vector<RenderableMeshlet> meshlets;
		std::vector<byte> local_indices;
		std::vector<MeshletCullBounds> cull_data;
		int32 quantization_grid_size;
		Sphere bounding_sphere;
	};

	class OMNIFORCE_API Mesh : public AssetBase {
	public:
		inline static constexpr uint32 OMNI_MAX_MESH_LOD_COUNT = 4;
	public:
		Mesh(const MeshData& lod0, const AABB& aabb);
		~Mesh();

		static Shared<Mesh> Create(const MeshData& lod0, const AABB& aabb);

		void Destroy() override;

		Shared<DeviceBuffer> GetBuffer(MeshBufferKey key) { return m_Buffers[key]; };
		const uint32& GetMeshletCount() const { return m_MeshletCount; }
		const int32& GetQuantizationGridSize() const { return m_QuantizationGridSize; }
		const Sphere& GetBoundingSphere() const { return m_BoundingSphere; }
		const AABB& GetAABB() const { return m_AABB; }

	private:
		std::map<MeshBufferKey, Shared<DeviceBuffer>> m_Buffers;
		uint32 m_MeshletCount = 0;
		int32 m_QuantizationGridSize = 0;
		Sphere m_BoundingSphere = {};
		AABB m_AABB = {}; // of lod 0
	};

}