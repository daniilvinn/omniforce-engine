#include "../Renderer.h"

#include <Platform/Vulkan/VulkanRenderer.h>

namespace Cursed {

	RendererAPI* Renderer::s_RendererAPI;

	void Renderer::Init(const RendererConfig& config)
	{
		s_RendererAPI = new VulkanRenderer(config);
	}

	void Renderer::Shutdown()
	{
		delete s_RendererAPI;
	}


}