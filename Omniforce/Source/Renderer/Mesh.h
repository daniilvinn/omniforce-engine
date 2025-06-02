#pragma once

#include <Foundation/Common.h>

#include <Core/BitStream.h>
#include <Asset/AssetBase.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/Meshlet.h>
#include <CodeGeneration/Device/MeshData.h>

#include <map>

namespace Omni {

	enum class MeshBufferKey : uint8 {
		GEOMETRY,
		ATTRIBUTES,
		MESHLETS,
		MICRO_INDICES,
		MESHLETS_CULL_DATA,
		RT_ATTRIBUTES,
		RT_INDICES
	};

	struct MeshData {
		struct {
			Ptr<BitStream> geometry;
			std::vector<byte> attributes;
			std::vector<RenderableMeshlet> meshlets;
			std::vector<byte> local_indices;
			std::vector<MeshClusterBounds> cull_data;
			int32 quantization_grid_size;
			Sphere bounding_sphere;
			bool use;
		} virtual_geometry;
		struct {
			std::vector<uint32> indices;
			GeometryLayoutTable layout;
			std::vector<byte> attributes;
		} ray_tracing;
		Ptr<RTAccelerationStructure> acceleration_structure;
	};

	class OMNIFORCE_API Mesh : public AssetBase {
	public:
		inline static constexpr uint32 OMNI_MAX_MESH_LOD_COUNT = 4;
	public:
		Mesh(IAllocator* allocator, MeshData& lod0, const AABB& aabb);
		~Mesh();

		static Ref<Mesh> Create(IAllocator* allocator, MeshData& mesh_data, const AABB& aabb);

		void Destroy() override;

		Ref<DeviceBuffer> GetBuffer(MeshBufferKey key) { return m_Buffers[key]; };
		const uint32& GetMeshletCount() const { return m_MeshletCount; }
		const int32& GetQuantizationGridSize() const { return m_QuantizationGridSize; }
		const Sphere& GetBoundingSphere() const { return m_BoundingSphere; }
		const AABB& GetAABB() const { return m_AABB; }
		GeometryLayoutTable GetAttributeLayout() const { return m_AttributeLayout; }
		WeakPtr<RTAccelerationStructure> GetAccelerationStructure() const { return m_BLAS; }
		bool IsVirtual() const { return m_IsVirtualizedGeometry; }

	private:
		std::map<MeshBufferKey, Ref<DeviceBuffer>> m_Buffers;
		Ptr<RTAccelerationStructure> m_BLAS;
		uint32 m_MeshletCount = 0;
		int32 m_QuantizationGridSize = 0;
		Sphere m_BoundingSphere = {};
		AABB m_AABB = {}; // of lod 0
		GeometryLayoutTable m_AttributeLayout = {};
		bool m_IsVirtualizedGeometry;
	};

}