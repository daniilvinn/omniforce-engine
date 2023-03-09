#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Pointers.hpp>

namespace Cursed {

	class CURSED_API GraphicsContext {
	public:
		static Shared<GraphicsContext> Create();
		
		virtual void Destroy() = 0;

	};

}