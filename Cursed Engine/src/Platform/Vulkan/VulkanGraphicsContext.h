#pragma once

#include <Renderer/GraphicsContext.h>
#include "VulkanCommon.h"

#include <vector>

namespace Cursed {

	class VulkanGraphicsContext : public GraphicsContext {
	public:
		VulkanGraphicsContext();
		~VulkanGraphicsContext();

		void Destroy() override;

		VkInstance GetVulkanInstance() const { return m_VulkanInstance; }

	private:
		static std::vector<const char*> GetVulkanExtensions();
		static std::vector<const char*> GetVulkanLayers();

	private:
		VkInstance m_VulkanInstance;
		Shared<VulkanDevice> m_Device;
		Shared<VulkanDebugUtils> m_DebugUtils;
	};

}