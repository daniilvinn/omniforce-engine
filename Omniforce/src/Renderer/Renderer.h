#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Core/Windowing/AppWindow.h>

#include <Renderer/ForwardDecl.h>

namespace Omni {

	struct RendererConfig {
		AppWindow* main_window;
		uint32 frames_in_flight;
		bool vsync;
	};

	class OMNIFORCE_API Renderer {
	public:

		using RenderFunction = std::function<void()>;

		static void Init(const RendererConfig& config);
		static void Shutdown();

		static void LoadShaderPack();
		static void Submit(RenderFunction func);

		static void BeginFrame();
		static void EndFrame();
		static void BeginRender(Shared<Image> target, uvec2 render_area, ivec2 offset, fvec4 clear_value);
		static void EndRender(Shared<Image> target);
		static void WaitDevice(); // to be used ONLY while shutting down the engine.

		static Shared<Image> GetSwapchainImage();
		static void ClearImage(Shared<Image> image, const fvec4& value);
		static void InsertBarrier(Shared<DeviceBuffer> buffer);
		static void RenderMesh(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo);


		static void Render();

	private:
		static RendererAPI* s_RendererAPI;

	};

}
