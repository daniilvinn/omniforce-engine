#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanAccelerationStructure.h>

#include <Platform/Vulkan/VulkanGraphicsContext.h>
#include <Platform/Vulkan/VulkanDeviceBuffer.h>
#include <Platform/Vulkan/VulkanDeviceCmdBuffer.h>
#include <Renderer/DeviceBuffer.h>
#include <Core/RuntimeExecutionContext.h>

#include <glm/gtc/packing.hpp>

namespace Omni {

	// =======
	// BLAS constructor
	// =======
	VulkanAccelerationStructure::VulkanAccelerationStructure(const BLASBuildInfo& build_info)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VkAccelerationStructureGeometryTrianglesDataKHR geometry_triangle_data = {};
		geometry_triangle_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry_triangle_data.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry_triangle_data.vertexData.deviceAddress = build_info.geometry->GetDeviceAddress();
		geometry_triangle_data.vertexStride = build_info.vertex_stride;
		geometry_triangle_data.maxVertex = build_info.vertex_count - 1;
		geometry_triangle_data.indexType = VK_INDEX_TYPE_UINT32;
		geometry_triangle_data.indexData.deviceAddress = build_info.indices->GetDeviceAddress();
		geometry_triangle_data.transformData.deviceAddress = 0;

		// Fill out info to get build size
		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		geometry.geometry.triangles = geometry_triangle_data;

		VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info = {};
		build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		build_geometry_info.geometryCount = 1;
		build_geometry_info.pGeometries = &geometry;
		build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		uint32 primitive_count = build_info.index_count / 3;

		VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = {};
		build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(
			device->Raw(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&build_geometry_info,
			&primitive_count,
			&build_sizes_info
		);

		// Create as buffer
		DeviceBufferSpecification as_buffer_spec = {};
		as_buffer_spec.size = build_sizes_info.accelerationStructureSize;
		as_buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		as_buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_STORAGE;
		as_buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		as_buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;

		m_Buffer = DeviceBuffer::Create(&g_PersistentAllocator, as_buffer_spec);
		WeakPtr<VulkanDeviceBuffer> vk_as_buffer = m_Buffer;

		// Create acceleration structure
		VkAccelerationStructureCreateInfoKHR as_create_info = {};
		as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		as_create_info.buffer = vk_as_buffer->Raw();
		as_create_info.offset = 0;
		as_create_info.size = build_sizes_info.accelerationStructureSize;
		as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		vkCreateAccelerationStructureKHR(device->Raw(), &as_create_info, nullptr, &m_AccelerationStructure);

		// Create scratch buffer
		DeviceBufferSpecification scratch_buffer_spec = {};
		scratch_buffer_spec.size = build_sizes_info.buildScratchSize;
		scratch_buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		scratch_buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		scratch_buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		scratch_buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_SCRATCH;

		VulkanDeviceBuffer scratch_buffer(scratch_buffer_spec);

		// Build info
		build_geometry_info.dstAccelerationStructure = m_AccelerationStructure;
		build_geometry_info.scratchData.deviceAddress = scratch_buffer.GetDeviceAddress();

		// Build range info
		VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};
		build_range_info.firstVertex = 0;
		build_range_info.primitiveCount = build_info.index_count / 3;
		build_range_info.primitiveOffset = 0;
		build_range_info.transformOffset = 0;

		const VkAccelerationStructureBuildRangeInfoKHR* ranges[] = { &build_range_info };

		// Create cmd buffer, record command and execute
		VulkanDeviceCmdBuffer cmd_buffer(DeviceCmdBufferLevel::PRIMARY, DeviceCmdBufferType::TRANSIENT, DeviceCmdType::GENERAL);

		cmd_buffer.Begin();
		vkCmdBuildAccelerationStructuresKHR(cmd_buffer, 1, &build_geometry_info, ranges);
		cmd_buffer.End();
		cmd_buffer.Execute(true);
	}

	// =======
	// TLAS constructor
	// =======
	VulkanAccelerationStructure::VulkanAccelerationStructure(const TLASBuildInfo& build_info)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		uint32 i = 0;
		Array<VkAccelerationStructureInstanceKHR> instances(&g_TransientAllocator);
		for (const auto& instance : build_info.instances) {
			// Get BlAS address
			WeakPtr<VulkanAccelerationStructure> vk_blas = instance.blas;

			VkAccelerationStructureDeviceAddressInfoKHR blas_address_info = {};
			blas_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			blas_address_info.accelerationStructure = vk_blas->Raw();

			VkDeviceAddress blas_address = vkGetAccelerationStructureDeviceAddressKHR(device->Raw(), &blas_address_info);

			// Create VkTransformMatrixKHR
			// Check if rotation is correct here
			glm::vec4 full_precision_quat = glm::unpackHalf(instance.transform.rotation);
			glm::mat4 rotation = glm::mat4_cast(glm::quat(full_precision_quat.w, full_precision_quat.x, full_precision_quat.y, full_precision_quat.z));
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), instance.transform.translation);
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), instance.transform.scale);

			glm::mat4 full_transform = translation * rotation * scale;
			VkTransformMatrixKHR vk_transform = ToVkTransform(full_transform);

			// Construct instance
			VkAccelerationStructureInstanceKHR vk_instance = {};
			vk_instance.accelerationStructureReference = blas_address;
			vk_instance.instanceCustomIndex = instance.custom_index;
			vk_instance.mask = instance.mask;
			vk_instance.instanceShaderBindingTableRecordOffset = instance.SBT_record_offset;
			vk_instance.flags = 0;
			vk_instance.transform = vk_transform;

			instances.Add(vk_instance);

			i++;
		}

		// Create TLAS instance buffer
		DeviceBufferSpecification instance_buffer_spec = {};
		instance_buffer_spec.size = sizeof(VkAccelerationStructureInstanceKHR) * build_info.instances.Size();
		instance_buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		instance_buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
		instance_buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		instance_buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_INPUT;

		VulkanDeviceBuffer instances_buffer(instance_buffer_spec, instances.Raw(), instances.Size() * sizeof(VkAccelerationStructureInstanceKHR));

		VkAccelerationStructureGeometryInstancesDataKHR geometry_instance_data = {};
		geometry_instance_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		geometry_instance_data.arrayOfPointers = false;
		geometry_instance_data.data.deviceAddress = instances_buffer.GetDeviceAddress();

		// Fill out info to get build size
		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		geometry.geometry.instances = geometry_instance_data;

		VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info = {};
		build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		build_geometry_info.geometryCount = 1;
		build_geometry_info.pGeometries = &geometry;
		build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		uint32 primitive_count = build_info.instances.Size();

		VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = {};
		build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(
			device->Raw(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&build_geometry_info,
			&primitive_count,
			&build_sizes_info
		);

		// Create as buffer
		DeviceBufferSpecification as_buffer_spec = {};
		as_buffer_spec.size = build_sizes_info.accelerationStructureSize;
		as_buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		as_buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_STORAGE;
		as_buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		as_buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;

		m_Buffer = DeviceBuffer::Create(&g_PersistentAllocator, as_buffer_spec);
		WeakPtr<VulkanDeviceBuffer> vk_as_buffer = m_Buffer;

		// Create acceleration structure
		VkAccelerationStructureCreateInfoKHR as_create_info = {};
		as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		as_create_info.buffer = vk_as_buffer->Raw();
		as_create_info.offset = 0;
		as_create_info.size = build_sizes_info.accelerationStructureSize;
		as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

		vkCreateAccelerationStructureKHR(device->Raw(), &as_create_info, nullptr, &m_AccelerationStructure);

		// Create scratch buffer
		DeviceBufferSpecification scratch_buffer_spec = {};
		scratch_buffer_spec.size = build_sizes_info.buildScratchSize;
		scratch_buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		scratch_buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		scratch_buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		scratch_buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_SCRATCH;

		VulkanDeviceBuffer scratch_buffer(scratch_buffer_spec);

		// Build info
		build_geometry_info.dstAccelerationStructure = m_AccelerationStructure;
		build_geometry_info.scratchData.deviceAddress = scratch_buffer.GetDeviceAddress();

		// Build range info
		VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};
		build_range_info.firstVertex = 0;
		build_range_info.primitiveCount = build_info.instances.Size();
		build_range_info.primitiveOffset = 0;
		build_range_info.transformOffset = 0;

		const VkAccelerationStructureBuildRangeInfoKHR* ranges[] = { &build_range_info };

		// Create cmd buffer, record command and execute
		VulkanDeviceCmdBuffer cmd_buffer(DeviceCmdBufferLevel::PRIMARY, DeviceCmdBufferType::TRANSIENT, DeviceCmdType::GENERAL);

		cmd_buffer.Begin();
		vkCmdBuildAccelerationStructuresKHR(cmd_buffer, 1, &build_geometry_info, ranges);
		cmd_buffer.End();
		cmd_buffer.Execute(true);
	}

	VulkanAccelerationStructure::~VulkanAccelerationStructure()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice()->Raw();
		auto as = m_AccelerationStructure;

		RuntimeExecutionContext::Get().GetObjectLifetimeManager().EnqueueObjectDeletion(
			[device, as]() {
				vkDestroyAccelerationStructureKHR(device, as, nullptr);
			}
		);
	}

}