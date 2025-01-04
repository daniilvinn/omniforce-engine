#pragma once

#include <Foundation/Common.h>

#include <Renderer/ForwardDecl.h>

namespace Omni {

	class OMNIFORCE_API GraphicsContext {
	public:
		static Shared<GraphicsContext> Create();
		
		virtual void Destroy() = 0;
		virtual Shared<Swapchain> GetSwapchain() = 0;

	};

}