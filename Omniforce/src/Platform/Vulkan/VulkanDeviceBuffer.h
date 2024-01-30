#pragma once

#include "VulkanCommon.h"

#include <Renderer/DeviceBuffer.h>

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.h>

namespace Omni {

	struct IndexBufferData {
		uint32 index_count;
		VkIndexType index_type;
	};

	struct VertexBufferData {
		uint32 vertex_count;
	};

	constexpr VkIndexType ExtractIndexType(BitMask buffer_flags) {
		if (buffer_flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT8)		return VK_INDEX_TYPE_UINT8_EXT;
		if (buffer_flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT16)	return VK_INDEX_TYPE_UINT16;
		if (buffer_flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT32)	return VK_INDEX_TYPE_UINT32;
	}

	class VulkanDeviceBuffer : public DeviceBuffer {
	public:
		VulkanDeviceBuffer(const DeviceBufferSpecification& spec);
		VulkanDeviceBuffer(const DeviceBufferSpecification& spec, void* data, uint64 data_size);
		~VulkanDeviceBuffer();

		void Destroy() override;
		
		DeviceBufferSpecification GetSpecification() const override { return m_Specification; }
		uint64 GetDeviceAddress() override;
		void UploadData(uint64 offset, void* data, uint64 data_size) override;

		VkBuffer Raw() const { return m_Buffer; }
		VmaAllocation RawAllocation() const { return m_Allocation; }
		void* GetAdditionalData() const { return m_Data; }



	private:
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation;

		DeviceBufferSpecification m_Specification;
		void* m_Data; // VBOs, IBOs and other buffers can have different data: vertex count, index count;

	};

}