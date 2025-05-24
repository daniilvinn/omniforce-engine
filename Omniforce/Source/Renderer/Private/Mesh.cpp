#include <Foundation/Common.h>
#include <Renderer/Mesh.h>

#include <Renderer/AccelerationStructure.h>

#include <memory>

namespace Omni {

	Mesh::Mesh(IAllocator* allocator, MeshData& mesh_data, const AABB& aabb)
		: m_AABB(aabb)
	{
		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

		buffer_spec.size = mesh_data.geometry->GetNumStorageBytesUsed();
		m_Buffers.emplace(MeshBufferKey::GEOMETRY, DeviceBuffer::Create(allocator, buffer_spec, (void*)mesh_data.geometry->GetStorage(), buffer_spec.size));

		buffer_spec.size = mesh_data.attributes.size();
		m_Buffers.emplace(MeshBufferKey::ATTRIBUTES, DeviceBuffer::Create(allocator, buffer_spec, (void*)mesh_data.attributes.data(), buffer_spec.size));

		buffer_spec.size = mesh_data.meshlets.size() * sizeof RenderableMeshlet;
		m_Buffers.emplace(MeshBufferKey::MESHLETS, DeviceBuffer::Create(allocator, buffer_spec, (void*)mesh_data.meshlets.data(), buffer_spec.size));

		buffer_spec.size = mesh_data.local_indices.size();
		m_Buffers.emplace(MeshBufferKey::MICRO_INDICES, DeviceBuffer::Create(allocator, buffer_spec, (void*)mesh_data.local_indices.data(), buffer_spec.size));

		buffer_spec.size = mesh_data.cull_data.size() * sizeof MeshClusterBounds;
		m_Buffers.emplace(MeshBufferKey::MESHLETS_CULL_DATA, DeviceBuffer::Create(allocator, buffer_spec, (void*)mesh_data.cull_data.data(), buffer_spec.size));

		buffer_spec.size = mesh_data.ray_tracing.attributes.size();
		m_Buffers.emplace(MeshBufferKey::RT_ATTRIBUTES, DeviceBuffer::Create(allocator, buffer_spec, (void*)mesh_data.ray_tracing.attributes.data(), buffer_spec.size));
		
		buffer_spec.size = mesh_data.ray_tracing.indices.size() * sizeof(uint32);
		m_Buffers.emplace(MeshBufferKey::RT_INDICES, DeviceBuffer::Create(allocator, buffer_spec, (void*)mesh_data.ray_tracing.indices.data(), buffer_spec.size));

		m_MeshletCount = mesh_data.meshlets.size();
		m_BoundingSphere = mesh_data.bounding_sphere;
		m_QuantizationGridSize = mesh_data.quantization_grid_size;
		m_AttributeLayout = mesh_data.ray_tracing.layout;

		m_BLAS = std::move(mesh_data.acceleration_structure);
	}

	Mesh::~Mesh()
	{

	}

	Ref<Mesh> Mesh::Create(IAllocator* allocator, MeshData& mesh_data, const AABB& aabb)
	{
		return CreateRef<Mesh>(allocator, allocator, mesh_data, aabb);
	}

	void Mesh::Destroy()
	{
	}

}