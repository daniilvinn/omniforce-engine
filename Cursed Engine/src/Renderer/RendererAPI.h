#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Memory/Pointers.hpp>



namespace Cursed {
	struct RendererConfig;

	class CURSED_API RendererAPI {
	public:
		RendererAPI() {};
		virtual ~RendererAPI() {};

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

	private:
		

	};

};