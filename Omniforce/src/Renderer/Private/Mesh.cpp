#include "../Mesh.h"

#include <Log/Logger.h>

#include <memory>
#include <vector>
#include <iostream>


namespace Omni {

	Mesh::Mesh(const MeshData& mesh_data, const AABB& aabb)
		: m_AABB(aabb)
	{
		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

		buffer_spec.size = mesh_data.geometry->GetNumStorageBytesUsed();
		m_Buffers.emplace(MeshBufferKey::GEOMETRY, DeviceBuffer::Create(buffer_spec, (void*)mesh_data.geometry->GetStorage(), buffer_spec.size));

		buffer_spec.size = mesh_data.attributes.size();
		m_Buffers.emplace(MeshBufferKey::ATTRIBUTES, DeviceBuffer::Create(buffer_spec, (void*)mesh_data.attributes.data(), buffer_spec.size));

		buffer_spec.size = mesh_data.meshlets.size() * sizeof RenderableMeshlet;
		m_Buffers.emplace(MeshBufferKey::MESHLETS, DeviceBuffer::Create(buffer_spec, (void*)mesh_data.meshlets.data(), buffer_spec.size));

		buffer_spec.size = mesh_data.local_indices.size();
		m_Buffers.emplace(MeshBufferKey::MICRO_INDICES, DeviceBuffer::Create(buffer_spec, (void*)mesh_data.local_indices.data(), buffer_spec.size));

		buffer_spec.size = mesh_data.cull_data.size() * sizeof MeshletCullBounds;
		m_Buffers.emplace(MeshBufferKey::MESHLETS_CULL_DATA, DeviceBuffer::Create(buffer_spec, (void*)mesh_data.cull_data.data(), buffer_spec.size));

		m_MeshletCount = mesh_data.meshlets.size();
		m_BoundingSphere = mesh_data.bounding_sphere;
		m_QuantizationGridSize = mesh_data.quantization_grid_size;
	}

	Mesh::~Mesh()
	{

	}

	Shared<Mesh> Mesh::Create(const MeshData& lod0, const AABB& aabb)
	{
		return std::make_shared<Mesh>(lod0, aabb);
	}

	void Mesh::Destroy()
	{
		for (auto& buffer : m_Buffers)
			buffer.second->Destroy();
	}

}