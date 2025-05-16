#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <Renderer/AccelerationStructure.h>
#include <Renderer/DeviceBuffer.h>

namespace Omni {

	class VulkanAccelerationStructure : public AccelerationStructure {
	public:
		VulkanAccelerationStructure(const BLASBuildInfo& build_info);
		VulkanAccelerationStructure(const TLASBuildInfo& build_info);
		~VulkanAccelerationStructure();

		VkAccelerationStructureKHR Raw() const { return m_AccelerationStructure; }

	private:
		VkAccelerationStructureKHR m_AccelerationStructure = VK_NULL_HANDLE;
		Ref<DeviceBuffer> m_Buffer = nullptr;

	};

}