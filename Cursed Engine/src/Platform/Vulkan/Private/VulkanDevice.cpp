#include "../VulkanDevice.h"

#include "../VulkanGraphicsContext.h"

namespace Cursed {

	// ===============
	// PHYSICAL DEVICE
	// ===============

	Shared<VulkanPhysicalDevice> VulkanPhysicalDevice::Select(VulkanGraphicsContext* ctx)
	{
		return std::make_shared<VulkanPhysicalDevice>(ctx);
	}

	// TODO: check for device surface support
	VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanGraphicsContext* ctx)
		: m_PhysicalDevice(VK_NULL_HANDLE), m_Indices()
	{
		VkInstance vk_instance = ctx->GetVulkanInstance();

		uint32 physical_device_count = 0;
		vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr);

		std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
		vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices.data());

		uint32* device_scores = new uint32[physical_device_count];
		memset(device_scores, 0, sizeof(uint32) * physical_device_count); // init memory

		uint32 index = 0;
		for (auto& device : physical_devices) {
			VkPhysicalDeviceProperties device_props = {};
			VkPhysicalDeviceMemoryProperties device_memory_props = {};

			vkGetPhysicalDeviceProperties(device, &device_props);
			vkGetPhysicalDeviceMemoryProperties(device, &device_memory_props);

			switch (device_props.deviceType)
			{
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:		device_scores[index] += 50; break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:	device_scores[index] += 5;	break;
			default: break;
				
			}

			uint64 device_memory_size = 0;
			for (int i = 0; i < device_memory_props.memoryHeapCount; i++) {
				device_memory_size += device_memory_props.memoryHeaps[i].size;
			}

			device_scores[index] += device_memory_size / 500'000'000;

			index++;
		}

		uint32 suitable_device_index = 0;
		for (int i = 0; i < physical_device_count; i++) 
		{
			if (device_scores[i] > device_scores[suitable_device_index])
				suitable_device_index = i;
		}

		delete[] device_scores;

		m_PhysicalDevice = physical_devices[suitable_device_index];

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProps);
		
		CURSED_CORE_TRACE("Selected Vulkan device: {0}", m_DeviceProps.deviceName);

		uint32 queue_family_property_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queue_family_property_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_property_count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queue_family_property_count, queue_family_properties.data());

		uint32 queue_family_index = 0;
		for (auto& family_prop : queue_family_properties) {
			if (family_prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
				{ m_Indices.graphics = queue_family_index; }
			else if (family_prop.queueFlags & VK_QUEUE_COMPUTE_BIT)	
				{ m_Indices.compute = queue_family_index; }
			else if (family_prop.queueFlags & VK_QUEUE_TRANSFER_BIT)
				{ m_Indices.transfer = queue_family_index; }

			queue_family_index++;
		}
		
		m_Indices.present = m_Indices.graphics;
	}

	std::vector<VkPhysicalDevice> VulkanPhysicalDevice::List(VulkanGraphicsContext* ctx)
	{
		uint32 physical_device_count = 0;
		vkEnumeratePhysicalDevices(ctx->GetVulkanInstance(), &physical_device_count, nullptr);

		std::vector<VkPhysicalDevice> devices(physical_device_count);
		vkEnumeratePhysicalDevices(ctx->GetVulkanInstance(), &physical_device_count, devices.data());

		return devices;
	}

	bool VulkanPhysicalDevice::IsExtensionSupported(const std::string& extension) const
	{
		uint32 count = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &count, nullptr);

		std::vector<VkExtensionProperties> supported_extensions(count);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &count, supported_extensions.data());

		for (auto& supported_ext : supported_extensions) {
			if (!strcmp(supported_ext.extensionName, extension.c_str())) {
				return true;
			}
		}

		CURSED_CORE_ERROR("Vulkan device extension \"{0}\" is not supported!", extension.c_str());

		return false;
	}

	// ==============
	// LOGICAL DEVICE
	// ==============
	VulkanDevice::VulkanDevice(Shared<VulkanPhysicalDevice> physical_device, const VkPhysicalDeviceFeatures& enabled_features)
		: m_Device(VK_NULL_HANDLE), m_PhysicalDevice(physical_device), m_GeneralQueue(VK_NULL_HANDLE), m_AsyncComputeQueue(VK_NULL_HANDLE)
	{
		const float prioriry = 1.0f;

		VkPhysicalDeviceFeatures supported_device_features = {};
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice->Raw(), &supported_device_features);

		VkDeviceQueueCreateInfo general_queue_create_info = {};
		general_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		general_queue_create_info.pQueuePriorities = &prioriry;
		general_queue_create_info.queueCount = 1;
		general_queue_create_info.queueFamilyIndex = m_PhysicalDevice->GetQueueFamilyIndices().graphics;
		
		VkDeviceQueueCreateInfo async_compute_queue_create_info = {};
		async_compute_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		async_compute_queue_create_info.pQueuePriorities = &prioriry;
		async_compute_queue_create_info.queueCount = 1;
		async_compute_queue_create_info.queueFamilyIndex = m_PhysicalDevice->GetQueueFamilyIndices().compute;

		VkDeviceQueueCreateInfo queue_create_infos[2] = { general_queue_create_info, async_compute_queue_create_info };
		std::vector<const char*> enabled_extensions = GetRequiredExtensions();

		VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature_enable_struct = {};
		dynamic_rendering_feature_enable_struct.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
		dynamic_rendering_feature_enable_struct.dynamicRendering = VK_TRUE;

		VkPhysicalDeviceIndexTypeUint8FeaturesEXT uint8_index_feature_enable_struct = {};
		uint8_index_feature_enable_struct.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT;
		uint8_index_feature_enable_struct.indexTypeUint8 = VK_TRUE;
		uint8_index_feature_enable_struct.pNext = &dynamic_rendering_feature_enable_struct;

		VkDeviceCreateInfo device_create_info = {};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pQueueCreateInfos = queue_create_infos;
		device_create_info.queueCreateInfoCount = 2;
		device_create_info.pEnabledFeatures = &enabled_features;
		device_create_info.ppEnabledExtensionNames = enabled_extensions.data();
		device_create_info.enabledExtensionCount = enabled_extensions.size();
		device_create_info.pNext = &uint8_index_feature_enable_struct;

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice->Raw(), &device_create_info, nullptr, &m_Device));
		vkGetDeviceQueue(m_Device, m_PhysicalDevice->GetQueueFamilyIndices().graphics, 0, &m_GeneralQueue);
		vkGetDeviceQueue(m_Device, m_PhysicalDevice->GetQueueFamilyIndices().compute, 0, &m_AsyncComputeQueue);

	}

	VulkanDevice::~VulkanDevice()
	{
		CURSED_ASSERT_TAGGED(!m_Device, "Vulkan device was not destroyed yet. Please, call VulkanDevice::Destroy().");
	}

	void VulkanDevice::Destroy()
	{
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}

	std::vector<const char*> VulkanDevice::GetRequiredExtensions()
	{
		std::vector<const char*> extensions;

		if (m_PhysicalDevice->IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
			extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}
		if (m_PhysicalDevice->IsExtensionSupported(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)) {
			extensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		}
		if (m_PhysicalDevice->IsExtensionSupported(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME)) {
			extensions.push_back(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME);
		}
		if (m_PhysicalDevice->IsExtensionSupported(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)) {
			extensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
		}

		CURSED_CORE_TRACE("Enabled Vulkan device extensions:");
		for (auto& ext : extensions) {
			CURSED_CORE_TRACE("\t{0}", ext);
		}

		return extensions;
	}

}