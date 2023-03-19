#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Pointers.hpp>

#include "ForwardDecl.h"

namespace Omni {

	class OMNIFORCE_API GraphicsContext {
	public:
		static Shared<GraphicsContext> Create();
		
		virtual void Destroy() = 0;
		virtual Shared<Swapchain> GetSwapchain() = 0;

	};

}