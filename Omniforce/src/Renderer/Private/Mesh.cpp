#include "../Mesh.h"

#include <memory>
#include <vector>

#include <iostream>

namespace Omni {

	Mesh::Mesh(const MeshData& lod0, const AABB& aabb)
		: m_AABB(aabb)
	{
		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

		buffer_spec.size = lod0.geometry.size() * sizeof glm::vec3;
		m_LODs[0].buffers.emplace(MeshBufferKey::GEOMETRY, DeviceBuffer::Create(buffer_spec, (void*)lod0.geometry.data(), buffer_spec.size));

		buffer_spec.size = lod0.attributes.size();
		m_LODs[0].buffers.emplace(MeshBufferKey::ATTRIBUTES, DeviceBuffer::Create(buffer_spec, (void*)lod0.attributes.data(), buffer_spec.size));

		buffer_spec.size = lod0.meshlets.size() * sizeof RenderableMeshlet;
		m_LODs[0].buffers.emplace(MeshBufferKey::MESHLETS, DeviceBuffer::Create(buffer_spec, (void*)lod0.meshlets.data(), buffer_spec.size));

		buffer_spec.size = lod0.local_indices.size();
		m_LODs[0].buffers.emplace(MeshBufferKey::MICRO_INDICES, DeviceBuffer::Create(buffer_spec, (void*)lod0.local_indices.data(), buffer_spec.size));

		buffer_spec.size = lod0.cull_data.size() * sizeof MeshletCullBounds;
		m_LODs[0].buffers.emplace(MeshBufferKey::MESHLETS_CULL_DATA, DeviceBuffer::Create(buffer_spec, (void*)lod0.cull_data.data(), buffer_spec.size));

		m_LODs[0].meshlet_count = lod0.meshlets.size();
		m_LODs[0].bounding_sphere = lod0.bounding_sphere;

		m_LODCount = 1;
	}

	Mesh::Mesh(const std::array<MeshData, OMNI_MAX_MESH_LOD_COUNT>& lods, const AABB& aabb)
		: m_AABB(aabb)
	{
		int32 i = 0;
		for (auto& lod : lods) {
			DeviceBufferSpecification buffer_spec = {};
			buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
			buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
			buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

			buffer_spec.size = lod.geometry.size() * sizeof glm::vec3;
			m_LODs[i].buffers.emplace(MeshBufferKey::GEOMETRY, DeviceBuffer::Create(buffer_spec, (void*)lod.geometry.data(), buffer_spec.size));

			buffer_spec.size = lod.attributes.size();
			m_LODs[i].buffers.emplace(MeshBufferKey::ATTRIBUTES, DeviceBuffer::Create(buffer_spec, (void*)lod.attributes.data(), buffer_spec.size));

			buffer_spec.size = lod.meshlets.size() * sizeof RenderableMeshlet;
			m_LODs[i].buffers.emplace(MeshBufferKey::MESHLETS, DeviceBuffer::Create(buffer_spec, (void*)lod.meshlets.data(), buffer_spec.size));

			buffer_spec.size = lod.local_indices.size();
			m_LODs[i].buffers.emplace(MeshBufferKey::MICRO_INDICES, DeviceBuffer::Create(buffer_spec, (void*)lod.local_indices.data(), buffer_spec.size));

			buffer_spec.size = lod.cull_data.size() * sizeof MeshletCullBounds;
			m_LODs[i].buffers.emplace(MeshBufferKey::MESHLETS_CULL_DATA, DeviceBuffer::Create(buffer_spec, (void*)lod.cull_data.data(), buffer_spec.size));

			m_LODs[i].meshlet_count = lod.meshlets.size();
			m_LODs[i].bounding_sphere = lod.bounding_sphere;

			i++;
		}

		m_LODCount = 4;
	}

	Mesh::~Mesh()
	{

	}

	Shared<Mesh> Mesh::Create(const MeshData& lod0, const AABB& aabb)
	{
		return std::make_shared<Mesh>(lod0, aabb);
	}

	Shared<Mesh> Mesh::Create(const std::array<MeshData, OMNI_MAX_MESH_LOD_COUNT>& lods, const AABB& aabb)
	{

		return std::make_shared<Mesh>(lods, aabb);
	}

	void Mesh::Destroy()
	{
		for (auto& lod : m_LODs) 
			for(auto& buffer : lod.buffers)
				buffer.second->Destroy();
	}

}