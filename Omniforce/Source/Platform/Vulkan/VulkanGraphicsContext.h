#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <Platform/Vulkan/VulkanSwapchain.h>
#include <Renderer/GraphicsContext.h>
#include <Renderer/Renderer.h>

namespace Omni {

	class VulkanGraphicsContext : public GraphicsContext {
	public:
		VulkanGraphicsContext() {};
		VulkanGraphicsContext(const RendererConfig& config);
		~VulkanGraphicsContext();
		void Destroy() override;

		static VulkanGraphicsContext* Get() { return s_Instance; }

		VkInstance GetVulkanInstance() const { return m_VulkanInstance; }
		Shared<VulkanDevice> GetDevice() const { return m_Device; }
		Shared<Swapchain> GetSwapchain() override { return ShareAs<Swapchain>(m_Swapchain); }

	private:
		static std::vector<const char*> GetVulkanExtensions();
		static std::vector<const char*> GetVulkanLayers();

	private:
		static VulkanGraphicsContext* s_Instance;

		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
		Shared<VulkanDevice> m_Device = nullptr;
		Shared<VulkanDebugUtils> m_DebugUtils = nullptr;
		Shared<VulkanSwapchain> m_Swapchain = nullptr;

	};

}