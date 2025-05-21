#pragma once

#include <Foundation/Common.h>
#include <Renderer/ShaderBindingTable.h>

#include <Renderer/Renderer.h>
#include <Core/Utils.h>

namespace Omni {

	ShaderBindingTable::ShaderBindingTable(const Array<byte>& group_handles, const Array<RTShaderGroup>& groups)
	{
		RendererCapabilities renderer_caps = Renderer::GetCapabilities();
		
		uint32 handle_size = renderer_caps.ray_tracing.shader_handle_size;
		uint32 handle_alignment = renderer_caps.ray_tracing.shader_handle_alignment;
		uint32 aligned_handle_size = Utils::Align(handle_size, handle_alignment);

		// Sort groups
		Array<byte> raygen_handle(&g_TransientAllocator);
		Array<byte> miss_handles(&g_TransientAllocator);
		Array<byte> hit_handles(&g_TransientAllocator);

		for (uint32 i = 0; i < groups.Size(); i++) {
			const RTShaderGroup& group = groups[i];

			bool group_type_detected = false;

			if (group.ray_generation) {
				OMNIFORCE_ASSERT_TAGGED(!group_type_detected, "Detected mixed shader groups, please split them");
				OMNIFORCE_ASSERT_TAGGED(raygen_handle.Size() == 0, "There may be only one raygen group");

				raygen_handle.Resize(aligned_handle_size);
				memcpy(raygen_handle.Raw(), group_handles.Raw() + handle_size * i, handle_size);

				group_type_detected = true;
			}

			if (group.miss) {
				OMNIFORCE_ASSERT_TAGGED(!group_type_detected, "Detected mixed shader groups, please split them");

				miss_handles.Resize(miss_handles.Size() + aligned_handle_size);
				memcpy(miss_handles.Raw() + miss_handles.Size() - aligned_handle_size, group_handles.Raw() + handle_size * i, handle_size);

				group_type_detected = true;
			}

			if (group.closest_hit || group.any_hit || group.intersection) {
				OMNIFORCE_ASSERT_TAGGED(!group_type_detected, "Detected mixed shader groups, please split them");

				hit_handles.Resize(hit_handles.Size() + aligned_handle_size);
				memcpy(hit_handles.Raw() + hit_handles.Size() - aligned_handle_size, group_handles.Raw() + handle_size * i, handle_size);

				group_type_detected = true;
			}
		}

		// Create table buffers
		// Raygen
		DeviceBufferSpecification raygen_table_spec = {};
		raygen_table_spec.size = handle_size + 64;
		raygen_table_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		raygen_table_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		raygen_table_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		raygen_table_spec.flags = (BitMask)DeviceBufferFlags::SBT;

		m_RayGenTable.buffer = DeviceBuffer::Create(&g_PersistentAllocator, raygen_table_spec);
		m_RayGenTable.size = handle_size;
		m_RayGenTable.stride = aligned_handle_size;


		// Miss
		DeviceBufferSpecification miss_table_spec = {};
		miss_table_spec.size = miss_handles.Size() + 64;
		miss_table_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		miss_table_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		miss_table_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		miss_table_spec.flags = (BitMask)DeviceBufferFlags::SBT;

		m_MissTable.buffer = DeviceBuffer::Create(&g_PersistentAllocator, miss_table_spec);
		m_MissTable.size = miss_handles.Size();
		m_MissTable.stride = aligned_handle_size;

		// Hit
		DeviceBufferSpecification hit_table_spec = {};
		hit_table_spec.size = hit_handles.Size() + 64;
		hit_table_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		hit_table_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		hit_table_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		hit_table_spec.flags = (BitMask)DeviceBufferFlags::SBT;

		m_HitTable.buffer = DeviceBuffer::Create(&g_PersistentAllocator, hit_table_spec);
		m_HitTable.size = hit_handles.Size();
		m_HitTable.stride = aligned_handle_size;

		// Apply base alignment and upload data, also compute proper device address offsets
		uint32 base_alignment = renderer_caps.ray_tracing.shader_base_alignment;

		// Raygen
		uint64 raygen_address = m_RayGenTable.buffer->GetDeviceAddress();
		uint32 raygen_table_base_offset = Utils::Align(raygen_address, base_alignment) - raygen_address;
		m_RayGenTable.buffer->UploadData(raygen_table_base_offset, raygen_handle.Raw(), handle_size);
		m_RayGenTable.bda_offset = raygen_table_base_offset;

		// Miss
		uint64 miss_address = m_MissTable.buffer->GetDeviceAddress();
		uint32 miss_table_base_offset = Utils::Align(miss_address, base_alignment) - miss_address;
		m_MissTable.buffer->UploadData(miss_table_base_offset, miss_handles.Raw(), miss_handles.Size());
		m_MissTable.bda_offset = miss_table_base_offset;

		// Hit
		uint64 hit_address = m_HitTable.buffer->GetDeviceAddress();
		uint32 hit_table_base_offset = Utils::Align(hit_address, base_alignment) - hit_address;
		m_HitTable.buffer->UploadData(hit_table_base_offset, hit_handles.Raw(), hit_handles.Size());
		m_HitTable.bda_offset = hit_table_base_offset;
	}

}