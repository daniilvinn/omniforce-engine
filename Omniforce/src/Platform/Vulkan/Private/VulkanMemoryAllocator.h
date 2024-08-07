#pragma once

#include "../VulkanCommon.h"

#include <vk_mem_alloc.h>
#include <shared_mutex>

namespace Omni {

	struct VulkanAllocatorStatistics
	{
		uint64 allocated;
		uint64 freed;
		uint64 currently_allocated;
	};

	class VulkanMemoryAllocator {
	public:
		static void Init();
		static void Destroy();
		static VulkanMemoryAllocator* Get() { return s_Instance; }

		void InvalidateAllocation(VmaAllocation allocation, uint64 size = VK_WHOLE_SIZE, uint64 offset = 0);

		VmaAllocation AllocateBuffer(VkBufferCreateInfo* create_info, uint32_t flags, VkBuffer* buffer);
		VmaAllocation AllocateImage(VkImageCreateInfo* create_info, uint32_t flags, VkImage* image);

		void DestroyBuffer(VkBuffer* buffer, VmaAllocation* allocation);
		void DestroyImage(VkImage* image, VmaAllocation* allocation);

		void* MapMemory(VmaAllocation allocation);
		void UnmapMemory(VmaAllocation allocation);

		VulkanAllocatorStatistics GetStats() const { return m_Statistics; }

	private:
		VulkanMemoryAllocator();
		~VulkanMemoryAllocator();

	private:
		static VulkanMemoryAllocator* s_Instance;

		VmaAllocator m_Allocator;

		VulkanAllocatorStatistics m_Statistics;

		std::shared_mutex m_Mutex;
	};

}