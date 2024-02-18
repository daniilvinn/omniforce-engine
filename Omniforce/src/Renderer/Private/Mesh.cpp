#include "../Mesh.h"

#include <memory>
#include <vector>

#include <iostream>

namespace Omni {

	Mesh::Mesh(
		const std::vector<glm::vec3>& geometry,
		const std::vector<byte>& attributes,
		const std::vector<RenderableMeshlet>& meshlets,
		const std::vector<byte>& local_indices,
		const std::vector<MeshletCullBounds>& cull_data,
		const Sphere& bounding_sphere
	)
	{
		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

		buffer_spec.size = geometry.size() * sizeof glm::vec3;
		m_Buffers.emplace(MeshBufferKey::GEOMETRY, DeviceBuffer::Create(buffer_spec, (void*)geometry.data(), buffer_spec.size));

		buffer_spec.size = attributes.size();
		m_Buffers.emplace(MeshBufferKey::ATTRIBUTES, DeviceBuffer::Create(buffer_spec, (void*)attributes.data(), buffer_spec.size));

		buffer_spec.size = meshlets.size() * sizeof RenderableMeshlet;
		m_Buffers.emplace(MeshBufferKey::MESHLETS, DeviceBuffer::Create(buffer_spec, (void*)meshlets.data(), buffer_spec.size));

		buffer_spec.size = local_indices.size();
		m_Buffers.emplace(MeshBufferKey::MICRO_INDICES, DeviceBuffer::Create(buffer_spec, (void*)local_indices.data(), buffer_spec.size));

		buffer_spec.size = cull_data.size() * sizeof MeshletCullBounds;
		m_Buffers.emplace(MeshBufferKey::MESHLETS_CULL_DATA, DeviceBuffer::Create(buffer_spec, (void*)cull_data.data(), buffer_spec.size));

		m_MeshletCount = meshlets.size();
		m_BoundingSphere = bounding_sphere;
	}

	Mesh::~Mesh()
	{

	}

	Shared<Mesh> Mesh::Create(
		const std::vector<glm::vec3>& geometry,
		const std::vector<byte>& attributes,
		const std::vector<RenderableMeshlet>& meshlets,
		const std::vector<byte>& local_indices,
		const std::vector<MeshletCullBounds>& cull_data,
		const Sphere& bounding_sphere
	)
	{
		return std::make_shared<Mesh>(geometry, attributes, meshlets, local_indices, cull_data, bounding_sphere);
	}

	void Mesh::Destroy()
	{
		for (auto& buffer : m_Buffers)
			buffer.second->Destroy();
	}

}