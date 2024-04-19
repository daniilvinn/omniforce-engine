#include "../DeviceMaterialPool.h"

#include <Asset/AssetManager.h>
#include <Renderer/DeviceCmdBuffer.h>
#include "Scene/SceneRenderer.h"

namespace Omni {

	DeviceMaterialPool::DeviceMaterialPool(SceneRenderer* context, uint64 size)
		: m_Context(context)
	{
		m_VirtualAllocator = VirtualMemoryBlock::Create(size);

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.size = size;
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

		m_PoolBuffer = DeviceBuffer::Create(buffer_spec);

		buffer_spec.size = 512;
		buffer_spec.buffer_usage = DeviceBufferUsage::STAGING_BUFFER;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
		buffer_spec.heap = DeviceBufferMemoryHeap::HOST;

		m_StagingForCopy = DeviceBuffer::Create(buffer_spec);
	}

	DeviceMaterialPool::~DeviceMaterialPool()
	{
		m_PoolBuffer->Destroy();
		m_StagingForCopy->Destroy();
		m_VirtualAllocator->Destroy();
	}

	uint64 DeviceMaterialPool::Allocate(AssetHandle material)
	{
		AssetManager* asset_manager = AssetManager::Get();
		Shared<Material> m = asset_manager->GetAsset<Material>(material);

		auto material_table = m->GetTable();

		uint16 material_size = 0;

		for (auto& entry : material_table)
			material_size += Material::GetRuntimeEntrySize(entry.second.index());

		byte* material_data = new byte[material_size];
		uint32 current_offset = 0;

		for (auto& material_entry : material_table) {
			if (material_entry.second.index() == 0) {
				AssetHandle texture_handle = std::get<AssetHandle>(material_entry.second);
				material_entry.second = m_Context->AcquireResourceIndex(asset_manager->GetAsset<Image>(texture_handle), SamplerFilteringMode::LINEAR);
			}
		}

		for (auto& entry : material_table) {			
			uint8 entry_size = Material::GetRuntimeEntrySize(entry.second.index());
			std::visit([&](const auto& value) {
				memcpy(material_data + current_offset, &value, entry_size);
			}, entry.second);
			
			current_offset += entry_size;
		}

		m_StagingForCopy->UploadData(0, material_data, material_size);

		uint32 dst_offset = m_VirtualAllocator->Allocate(material_size, 16);
		OMNIFORCE_ASSERT_TAGGED(dst_offset != UINT32_MAX, "Failed to allocate material data. Exceeded limit?");
		m_OffsetsMap.emplace(material, dst_offset);

		Shared<DeviceCmdBuffer> cmd_buffer = DeviceCmdBuffer::Create(DeviceCmdBufferLevel::PRIMARY, DeviceCmdBufferType::TRANSIENT, DeviceCmdType::GENERAL);
		cmd_buffer->Begin();
		m_StagingForCopy->CopyRegionTo(cmd_buffer, m_PoolBuffer, 0, dst_offset, material_size);
		cmd_buffer->End();
		cmd_buffer->Execute(true);
		cmd_buffer->Destroy();

		return dst_offset;
	}

	void DeviceMaterialPool::Free(AssetHandle material)
	{
		m_VirtualAllocator->Free(m_OffsetsMap.at(material));
	}

	uint64 DeviceMaterialPool::GetStorageBufferAddress() const
	{
		return m_PoolBuffer->GetDeviceAddress();
	}

}