#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

namespace Omni {
	class VulkanDebugUtils {
	public:
		VulkanDebugUtils() {}
		VulkanDebugUtils(VkInstance instance);
		~VulkanDebugUtils();

		static VkDebugUtilsMessengerCreateInfoEXT GetMessengerCreateInfo();

	private:
		VkDebugUtilsMessengerEXT m_Logger;

	};
}