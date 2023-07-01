#define VMA_IMPLEMENTATION
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.h>

#include <Core/Utils.h>
#include "VulkanMemoryAllocator.h"

#include "../VulkanGraphicsContext.h"

#if OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG
#define OMNIFORCE_TRACE_DEVICE_ALLOCATIONS 0
#else
#define OMNIFORCE_TRACE_DEVICE_ALLOCATIONS 0
#endif

namespace Omni {

	VulkanMemoryAllocator* VulkanMemoryAllocator::s_Instance = nullptr;

	VulkanMemoryAllocator::VulkanMemoryAllocator()
	{
		VulkanGraphicsContext* vk_context = VulkanGraphicsContext::Get();

		VmaVulkanFunctions vulkan_functions = {};
		vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocator_create_info = {};
		allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
		allocator_create_info.instance = vk_context->GetVulkanInstance();
		allocator_create_info.physicalDevice = vk_context->GetDevice()->GetPhysicalDevice()->Raw();
		allocator_create_info.device = vk_context->GetDevice()->Raw();
		allocator_create_info.pVulkanFunctions = &vulkan_functions;

		vmaCreateAllocator(&allocator_create_info, &m_Allocator);

		m_Statistics = { 0, 0, 0 };
	}

	VulkanMemoryAllocator::~VulkanMemoryAllocator()
	{
		vmaDestroyAllocator(m_Allocator);
		OMNIFORCE_CORE_TRACE("Destroying vulkan memory allocator: ");
		OMNIFORCE_CORE_TRACE("\tTotal memory allocated: {0}", Utils::FormatAllocationSize(m_Statistics.allocated));
		OMNIFORCE_CORE_TRACE("\tTotal memory freed: {0}", Utils::FormatAllocationSize(m_Statistics.freed));
		OMNIFORCE_CORE_TRACE("\tIn use at the moment: {0}", Utils::FormatAllocationSize(m_Statistics.currently_allocated));
	}

	void VulkanMemoryAllocator::Init()
	{
		s_Instance = new VulkanMemoryAllocator;
	}

	void VulkanMemoryAllocator::Destroy()
	{
		delete s_Instance;
	}

	VmaAllocation VulkanMemoryAllocator::AllocateBuffer(VkBufferCreateInfo* create_info, uint32_t flags, VkBuffer* buffer)
	{
		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		allocation_create_info.flags = flags;

		VmaAllocation allocation;
		VmaAllocationInfo allocation_info;

		VK_CHECK_RESULT(vmaCreateBuffer(m_Allocator, create_info, &allocation_create_info, buffer, &allocation, &allocation_info));

		m_Statistics.allocated += allocation_info.size;
		m_Statistics.currently_allocated += allocation_info.size;

		if (OMNIFORCE_TRACE_DEVICE_ALLOCATIONS) {
			OMNIFORCE_CORE_TRACE("Allocating device buffer:");
			OMNIFORCE_CORE_TRACE("\tRequested size: {0}", Utils::FormatAllocationSize(create_info->size));
			OMNIFORCE_CORE_TRACE("\tAllocated size: {0}", Utils::FormatAllocationSize(allocation_info.size));
			OMNIFORCE_CORE_TRACE("\tCurrently allocated: {0}", Utils::FormatAllocationSize(m_Statistics.currently_allocated));
		}
		return allocation;
	}

	VmaAllocation VulkanMemoryAllocator::AllocateImage(VkImageCreateInfo* create_info, uint32_t flags, VkImage* image)
	{
		if (create_info->extent.depth == 0)
			OMNIFORCE_CORE_WARNING("Trying to allocate image with 0 depth. No allocation done");

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		allocation_create_info.flags = flags;

		VmaAllocation allocation;
		VmaAllocationInfo allocation_info;

		VK_CHECK_RESULT(vmaCreateImage(m_Allocator, create_info, &allocation_create_info, image, &allocation, &allocation_info));
		m_Statistics.allocated += allocation_info.size;
		m_Statistics.currently_allocated += allocation_info.size;

		if (OMNIFORCE_TRACE_DEVICE_ALLOCATIONS) {
			OMNIFORCE_CORE_TRACE("Allocating device image:");
			OMNIFORCE_CORE_TRACE("\tAllocated size: {0}", Utils::FormatAllocationSize(allocation_info.size));
			OMNIFORCE_CORE_TRACE("\tCurrently allocated: {0}", Utils::FormatAllocationSize(m_Statistics.currently_allocated));
		}
		return allocation;
	}

	void VulkanMemoryAllocator::DestroyBuffer(VkBuffer* buffer, VmaAllocation* allocation)
	{
		VmaAllocationInfo allocation_info;
		vmaGetAllocationInfo(m_Allocator, *allocation, &allocation_info);

		vmaDestroyBuffer(m_Allocator, *buffer, *allocation);

		m_Statistics.freed += allocation_info.size;
		m_Statistics.currently_allocated -= allocation_info.size;

		if (OMNIFORCE_TRACE_DEVICE_ALLOCATIONS) {
			OMNIFORCE_CORE_TRACE("Destroying device buffer:");
			OMNIFORCE_CORE_TRACE("\tFreed memory: {0}", Utils::FormatAllocationSize(allocation_info.size));
			OMNIFORCE_CORE_TRACE("\tCurrently allocated: {0}", Utils::FormatAllocationSize(m_Statistics.currently_allocated));
		}

		*buffer = VK_NULL_HANDLE;
		*allocation = VK_NULL_HANDLE;
	}

	void VulkanMemoryAllocator::DestroyImage(VkImage* image, VmaAllocation* allocation)
	{
		VmaAllocationInfo allocation_info;
		vmaGetAllocationInfo(m_Allocator, *allocation, &allocation_info);

		vmaDestroyImage(m_Allocator, *image, *allocation);

		m_Statistics.freed += allocation_info.size;
		m_Statistics.currently_allocated -= allocation_info.size;

		if (OMNIFORCE_TRACE_DEVICE_ALLOCATIONS) {
			OMNIFORCE_CORE_TRACE("Destroying device image:");
			OMNIFORCE_CORE_TRACE("\tFreed memory: {0}", Utils::FormatAllocationSize(allocation_info.size));
			OMNIFORCE_CORE_TRACE("\tCurrently allocated: {0}", Utils::FormatAllocationSize(m_Statistics.currently_allocated));
		}

		*image = VK_NULL_HANDLE;
		*allocation = VK_NULL_HANDLE;
	}


	void* VulkanMemoryAllocator::MapMemory(VmaAllocation allocation)
	{
		void* mapped_memory;
		VK_CHECK_RESULT(vmaMapMemory(m_Allocator, allocation, &mapped_memory));
		return mapped_memory;
	}

	void VulkanMemoryAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(m_Allocator, allocation);
	}

}