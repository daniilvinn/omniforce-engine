#include "../GraphicsContext.h"

#include "Platform/Vulkan/VulkanGraphicsContext.h"

namespace Omni {

	Shared<GraphicsContext> GraphicsContext::Create()
	{
		return std::make_shared<VulkanGraphicsContext>();
	}

}