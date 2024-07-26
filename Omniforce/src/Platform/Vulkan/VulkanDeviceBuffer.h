#pragma once

#include "VulkanCommon.h"

#include <Renderer/DeviceBuffer.h>
#include <Renderer/Renderer.h>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.h>

namespace Omni {

	struct IndexBufferData {
		uint32 index_count;
		VkIndexType index_type;
	};

	struct VertexBufferData {
		uint32 vertex_count;
	};

	struct MeshletRenderBufferData {
		uint32 meshlet_count;
	};

	constexpr VkIndexType ExtractIndexType(BitMask buffer_flags) {
		if (buffer_flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT8)		return VK_INDEX_TYPE_UINT8_EXT;
		if (buffer_flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT16)	return VK_INDEX_TYPE_UINT16;
		if (buffer_flags & (uint64)DeviceBufferFlags::INDEX_TYPE_UINT32)	return VK_INDEX_TYPE_UINT32;

		return VK_INDEX_TYPE_MAX_ENUM;
	}

	class VulkanDeviceBuffer : public DeviceBuffer {
	public:
		VulkanDeviceBuffer(const DeviceBufferSpecification& spec);
		VulkanDeviceBuffer(const DeviceBufferSpecification& spec, void* data, uint64 data_size);
		~VulkanDeviceBuffer();

		void Destroy() override;

		VkBuffer Raw() const { return m_Buffer; }
		
		DeviceBufferSpecification GetSpecification() const override { return m_Specification; }
		uint64 GetDeviceAddress() override;
		uint64 GetPerFrameSize() override { return m_Specification.size / Renderer::GetConfig().frames_in_flight; }
		uint64 GetFrameOffset() override { return Renderer::GetCurrentFrameIndex() * GetPerFrameSize(); }
		VmaAllocation RawAllocation() const { return m_Allocation; }
		void* GetAdditionalData() const { return m_Data; }

		void UploadData(uint64 offset, void* data, uint64 data_size) override;
		void CopyRegionTo(Shared<DeviceCmdBuffer> cmd_buffer, Shared<DeviceBuffer> dst_buffer, uint64 src_offset, uint64 dst_offset, uint64 size) override;
		void Clear(Shared<DeviceCmdBuffer> cmd_buffer, uint64 offset, uint64 size, uint32 value) override;

	private:
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation;

		DeviceBufferSpecification m_Specification;
		void* m_Data; // VBOs, IBOs and other buffers can have different data: vertex count, index count;

	};

}