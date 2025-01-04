#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

namespace Omni {
	class VulkanDebugUtils {
	public:
		VulkanDebugUtils(VulkanGraphicsContext* ctx);
		~VulkanDebugUtils();

		void Destroy(VulkanGraphicsContext* ctx);

		static VkDebugUtilsMessengerCreateInfoEXT GetMessengerCreateInfo();

	private:
		VkDebugUtilsMessengerEXT m_Logger;

	};
}