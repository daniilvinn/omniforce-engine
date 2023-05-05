#include "../VulkanDeviceBuffer.h"

#include "VulkanMemoryAllocator.h"
#include <Platform/Vulkan/VulkanGraphicsContext.h>

namespace Omni {

#pragma region converts
	VkBufferUsageFlags convert(DeviceBufferUsage usage) 
	{
		switch (usage)
		{
		case DeviceBufferUsage::VERTEX_BUFFER:						return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		case DeviceBufferUsage::INDEX_BUFFER:						return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		case DeviceBufferUsage::UNIFORM_BUFFER:						return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		case DeviceBufferUsage::STORAGE_BUFFER:						return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		case DeviceBufferUsage::STAGING_BUFFER:						return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		default:													std::unreachable();
		}
	}

	VmaAllocationCreateFlags convert(DeviceBufferMemoryUsage usage) {
		switch (usage)
		{
		case DeviceBufferMemoryUsage::READ_BACK:				return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;			
		case DeviceBufferMemoryUsage::COHERENT_WRITE:			return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;  
		case DeviceBufferMemoryUsage::NO_HOST_ACCESS:				return 0;														
		default:													std::unreachable();												
		}
	}
#pragma endregion

	VulkanDeviceBuffer::VulkanDeviceBuffer(const DeviceBufferSpecification& spec)
		: m_Buffer(VK_NULL_HANDLE), m_Specification(spec), m_Data(nullptr)
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
		: m_Buffer(VK_NULL_HANDLE), m_Specification(spec), m_Data(nullptr)
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

		if(m_Data) delete m_Data;
	}

	void VulkanDeviceBuffer::UploadData(uint64 offset, void* data, uint64 data_size)
	{
		if (m_Specification.memory_usage == DeviceBufferMemoryUsage::NO_HOST_ACCESS) 
		{
			Shared<VulkanDevice> device = VulkanGraphicsContext::Get()->GetDevice();

			DeviceBufferSpecification staging_buffer_spec = {};
			staging_buffer_spec.size = data_size;
			staging_buffer_spec.buffer_usage = DeviceBufferUsage::STAGING_BUFFER;
			staging_buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;

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
			// TODO: flush memory ranges for non-coherent allocations so changes are visible on GPU
			VulkanMemoryAllocator* allocator = VulkanMemoryAllocator::Get();
			void* memory = allocator->MapMemory(m_Allocation);
			memcpy((byte*)memory + offset, data, data_size);
			allocator->UnmapMemory(m_Allocation);
		}

		// Further data should only be set for non-staging buffers, since staging buffer will not be used in rendering at all
		if (m_Specification.flags & (uint64)DeviceBufferFlags::CREATE_STAGING_BUFFER)
			return;

		if (m_Specification.buffer_usage == DeviceBufferUsage::INDEX_BUFFER) 
		{
			if(!m_Data) m_Data = new IndexBufferData;

			IndexBufferData* ibo_data = (IndexBufferData*)m_Data;
			uint8 index_size = 0;
			if (m_Specification.flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT8)  index_size = 1;
			if (m_Specification.flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT16) index_size = 2;
			if (m_Specification.flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT32) index_size = 4;

			OMNIFORCE_ASSERT_TAGGED(index_size, "Index size was not set. Unable to evaluate index count");

			ibo_data->index_count = data_size / index_size;
			ibo_data->index_type = ExtractIndexType(m_Specification.flags);

		}

		else if (m_Specification.buffer_usage == DeviceBufferUsage::VERTEX_BUFFER) {
			if (!m_Data) m_Data = new VertexBufferData;

			VertexBufferData* vbo_data = (VertexBufferData*)m_Data;
			
			vbo_data->vertex_count = data_size / sizeof(float32);
		}
		
	}

}