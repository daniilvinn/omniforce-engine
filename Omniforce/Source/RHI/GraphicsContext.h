#pragma once

#include <Foundation/Common.h>

#include <RHI/ForwardDecl.h>

namespace Omni {

	class OMNIFORCE_API GraphicsContext {
	public:
		static Ref<GraphicsContext> Create();
		
		virtual Ref<Swapchain> GetSwapchain() = 0;

	};

}