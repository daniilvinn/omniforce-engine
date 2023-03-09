#include "../VulkanGraphicsContext.h"
#include "../VulkanDevice.h"
#include "../VulkanDebugUtils.h"

#include <GLFW/glfw3.h>
#include <vector>

namespace Cursed {

	VulkanGraphicsContext::VulkanGraphicsContext()
	{
		VkApplicationInfo application_info = {};
		application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		application_info.apiVersion = VK_API_VERSION_1_3;
		application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		application_info.pEngineName = "Cursed Engine";
		application_info.pApplicationName = "Cursed Engine Core";

		std::vector<const char*> extensions = GetVulkanExtensions();
		std::vector<const char*> layers = GetVulkanLayers();


		VkInstanceCreateInfo instance_create_info = {};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		if (CURSED_BUILD_CONFIG == CURSED_DEBUG_CONFIG) {
			VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = VulkanDebugUtils::GetMessengerCreateInfo();
			instance_create_info.pNext = &messenger_create_info;
		}

		instance_create_info.pApplicationInfo = &application_info;
		instance_create_info.ppEnabledExtensionNames = extensions.data();
		instance_create_info.enabledExtensionCount = extensions.size();
		instance_create_info.ppEnabledLayerNames = layers.data();
		instance_create_info.enabledLayerCount = layers.size();

		VK_CHECK_RESULT(vkCreateInstance(&instance_create_info, nullptr, &m_VulkanInstance));

		if(CURSED_BUILD_CONFIG == CURSED_DEBUG_CONFIG)
			m_DebugUtils = std::make_shared<VulkanDebugUtils>(this);

		VkPhysicalDeviceFeatures device_features = {};
		device_features.samplerAnisotropy = true;
		device_features.geometryShader = true;
		device_features.tessellationShader = true;

		Shared<VulkanPhysicalDevice> device = VulkanPhysicalDevice::Select(this);
		m_Device = std::make_shared<VulkanDevice>(device, std::forward<VkPhysicalDeviceFeatures>(device_features));

	}

	VulkanGraphicsContext::~VulkanGraphicsContext()
	{
		
	}

	void VulkanGraphicsContext::Destroy()
	{

	}

	std::vector<const char*> VulkanGraphicsContext::GetVulkanExtensions()
{
		uint32 glfw_extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

		if (CURSED_BUILD_CONFIG == CURSED_DEBUG_CONFIG) 
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		return extensions;
	}

	std::vector<const char*> VulkanGraphicsContext::GetVulkanLayers()
	{
		std::vector<const char*> layers;
		if (CURSED_BUILD_CONFIG == CURSED_DEBUG_CONFIG)
		{
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}

		return layers;
	}

}