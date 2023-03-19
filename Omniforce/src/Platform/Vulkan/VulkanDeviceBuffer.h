#pragma once

#include "VulkanCommon.h"

#include <Renderer/DeviceBuffer.h>

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.h>

namespace Omni {

	class VulkanDeviceBuffer : public DeviceBuffer {
	public:
		VulkanDeviceBuffer(const DeviceBufferSpecification& spec);
		VulkanDeviceBuffer(const DeviceBufferSpecification& spec, void* data, uint64 data_size);
		~VulkanDeviceBuffer();

		void Destroy() override;
		
		VkBuffer Raw() const { return m_Buffer; }
		VmaAllocation RawAllocation() const { return m_Allocation; }

		void UploadData(uint64 offset, void* data, uint64 data_size);
		DeviceBufferSpecification GetSpecification() const override { return m_Specification; }

	private:
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation;

		DeviceBufferSpecification m_Specification;


	};

}