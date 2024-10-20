#pragma once

#include "VulkanCommon.h"

#include <shared_mutex>

namespace Omni {

	struct QueueFamilyIndex 
	{
		uint32 graphics;
		uint32 transfer;
		uint32 compute;
		uint32 present;
	};

	class VulkanPhysicalDevice {
	public:
		VulkanPhysicalDevice(VulkanGraphicsContext* ctx);
		~VulkanPhysicalDevice();

		static Shared<VulkanPhysicalDevice> Select(VulkanGraphicsContext* ctx);
		static std::vector<VkPhysicalDevice> List(VulkanGraphicsContext* ctx);

		VkPhysicalDevice Raw() const { return m_PhysicalDevice; }
		VkPhysicalDeviceProperties2 GetProperties() const { return m_DeviceProps; }
		QueueFamilyIndex GetQueueFamilyIndices() const { return m_Indices; }

		bool IsExtensionSupported(const std::string& extension) const;

	private:
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties2 m_DeviceProps;
		QueueFamilyIndex m_Indices;
	};

	class VulkanDevice {
	public:
		VulkanDevice(Shared<VulkanPhysicalDevice> physical_device, const VkPhysicalDeviceFeatures& features);
		~VulkanDevice();
		void Destroy();

		VkDevice Raw() const { return m_Device; }
		Shared<VulkanPhysicalDevice> GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkQueue GetGeneralQueue() const { return m_GeneralQueue; }
		VkQueue GetAsyncComputeQueue() const { return m_GeneralQueue; }

		VulkanDeviceCmdBuffer AllocateTransientCmdBuffer();
		void ExecuteTransientCmdBuffer(VulkanDeviceCmdBuffer cmd_buffer, bool wait = false) const;

	private:
		std::vector<const char*> GetRequiredExtensions();

	private:
		VkDevice m_Device;
		Shared<VulkanPhysicalDevice> m_PhysicalDevice;
		VkQueue m_GeneralQueue;
		VkQueue m_AsyncComputeQueue;

		VkCommandPool m_CmdPool;
		std::shared_mutex m_Mutex;
	};

}