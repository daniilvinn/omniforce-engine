#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <RHI/AccelerationStructure.h>
#include <RHI/DeviceBuffer.h>

namespace Omni {

	class VulkanRTAccelerationStructure : public RTAccelerationStructure {
	public:
		VulkanRTAccelerationStructure(const BLASBuildInfo& build_info);
		VulkanRTAccelerationStructure(const TLASBuildInfo& build_info);
		~VulkanRTAccelerationStructure();

		VkAccelerationStructureKHR Raw() const { return m_AccelerationStructure; }

	private:
		VkAccelerationStructureKHR m_AccelerationStructure = VK_NULL_HANDLE;
		Ref<DeviceBuffer> m_Buffer = nullptr;

	};

}