#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Asset/AssetBase.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/Meshlet.h>

#include <map>

namespace Omni {

	enum class MeshBufferKey : uint8 {
		GEOMETRY,
		ATTRIBUTES,
		MESHLETS,
		MICRO_INDICES,
		MESHLETS_CULL_DATA
	};

	class OMNIFORCE_API Mesh : public AssetBase {
	public:
		Mesh(
			const std::vector<glm::vec3>& geometry,
			const std::vector<byte>& attributes,
			const std::vector<RenderableMeshlet>& meshlets,
			const std::vector<byte>& local_indices,
			const std::vector<MeshletCullBounds>& cull_data,
			const Sphere& bounding_sphere
		);
		~Mesh();
		static Shared<Mesh> Create(
			const std::vector<glm::vec3>& geometry,
			const std::vector<byte>& attributes,
			const std::vector<RenderableMeshlet>& meshlets,
			const std::vector<byte>& local_indices,
			const std::vector<MeshletCullBounds>& cull_data,
			const Sphere& bounding_sphere
		);

		void Destroy() override;

		Shared<DeviceBuffer> GetBuffer(MeshBufferKey key) { return m_Buffers.at(key); };
		const uint32& GetMeshletCount() const { return m_MeshletCount; }
		const Sphere& GetBoundingSphere() const { return m_BoundingSphere; }

	private:
		std::map<MeshBufferKey, Shared<DeviceBuffer>> m_Buffers;
		uint32 m_MeshletCount = 0;
		Sphere m_BoundingSphere;
	};

}