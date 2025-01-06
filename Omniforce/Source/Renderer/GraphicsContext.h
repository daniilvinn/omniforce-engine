#pragma once

#include <Foundation/Common.h>

#include <Renderer/ForwardDecl.h>

namespace Omni {

	class OMNIFORCE_API GraphicsContext {
	public:
		static Ref<GraphicsContext> Create();
		
		virtual void Destroy() = 0;
		virtual Ref<Swapchain> GetSwapchain() = 0;

	};

}