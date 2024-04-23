#define VOLK_IMPLEMENTATION
#include <volk.h>

#include "../VulkanGraphicsContext.h"
#include "../VulkanDevice.h"
#include "../VulkanDebugUtils.h"
#include "../VulkanSwapchain.h"
#include "VulkanMemoryAllocator.h"

#include <vector>

#include <GLFW/glfw3.h>

namespace Omni {

	VulkanGraphicsContext* VulkanGraphicsContext::s_Instance;

	VulkanGraphicsContext::VulkanGraphicsContext(const RendererConfig& config)
	{
		if (s_Instance) {
			OMNIFORCE_CORE_ERROR("Graphics context is already initialized.");
			return;
		}

		s_Instance = this;

		volkInitialize();

		VkApplicationInfo application_info = {};
		application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		application_info.apiVersion = VK_API_VERSION_1_3;
		application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		application_info.pEngineName = "Omniforce Engine";
		application_info.pApplicationName = "Omniforce Engine";

		std::vector<const char*> extensions = GetVulkanExtensions();
		std::vector<const char*> layers = GetVulkanLayers();

		VkInstanceCreateInfo instance_create_info = {};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		if (OMNIFORCE_BUILD_CONFIG != OMNIFORCE_RELEASE_CONFIG) {
			VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = VulkanDebugUtils::GetMessengerCreateInfo();
			instance_create_info.pNext = &messenger_create_info;
		}

		instance_create_info.pApplicationInfo = &application_info;
		instance_create_info.ppEnabledExtensionNames = extensions.data();
		instance_create_info.enabledExtensionCount = extensions.size();
		instance_create_info.ppEnabledLayerNames = layers.data();
		instance_create_info.enabledLayerCount = layers.size();
#ifdef OMNIFORCE_DEBUG

#endif

		VK_CHECK_RESULT(vkCreateInstance(&instance_create_info, nullptr, &m_VulkanInstance));
		volkLoadInstance(m_VulkanInstance);

		if(OMNIFORCE_BUILD_CONFIG != OMNIFORCE_RELEASE_CONFIG)
			m_DebugUtils = std::make_shared<VulkanDebugUtils>(this);

		VkPhysicalDeviceFeatures device_features = {};
		device_features.samplerAnisotropy = true;
		device_features.geometryShader = true;
		device_features.tessellationShader = true;
		device_features.shaderInt64 = true;
		device_features.wideLines = true;
		device_features.shaderInt16 = true;

		Shared<VulkanPhysicalDevice> device = VulkanPhysicalDevice::Select(this);
		m_Device = std::make_shared<VulkanDevice>(device, std::forward<VkPhysicalDeviceFeatures>(device_features));
		volkLoadDevice(m_Device->Raw());

		GLFWwindow* window_handle = (GLFWwindow*)config.main_window->Raw();
		ivec2 window_size = {};
		glfwGetFramebufferSize(window_handle, &window_size.x, &window_size.y);

		SwapchainSpecification swapchain_spec = {};
		swapchain_spec.main_window = config.main_window;
		swapchain_spec.frames_in_flight = config.frames_in_flight;
		swapchain_spec.extent = window_size;
		swapchain_spec.vsync = config.vsync;

		m_Swapchain = std::make_shared<VulkanSwapchain>(swapchain_spec);
		m_Swapchain->CreateSurface(swapchain_spec);
		m_Swapchain->CreateSwapchain(swapchain_spec);

		VulkanMemoryAllocator::Init();
	}

	VulkanGraphicsContext::~VulkanGraphicsContext()
	{
		Destroy();
	}

	void VulkanGraphicsContext::Destroy()
	{
		vkDeviceWaitIdle(m_Device->Raw());
		VulkanMemoryAllocator::Destroy();
		m_Swapchain->DestroySwapchain();
		m_Swapchain->DestroySurface();
		m_Device->Destroy();
		if (OMNIFORCE_BUILD_CONFIG == OMNIFORCE_RELEASE_CONFIG)
			m_DebugUtils->Destroy(this);
		vkDestroyInstance(m_VulkanInstance, nullptr);
		volkFinalize();
	}

	std::vector<const char*> VulkanGraphicsContext::GetVulkanExtensions()
	{
		uint32 glfw_extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

		if (OMNIFORCE_BUILD_CONFIG != OMNIFORCE_RELEASE_CONFIG) 
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		return extensions;
	}

	std::vector<const char*> VulkanGraphicsContext::GetVulkanLayers()
	{
		uint32 property_count = 0;
		vkEnumerateInstanceLayerProperties(&property_count, nullptr);

		std::vector<VkLayerProperties> layer_properties(property_count);
		vkEnumerateInstanceLayerProperties(&property_count, layer_properties.data());

		std::vector<const char*> layers;
		if (OMNIFORCE_BUILD_CONFIG != OMNIFORCE_RELEASE_CONFIG)
		{
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}

		return layers;
	}

}