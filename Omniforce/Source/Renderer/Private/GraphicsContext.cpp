#include <Foundation/Common.h>
#include <Renderer/GraphicsContext.h>

#include "Platform/Vulkan/VulkanGraphicsContext.h"

namespace Omni {

	Shared<GraphicsContext> GraphicsContext::Create()
	{
		return std::make_shared<VulkanGraphicsContext>();
	}

}