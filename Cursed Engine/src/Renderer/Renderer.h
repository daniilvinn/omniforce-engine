#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Core/Windowing/AppWindow.h>

#include <Renderer/ForwardDecl.h>

namespace Cursed {

	struct RendererConfig {
		AppWindow* main_window;
		uint32 frames_in_flight;
		bool vsync;
	};

	class CURSED_API Renderer {
	public:

		using RenderFunction = std::function<void()>;

		static void Init(const RendererConfig& config);
		static void Shutdown();

		static void Submit(RenderFunction func);
		static void BeginFrame();
		static void EndFrame();
		static Shared<Image> GetSwapchainImage();
		static void ClearImage(Shared<Image> image, const fvec4& value);



		static void Render();

	private:
		static RendererAPI* s_RendererAPI;

	};

}
