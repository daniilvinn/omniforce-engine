#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Core/Windowing/AppWindow.h>

#include <Renderer/ForwardDecl.h>

namespace Cursed {

	struct RendererConfig {
		Shared<AppWindow> main_window;
		uint32 frames_in_flight;
	};

	class CURSED_API Renderer {
	public:

		static void Init(const RendererConfig& config);
		static void Shutdown();

	private:
		static RendererAPI* s_RendererAPI;

	};

}
