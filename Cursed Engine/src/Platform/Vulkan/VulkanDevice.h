#pragma once

#include "VulkanCommon.h"

namespace Cursed {

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

		static Shared<VulkanPhysicalDevice> Select(VulkanGraphicsContext* ctx);
		static std::vector<VkPhysicalDevice> List(VulkanGraphicsContext* ctx);

		VkPhysicalDevice Raw() const { return m_PhysicalDevice; }
		VkPhysicalDeviceProperties GetProps() const { return m_DeviceProps; }
		QueueFamilyIndex GetQueueFamilyIndices() const { return m_Indices; }

		bool IsExtensionSupported(const std::string& extension) const;

	private:
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties m_DeviceProps;
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

		VkCommandBuffer AllocateTransientCmdBuffer() const;
		void ExecuteTransientCmdBuffer(VkCommandBuffer cmd_buffer, bool wait = false) const;

	private:
		std::vector<const char*> GetRequiredExtensions();

	private:
		VkDevice m_Device;
		Shared<VulkanPhysicalDevice> m_PhysicalDevice;
		VkQueue m_GeneralQueue;
		VkQueue m_AsyncComputeQueue;

		VkCommandPool m_CmdPool;
	};

}