#pragma once

#include "../VulkanCommon.h"

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.h>

namespace Omni {

	class VulkanMemoryAllocator {
	public:
		static void Init();
		static void Destroy();
		static VulkanMemoryAllocator* Get() { return s_Instance; }

		VmaAllocation AllocateBuffer(VkBufferCreateInfo* create_info, uint32_t flags, VkBuffer* buffer);
		VmaAllocation AllocateImage(VkImageCreateInfo* create_info, uint32_t flags, VkImage* image);

		void DestroyImage(VkImage* image, VmaAllocation allocation);
		void DestroyBuffer(VkBuffer* buffer, VmaAllocation* allocation);

		void* MapMemory(VmaAllocation allocation);
		void UnmapMemory(VmaAllocation allocation);

	private:
		VulkanMemoryAllocator();
		~VulkanMemoryAllocator();

	private:
		static VulkanMemoryAllocator* s_Instance;

		VmaAllocator m_Allocator;

		struct AllocatorStatistics 
		{
			uint64 allocated;
			uint64 freed;
			uint64 currently_allocated;
		} m_Statistics;

	};

}