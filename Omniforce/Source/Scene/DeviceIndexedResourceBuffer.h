#pragma once 

#include <Foundation/Common.h>

#include <Renderer/DeviceBuffer.h>
#include <Renderer/DeviceCmdBuffer.h>

namespace Omni {

	template<typename T>
	class DeviceIndexedResourceBuffer {
	public:
		DeviceIndexedResourceBuffer(uint32 buffer_size)
			: m_IndexAllocator(VirtualMemoryBlock::Create(buffer_size))
		{
			DeviceBufferSpecification buffer_spec = {};
			buffer_spec.size = buffer_size;
			buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
			buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
			buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

			m_DeviceBuffer = DeviceBuffer::Create(buffer_spec);
			
			buffer_spec.size = sizeof(T);
			buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
			buffer_spec.buffer_usage = DeviceBufferUsage::STAGING_BUFFER;

			m_StagingForCopy = DeviceBuffer::Create(buffer_spec);
		}

		void Destroy() {
			m_DeviceBuffer->Destroy();
			m_StagingForCopy->Destroy();
		}

		uint32 Allocate(const UUID& id, const T& data) {
			uint32 offset = m_IndexAllocator->Allocate(sizeof(T), alignof(T));
			uint32 index = offset / sizeof (T);

			OMNIFORCE_ASSERT_TAGGED(offset != UINT32_MAX, "Failed to find free block. It can be caused by fragmentation or exceeded resource limits");

			m_StagingForCopy->UploadData(0, (void*)&data, sizeof (T));

			Shared<DeviceCmdBuffer> cmd_buffer = DeviceCmdBuffer::Create(DeviceCmdBufferLevel::PRIMARY, DeviceCmdBufferType::TRANSIENT, DeviceCmdType::GENERAL);
			cmd_buffer->Begin();
			m_StagingForCopy->CopyRegionTo(cmd_buffer, m_DeviceBuffer, 0, offset, sizeof (T));
			cmd_buffer->End();
			cmd_buffer->Execute(true);

			m_Indices.emplace(id, index);
			cmd_buffer->Destroy();

			return offset;
		}

		uint32 ReleaseIndex(const AssetHandle& id) {
			m_IndexAllocator->Free(m_Indices.at(id));
			m_Indices.erase(id);
		}

		uint32 GetIndex(const AssetHandle& id) const { return m_Indices.at(id); }

		uint64 GetStorageBDA() const { return m_DeviceBuffer->GetDeviceAddress(); }
		Shared<DeviceBuffer> GetStorage() const { return m_DeviceBuffer; }

	private:
		Scope<VirtualMemoryBlock> m_IndexAllocator;
		rhumap<UUID, uint32> m_Indices;
		Shared<DeviceBuffer> m_DeviceBuffer;
		Shared<DeviceBuffer> m_StagingForCopy;
	};

}