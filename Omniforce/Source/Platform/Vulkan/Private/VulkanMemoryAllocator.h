#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>
#include <Platform/Vulkan/VulkanDeviceBuffer.h>

#include <shared_mutex>

#include <vk_mem_alloc.h>

namespace Omni {

	struct VulkanAllocatorStatistics
	{
		VulkanAllocatorStatistics()
			: allocated(0)
			, freed(0)
			, currently_allocated(0)
			, num_active_images(0)
			, num_active_buffers(0)
		{}

		uint64 allocated;
		uint64 freed;
		uint64 currently_allocated;
		uint64 num_active_images;
		uint64 num_active_buffers;
	};

	class VulkanMemoryAllocator {
	public:
		static void Init();
		static VulkanMemoryAllocator* Get() { return s_Instance; }

		void InvalidateAllocation(VmaAllocation allocation, uint64 size = VK_WHOLE_SIZE, uint64 offset = 0);

		VmaAllocation AllocateBuffer(const DeviceBufferSpecification& spec, VkBufferCreateInfo* create_info, uint32_t flags, VkBuffer* buffer);
		VmaAllocation AllocateImage(VkImageCreateInfo* create_info, uint32_t flags, VkImage* image);

		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);
		void DestroyImage(VkImage image, VmaAllocation allocation);

		void* MapMemory(VmaAllocation allocation);
		void UnmapMemory(VmaAllocation allocation);

		VulkanAllocatorStatistics GetStats() const { return m_Statistics; }
		void SetAllocationName(VmaAllocation allocation, const std::string& name);

	private:
		VulkanMemoryAllocator();

	private:
		static VulkanMemoryAllocator* s_Instance;

		VmaAllocator m_Allocator;

		VulkanAllocatorStatistics m_Statistics;

		std::shared_mutex m_Mutex;
	};

}