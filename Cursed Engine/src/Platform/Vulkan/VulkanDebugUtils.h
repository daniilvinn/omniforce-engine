#pragma once

#include "VulkanCommon.h"

namespace Cursed {
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