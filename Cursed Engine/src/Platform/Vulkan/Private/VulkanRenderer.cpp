#include "../VulkanRenderer.h"

#include "../VulkanGraphicsContext.h"

namespace Cursed {

	VulkanRenderer::VulkanRenderer(const RendererConfig& config)
	{
		m_GraphicsContext = std::make_shared<VulkanGraphicsContext>();
	}

	VulkanRenderer::~VulkanRenderer()
	{

	}

}