#include "../GraphicsContext.h"

#include "Platform/Vulkan/VulkanGraphicsContext.h"

namespace Cursed {

	Shared<GraphicsContext> GraphicsContext::Create()
	{
		return std::make_shared<VulkanGraphicsContext>();
	}

}