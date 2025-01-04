#include <Foundation/Common.h>
#include <Renderer/UI/ImGuiRenderer.h>

#include <Platform/Vulkan/UI/VulkanImGuiRenderer.h>

namespace Omni {

	ImGuiRenderer* ImGuiRenderer::Create()
	{
		return new VulkanImGuiRenderer;
	}

}