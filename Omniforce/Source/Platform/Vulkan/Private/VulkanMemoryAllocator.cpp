#include <Foundation/Common.h>
#include <Platform/Vulkan/Private/VulkanMemoryAllocator.h>

#include <Core/Utils.h>
#include <Platform/Vulkan/VulkanGraphicsContext.h>
#include <Platform/Vulkan/VulkanDeviceBuffer.h>
#include <Core/RuntimeExecutionContext.h>

#if OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG
	#define OMNIFORCE_TRACE_DEVICE_ALLOCATIONS 1
#else
	#define OMNIFORCE_TRACE_DEVICE_ALLOCATIONS 0
#endif

#undef VMA_LEAK_LOG_FORMAT(format, ...)
#define VMA_LEAK_LOG_FORMAT(format, ...) do { \
       printf((format), __VA_ARGS__); \
       printf("\n"); \
   } while(false)

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Omni {

	VulkanMemoryAllocator* VulkanMemoryAllocator::s_Instance = nullptr;

	VulkanMemoryAllocator::VulkanMemoryAllocator()
	{
		VulkanGraphicsContext* vk_context = VulkanGraphicsContext::Get();

		VmaVulkanFunctions vulkan_functions = {};
		vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocator_create_info = {};
		allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
		allocator_create_info.instance = vk_context->GetVulkanInstance();
		allocator_create_info.physicalDevice = vk_context->GetDevice()->GetPhysicalDevice()->Raw();
		allocator_create_info.device = vk_context->GetDevice()->Raw();
		allocator_create_info.pVulkanFunctions = &vulkan_functions;
		allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		vmaCreateAllocator(&allocator_create_info, &m_Allocator);

		m_Statistics = { 0, 0, 0 };

		RuntimeExecutionContext::Get().GetObjectLifetimeManager().EnqueueCoreObjectDelection(
			[allocator = m_Allocator, stats = m_Statistics, instance = s_Instance]() {
				OMNIFORCE_CORE_TRACE("Destroying vulkan memory allocator: ");
				OMNIFORCE_CORE_TRACE("\tTotal memory allocated: {0}", Utils::FormatAllocationSize(stats.allocated));
				OMNIFORCE_CORE_TRACE("\tTotal memory freed: {0}", Utils::FormatAllocationSize(stats.freed));
				OMNIFORCE_CORE_TRACE("\tIn use at the moment: {0}", Utils::FormatAllocationSize(stats.currently_allocated));
				vmaDestroyAllocator(allocator);

				delete s_Instance;
			}
		);
	}

	void VulkanMemoryAllocator::Init()
	{
		s_Instance = new VulkanMemoryAllocator;
	}

	void VulkanMemoryAllocator::InvalidateAllocation(VmaAllocation allocation, uint64 size /*= VK_WHOLE_SIZE*/, uint64 offset /*= 0*/)
	{
		std::lock_guard lock(m_Mutex);
		vmaInvalidateAllocation(m_Allocator, allocation, offset, size);
	}

	VmaAllocation VulkanMemoryAllocator::AllocateBuffer(const DeviceBufferSpecification& spec, VkBufferCreateInfo* create_info, uint32_t flags, VkBuffer* buffer)
	{
		std::lock_guard lock(m_Mutex);
		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.usage = spec.heap == DeviceBufferMemoryHeap::DEVICE ? VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE : VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
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
		std::lock_guard lock(m_Mutex);
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

	void VulkanMemoryAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		std::lock_guard lock(m_Mutex);
		VmaAllocationInfo allocation_info;
		vmaGetAllocationInfo(m_Allocator, allocation, &allocation_info);

		vmaDestroyBuffer(m_Allocator, buffer, allocation);

		m_Statistics.freed += allocation_info.size;
		m_Statistics.currently_allocated -= allocation_info.size;

		if (OMNIFORCE_TRACE_DEVICE_ALLOCATIONS) {
			OMNIFORCE_CORE_TRACE("Destroying device buffer:");
			OMNIFORCE_CORE_TRACE("\tFreed memory: {0}", Utils::FormatAllocationSize(allocation_info.size));
			OMNIFORCE_CORE_TRACE("\tCurrently allocated: {0}", Utils::FormatAllocationSize(m_Statistics.currently_allocated));
		}
	}

	void VulkanMemoryAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		std::lock_guard lock(m_Mutex);
		VmaAllocationInfo allocation_info;
		vmaGetAllocationInfo(m_Allocator, allocation, &allocation_info);

		vmaDestroyImage(m_Allocator, image, allocation);

		m_Statistics.freed += allocation_info.size;
		m_Statistics.currently_allocated -= allocation_info.size;

		if (OMNIFORCE_TRACE_DEVICE_ALLOCATIONS) {
			OMNIFORCE_CORE_TRACE("Destroying device image:");
			OMNIFORCE_CORE_TRACE("\tFreed memory: {0}", Utils::FormatAllocationSize(allocation_info.size));
			OMNIFORCE_CORE_TRACE("\tCurrently allocated: {0}", Utils::FormatAllocationSize(m_Statistics.currently_allocated));
		}
	}


	void* VulkanMemoryAllocator::MapMemory(VmaAllocation allocation)
	{
		std::lock_guard lock(m_Mutex);
		void* mapped_memory;
		VK_CHECK_RESULT(vmaMapMemory(m_Allocator, allocation, &mapped_memory));
		return mapped_memory;
	}

	void VulkanMemoryAllocator::UnmapMemory(VmaAllocation allocation)
	{
		std::lock_guard lock(m_Mutex);
		vmaUnmapMemory(m_Allocator, allocation);
	}

}