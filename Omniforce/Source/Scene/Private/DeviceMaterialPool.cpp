#include <Foundation/Common.h>
#include <Scene/DeviceMaterialPool.h>

#include <Asset/AssetManager.h>
#include <Renderer/DeviceCmdBuffer.h>
#include <Scene/ISceneRenderer.h>
#include <CodeGeneration/Device/RTMaterial.h>
#include <Core/EngineConfig.h>

namespace Omni {

	static EngineConfigValue<bool> s_BuildVirtualGeometry("Renderer.UseVirtualGeometry", "Enables virtual geometry raster renderer");

	DeviceMaterialPool::DeviceMaterialPool(ISceneRenderer* context, uint64 size)
		: m_Context(context)
	{
		m_VirtualAllocator = VirtualMemoryBlock::Create(&g_PersistentAllocator, size);

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.size = size;
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

		m_PoolBuffer = DeviceBuffer::Create(&g_PersistentAllocator, buffer_spec);

		buffer_spec.size = 512;
		buffer_spec.buffer_usage = DeviceBufferUsage::STAGING_BUFFER;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
		buffer_spec.heap = DeviceBufferMemoryHeap::HOST;

		m_StagingForCopy = DeviceBuffer::Create(&g_PersistentAllocator, buffer_spec);
	}

	DeviceMaterialPool::~DeviceMaterialPool()
	{
		m_VirtualAllocator->Destroy();
	}

	uint64 DeviceMaterialPool::Allocate(AssetHandle material)
	{
		AssetManager* asset_manager = AssetManager::Get();
		Ref<Material> m = asset_manager->GetAsset<Material>(material);

		auto material_table = m->GetTable();

		// Reserve first 4 bytes for pipeline UUID and RT material
		uint16 material_size = 4 + sizeof(RTMaterial);

		for (auto& entry : material_table)
			material_size += Material::GetRuntimeEntrySize(entry.second.index());

		byte* material_data = new byte[material_size];
		uint32 current_offset = 4 + sizeof(RTMaterial); // also add padding due to RTMaterial

		for (auto& material_entry : material_table) {
			if (material_entry.second.index() == 0) {
				MaterialTextureProperty texture_property = std::get<MaterialTextureProperty>(material_entry.second);
				texture_property.first = m_Context->AcquireResourceIndex(asset_manager->GetAsset<Image>(texture_property.first), SamplerFilteringMode::LINEAR);

				// HACK: encode texture index and uv channel index in AssetHandle.
				// Very cursed approach but it should works.
				// Probably needs to be redesigned to something like "RuntimeMaterialInstance"
				material_entry.second = MaterialTextureProperty((uint64)texture_property.first | (((uint64)texture_property.second) << 32), 0);
			}
		}

		// Create RT material
		RTMaterial rt_material = {};

		// Fill-in flags
		rt_material.Metadata.HasBaseColor				= material_table.contains("BASE_COLOR_MAP");
		rt_material.Metadata.HasNormal					= material_table.contains("NORMAL_MAP");
		rt_material.Metadata.HasMetallicRoughness		= material_table.contains("METALLIC_ROUGHNESS_MAP");
		rt_material.Metadata.HasOcclusion				= material_table.contains("OCCLUSION_MAP");
		rt_material.Metadata.DoubleSided				= std::get<uint32>(material_table.at("DOUBLE_SIDED"));

		// Fill-in data
		if (rt_material.Metadata.HasBaseColor) {
			memcpy(&rt_material.BaseColor, &material_table["BASE_COLOR_MAP"], sizeof(DeviceTexture));
		}
		if (rt_material.Metadata.HasNormal) {
			memcpy(&rt_material.Normal, &material_table["NORMAL_MAP"], sizeof(DeviceTexture));
		}
		if (rt_material.Metadata.HasMetallicRoughness) {
			memcpy(&rt_material.MetallicRoughness, &material_table["METALLIC_ROUGHNESS_MAP"], sizeof(DeviceTexture));
		}
		if (rt_material.Metadata.HasOcclusion) {
			memcpy(&rt_material.Occlusion, &material_table["OCCLUSION_MAP"], sizeof(DeviceTexture));
		}

		// Handle uniform color
		if (material_table.contains("BASE_COLOR_FACTOR")) {
			memcpy(&rt_material.UniformColor, &material_table["BASE_COLOR_FACTOR"], sizeof(glm::vec4));
		}
		else {
			glm::vec4 default_value = glm::vec4(1.0f);
			memcpy(&rt_material.UniformColor, &default_value, sizeof(glm::vec4));
		}

		memcpy(material_data, &rt_material, sizeof(RTMaterial));

		// Copy pipeline device id to first 4 bytes
		uint32 device_pipeline_id = s_BuildVirtualGeometry.Get() ? m->GetPipeline()->GetDeviceID() : 0;
		memcpy(material_data + sizeof(RTMaterial), &device_pipeline_id, sizeof(device_pipeline_id));

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

		Ref<DeviceCmdBuffer> cmd_buffer = DeviceCmdBuffer::Create(&g_TransientAllocator, DeviceCmdBufferLevel::PRIMARY, DeviceCmdBufferType::TRANSIENT, DeviceCmdType::GENERAL);
		cmd_buffer->Begin();
		m_StagingForCopy->CopyRegionTo(cmd_buffer, m_PoolBuffer, 0, dst_offset, material_size);
		cmd_buffer->End();
		cmd_buffer->Execute(true);

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