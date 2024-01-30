#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Core/Windowing/AppWindow.h>

#include <Renderer/ForwardDecl.h>
#include "DeviceCmdBuffer.h"

namespace Omni {

	struct RendererConfig {
		AppWindow* main_window;
		uint32 frames_in_flight;
		bool vsync;
	};

	struct RendererCapabilities {
		bool mesh_shading;
		bool ray_tracing;
	};

	class OMNIFORCE_API Renderer {
	public:

		using RenderFunction = std::function<void()>;

		static void Init(const RendererConfig& config);
		static void Shutdown();

		static RendererConfig GetConfig();
		static uint32 GetCurrentFrameIndex();
		static uint32 GetDeviceMinimalUniformBufferOffsetAlignment();
		static uint32 GetDeviceMinimalStorageBufferOffsetAlignment();
		static Shared<ImageSampler> GetNearestSampler();
		static Shared<ImageSampler> GetLinearSampler();
		static Shared<Image> GetSwapchainImage();
		static Shared<DeviceCmdBuffer> GetCmdBuffer();

		static void LoadShaderPack();
		static void Submit(RenderFunction func);

		static void BeginFrame();
		static void EndFrame();
		static void BeginRender(Shared<Image> target, uvec3 render_area, ivec2 offset, fvec4 clear_value);
		static void EndRender(Shared<Image> target);
		static void WaitDevice(); // to be used ONLY while shutting down the engine.
		static void BindSet(Shared<DescriptorSet> set, Shared<Pipeline> pipeline, uint8 index);
		static void CopyToSwapchain(Shared<Image> image);

		static void ClearImage(Shared<Image> image, const fvec4& value);
		static void InsertBarrier(Shared<Image> image);
		static void RenderMesh(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo, MiscData data);
		static void RenderQuads(Shared<Pipeline> pipeline, MiscData data);
		static void RenderQuads(Shared<Pipeline> pipeline, uint32 amount, MiscData data);
		static void RenderLines(Shared<Pipeline> pipeline, uint32 amount, MiscData data);

		static void Render();
		static void RenderImGui();

	private:
		static RendererAPI* s_RendererAPI;

	};

}
