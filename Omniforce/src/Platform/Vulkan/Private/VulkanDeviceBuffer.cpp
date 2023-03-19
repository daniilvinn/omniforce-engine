#include "../VulkanDeviceBuffer.h"

#include "VulkanMemoryAllocator.h"
#include <Platform/Vulkan/VulkanGraphicsContext.h>

namespace Omni {

	VkBufferUsageFlags convert(DeviceBufferUsage usage) 
	{
		switch (usage)
		{
		case DeviceBufferUsage::VERTEX_BUFFER:
			return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case DeviceBufferUsage::INDEX_BUFFER:
			return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case DeviceBufferUsage::UNIFORM_BUFFER:
			return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		case DeviceBufferUsage::STORAGE_BUFFER:
			return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		default:
			std::unreachable();
			break;
		}
	}

	VmaAllocationCreateFlags convert(DeviceBufferMemoryUsage usage) {
		switch (usage)
		{
		case DeviceBufferMemoryUsage::FREQUENT_ACCESS:
			return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
			break;
		case DeviceBufferMemoryUsage::ONE_TIME_HOST_ACCESS:
			return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			break;
		case DeviceBufferMemoryUsage::NO_HOST_ACCESS:
			return 0;
		default:
			std::unreachable();
			break;
		}
	}

	VulkanDeviceBuffer::VulkanDeviceBuffer(const DeviceBufferSpecification& spec)
		: m_Buffer(VK_NULL_HANDLE), m_Specification(spec)
	{
		VulkanMemoryAllocator* alloc = VulkanMemoryAllocator::Get();
		uint64 vma_flags = convert(spec.memory_usage);

		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = spec.size;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_create_info.usage = convert(spec.buffer_usage);
		
		m_Allocation = alloc->AllocateBuffer(&buffer_create_info, vma_flags, &m_Buffer);
	}

	VulkanDeviceBuffer::VulkanDeviceBuffer(const DeviceBufferSpecification& spec, void* data, uint64 data_size)
		: m_Buffer(VK_NULL_HANDLE), m_Specification(spec)
	{
		VulkanMemoryAllocator* alloc = VulkanMemoryAllocator::Get();
		uint64 vma_flags = convert(spec.memory_usage);

		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = spec.size;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_create_info.usage = convert(spec.buffer_usage);

		if (m_Specification.memory_usage == DeviceBufferMemoryUsage::NO_HOST_ACCESS) {
			buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}
		else if (m_Specification.flags & (BitMask)DeviceBufferFlags::CREATE_STAGING_BUFFER) {
			buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}

		m_Allocation = alloc->AllocateBuffer(&buffer_create_info, vma_flags, &m_Buffer);
		this->UploadData(0, data, data_size);
	}

	VulkanDeviceBuffer::~VulkanDeviceBuffer()
	{
		this->Destroy();
	}

	void VulkanDeviceBuffer::Destroy()
	{
		VulkanMemoryAllocator* alloc = VulkanMemoryAllocator::Get();
		alloc->DestroyBuffer(&m_Buffer, &m_Allocation);
	}

	void VulkanDeviceBuffer::UploadData(uint64 offset, void* data, uint64 data_size)
	{
		if (m_Specification.memory_usage == DeviceBufferMemoryUsage::NO_HOST_ACCESS) 
		{
			Shared<VulkanDevice> device = VulkanGraphicsContext::Get()->GetDevice();

			DeviceBufferSpecification staging_buffer_spec = {};
			staging_buffer_spec.size = data_size;
			staging_buffer_spec.buffer_usage = m_Specification.buffer_usage;
			staging_buffer_spec.memory_usage = DeviceBufferMemoryUsage::ONE_TIME_HOST_ACCESS;
			staging_buffer_spec.flags = (uint64)DeviceBufferFlags::CREATE_STAGING_BUFFER;

			VulkanDeviceBuffer staging_buffer(staging_buffer_spec, data, data_size);

			VkBufferCopy buffer_copy = {};
			buffer_copy.size = data_size;
			buffer_copy.srcOffset = 0;
			buffer_copy.dstOffset = 0;

			VkCommandBuffer cmd_buffer = device->AllocateTransientCmdBuffer();

			vkCmdCopyBuffer(cmd_buffer, staging_buffer.Raw(), m_Buffer, 1, &buffer_copy);

			VkBufferMemoryBarrier buffer_memory_barrier = {};
			buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			buffer_memory_barrier.buffer = m_Buffer;
			buffer_memory_barrier.offset = 0;
			buffer_memory_barrier.size = data_size;
			buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(
				cmd_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0,
				0,
				nullptr,
				1,
				&buffer_memory_barrier,
				0,
				nullptr
			);

			device->ExecuteTransientCmdBuffer(cmd_buffer);
		}
		else 
		{
			VulkanMemoryAllocator* allocator = VulkanMemoryAllocator::Get();
			void* memory = allocator->MapMemory(m_Allocation);
			memcpy((byte*)memory + offset, data, data_size);
			allocator->UnmapMemory(m_Allocation);
		}
	}

}