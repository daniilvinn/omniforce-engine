#pragma once

#include "SceneCommon.h"
#include "Renderer/DeviceBuffer.h"
#include "Memory/VirtualMemoryBlock.h"
#include "Asset/Material.h"

namespace Omni {

	// Allocator with memory pool which holds all materials' data. Materials are accessed by BDA with offset relative to address of pool.
	class DeviceMaterialPool {
	public:
		DeviceMaterialPool(SceneRenderer* context, uint64 size);
		~DeviceMaterialPool();

		// @brief Allocates material in device memory
		// @return offset of the material data
		uint64 Allocate(AssetHandle material);

		// @brief Frees corresponding indices of the material in the pool
		void Free(AssetHandle material);

		uint64 GetStorageBufferAddress() const;

		uint64 GetOffset(AssetHandle material) const {
			OMNIFORCE_ASSERT_TAGGED(m_OffsetsMap.contains(material), "Attempted to get device material pool offset for material, \
				which was not previously allocated in the pool. Aborting execution");
			return m_OffsetsMap.at(material);
		}

	private:
		Shared<DeviceBuffer> m_PoolBuffer;
		Shared<DeviceBuffer> m_StagingForCopy;
		Scope<VirtualMemoryBlock> m_VirtualAllocator;
		rh::unordered_map<AssetHandle, uint32> m_OffsetsMap; // material id - offset map
		SceneRenderer* m_Context;
	};

}