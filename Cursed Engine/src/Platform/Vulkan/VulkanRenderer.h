#pragma once

#include <Renderer/RendererAPI.h>
#include "VulkanCommon.h"

namespace Cursed {

	class VulkanRenderer : public RendererAPI {
	public:
		VulkanRenderer(const RendererConfig& config);
		~VulkanRenderer() override;

	private:
		Shared<VulkanGraphicsContext> m_GraphicsContext;

	};

}
