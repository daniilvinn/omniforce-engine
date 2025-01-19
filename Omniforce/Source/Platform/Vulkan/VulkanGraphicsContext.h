#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <Platform/Vulkan/VulkanDebugUtils.h>
#include <Platform/Vulkan/VulkanSwapchain.h>
#include <Renderer/GraphicsContext.h>
#include <Renderer/Renderer.h>

namespace Omni {

	class VulkanGraphicsContext : public GraphicsContext {
	public:
		VulkanGraphicsContext() {};
		VulkanGraphicsContext(const RendererConfig& config);

		static VulkanGraphicsContext* Get() { return s_Instance; }

		VkInstance GetVulkanInstance() const { return m_VulkanInstance; }
		Ref<VulkanDevice> GetDevice() const { return m_Device; }
		Ref<Swapchain> GetSwapchain() override { return m_Swapchain; }

	private:
		static std::vector<const char*> GetVulkanExtensions();
		static std::vector<const char*> GetVulkanLayers();

	private:
		static VulkanGraphicsContext* s_Instance;

		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
		Ref<VulkanDevice> m_Device;
		Ref<VulkanDebugUtils> m_DebugUtils;
		Ref<VulkanSwapchain> m_Swapchain;

	};

}