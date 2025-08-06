#include <Foundation/Common.h>
#include <RHI/GraphicsContext.h>

#include "Platform/Vulkan/VulkanGraphicsContext.h"

namespace Omni {

	Ref<GraphicsContext> GraphicsContext::Create()
	{
		return CreateRef<VulkanGraphicsContext>(&g_PersistentAllocator);
	}

}