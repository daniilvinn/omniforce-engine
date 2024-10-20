#include "../VulkanDevice.h"

#include "../VulkanGraphicsContext.h"
#include "../VulkanDeviceCmdBuffer.h"

namespace Omni {

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

		// Fetch and preferred work group size for mesh shading
		VkPhysicalDeviceMeshShaderPropertiesEXT* mesh_shader_properties = new VkPhysicalDeviceMeshShaderPropertiesEXT{};
		mesh_shader_properties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;

		m_DeviceProps = {};
		m_DeviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		m_DeviceProps.pNext = mesh_shader_properties;

		vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &m_DeviceProps);

		OMNIFORCE_CORE_TRACE("Selected Vulkan device: {0}", m_DeviceProps.properties.deviceName);
	}

	VulkanPhysicalDevice::~VulkanPhysicalDevice()
	{
		// TODO: test this code
		void* node = &m_DeviceProps;
		while (node != nullptr) {
			intptr_t offset_ptr = ((intptr_t)node + 4); // offsetting to pNext
			void* next_node = (void*)offset_ptr;

			delete node;
			node = next_node;
		}
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

		for (const auto& supported_ext : supported_extensions) {
			if (!strcmp(supported_ext.extensionName, extension.c_str())) {
				return true;
			}
		}

		OMNIFORCE_CORE_ERROR("Vulkan device extension \"{0}\" is not supported!", extension.c_str());

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

		VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shading_enable_struct = {};
		mesh_shading_enable_struct.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
		mesh_shading_enable_struct.meshShader = true;
		mesh_shading_enable_struct.taskShader = true;
		mesh_shading_enable_struct.meshShaderQueries = true;

		VkPhysicalDeviceIndexTypeUint8FeaturesEXT uint8_index_feature_enable_struct = {};
		uint8_index_feature_enable_struct.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT;
		uint8_index_feature_enable_struct.pNext = &mesh_shading_enable_struct;
		uint8_index_feature_enable_struct.indexTypeUint8 = true;

		VkPhysicalDeviceVulkan11Features vulkan_1_1_features = {};
		vulkan_1_1_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		vulkan_1_1_features.pNext = &uint8_index_feature_enable_struct;
		vulkan_1_1_features.shaderDrawParameters = true;
		vulkan_1_1_features.storageBuffer16BitAccess = true;

		VkPhysicalDeviceVulkan12Features vulkan_1_2_features = {};
		vulkan_1_2_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		vulkan_1_2_features.pNext = &vulkan_1_1_features;
		vulkan_1_2_features.descriptorIndexing = true;
		vulkan_1_2_features.shaderSampledImageArrayNonUniformIndexing = true;
		vulkan_1_2_features.descriptorBindingPartiallyBound = true;
		vulkan_1_2_features.runtimeDescriptorArray = true;
		vulkan_1_2_features.scalarBlockLayout = true;
		vulkan_1_2_features.bufferDeviceAddress = true;
		vulkan_1_2_features.bufferDeviceAddressCaptureReplay = OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG;
		vulkan_1_2_features.drawIndirectCount = true;
		vulkan_1_2_features.storageBuffer8BitAccess = true;
		vulkan_1_2_features.shaderInt8 = true;
		vulkan_1_2_features.storagePushConstant8 = true;
		vulkan_1_2_features.shaderFloat16 = true;

		VkPhysicalDeviceVulkan13Features vulkan_1_3_features = {};
		vulkan_1_3_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vulkan_1_3_features.pNext = &vulkan_1_2_features;
		vulkan_1_3_features.dynamicRendering = true;
		vulkan_1_3_features.maintenance4 = true;
		vulkan_1_3_features.synchronization2 = true;

		VkPhysicalDeviceFeatures2 device_features2 = {};
		device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		device_features2.features = enabled_features;
		device_features2.pNext = &vulkan_1_3_features;

		VkDeviceCreateInfo device_create_info = {};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pQueueCreateInfos = queue_create_infos;
		device_create_info.queueCreateInfoCount = 2;
		device_create_info.pEnabledFeatures = {};
		device_create_info.ppEnabledExtensionNames = enabled_extensions.data();
		device_create_info.enabledExtensionCount = enabled_extensions.size();
		device_create_info.pNext = &device_features2;

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice->Raw(), &device_create_info, nullptr, &m_Device));
		vkGetDeviceQueue(m_Device, m_PhysicalDevice->GetQueueFamilyIndices().graphics, 0, &m_GeneralQueue);
		vkGetDeviceQueue(m_Device, m_PhysicalDevice->GetQueueFamilyIndices().compute, 0, &m_AsyncComputeQueue);

		VkCommandPoolCreateInfo cmd_pool_create_info = {};
		cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_create_info.queueFamilyIndex = m_PhysicalDevice->GetQueueFamilyIndices().graphics;
		cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		VK_CHECK_RESULT(vkCreateCommandPool(m_Device, &cmd_pool_create_info, nullptr, &m_CmdPool));

	}

	VulkanDevice::~VulkanDevice()
	{
		OMNIFORCE_ASSERT_TAGGED(!m_Device, "Vulkan device was not destroyed yet. Please, call VulkanDevice::Destroy().");
	}

	void VulkanDevice::Destroy()
	{
		vkDestroyCommandPool(m_Device, m_CmdPool, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}

	VulkanDeviceCmdBuffer VulkanDevice::AllocateTransientCmdBuffer()
	{
		VulkanDeviceCmdBuffer cmd_buffer(DeviceCmdBufferLevel::PRIMARY, DeviceCmdBufferType::TRANSIENT, DeviceCmdType::GENERAL);
		cmd_buffer.Begin();

		return cmd_buffer;
	}

	void VulkanDevice::ExecuteTransientCmdBuffer(VulkanDeviceCmdBuffer cmd_buffer, bool wait /*= false*/) const
	{
		cmd_buffer.End();
		cmd_buffer.Execute(true); // TODO: make such feature that cmd buffer can be launched asynchronously and will be self-destroyed on work finished
		cmd_buffer.Destroy();
	}

	std::vector<const char*> VulkanDevice::GetRequiredExtensions()
	{
		std::vector<const char*> extensions;

		if (m_PhysicalDevice->IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
			extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}
		if (m_PhysicalDevice->IsExtensionSupported(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME)) {
			extensions.push_back(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME);
		}
		OMNIFORCE_ASSERT_TAGGED(m_PhysicalDevice->IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME), "Mesh shading is not supported, aborting execution");
		extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);

		if (m_PhysicalDevice->IsExtensionSupported(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME) && OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG) {
			extensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
		}

		if (m_PhysicalDevice->IsExtensionSupported(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)) {
			extensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		}

		OMNIFORCE_CORE_TRACE("Enabled Vulkan device extensions:");
		for (auto& ext : extensions) {
			OMNIFORCE_CORE_TRACE("\t{0}", ext);
		}

		return extensions;
	}

}