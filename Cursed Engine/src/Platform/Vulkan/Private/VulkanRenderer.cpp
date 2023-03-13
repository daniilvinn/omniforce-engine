#include "../VulkanRenderer.h"

#include "../VulkanGraphicsContext.h"
#include "../VulkanDevice.h"

namespace Cursed {

	VulkanRenderer::VulkanRenderer(const RendererConfig& config)
	{
		m_GraphicsContext = std::make_shared<VulkanGraphicsContext>(config);

		auto base_swapchain = m_GraphicsContext->GetSwapchain();
		m_Swapchain = ShareAs<VulkanSwapchain>(base_swapchain);
	}

	VulkanRenderer::~VulkanRenderer()
	{
		
	}

	void VulkanRenderer::BeginFrame()
	{
		auto device = m_GraphicsContext->GetDevice();
		m_Swapchain->BeginFrame();
	}

	void VulkanRenderer::EndFrame()
	{
		m_Swapchain->EndFrame();
	}

}