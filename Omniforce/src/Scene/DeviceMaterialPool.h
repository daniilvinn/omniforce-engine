#pragma once

#include "SceneCommon.h"
#include "Renderer/DeviceBuffer.h"
#include "Asset/Material.h"

namespace Omni {

	// Linear allocator for material indices in GPU memory. It is linear allocator.
	class DeviceMaterialPool {
	public:
		DeviceMaterialPool(uint64 size);
		~DeviceMaterialPool();

		// @brief Allocates material in device memory
		// @return offset of the material data
		uint32 Allocate(Material* material);

		// @brief Frees corresponding indices of the material in the pool
		void Free(UUID material_id);

	private:
		DeviceBuffer* m_Buffer;
		robin_hood::unordered_map<UUID, uint32> m_Offsets;

	};

}