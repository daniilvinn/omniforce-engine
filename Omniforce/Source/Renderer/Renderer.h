#pragma once

#include <Foundation/Common.h>

#include <Core/Windowing/AppWindow.h>

#include <Renderer/ForwardDecl.h>
#include <Renderer/DeviceCmdBuffer.h>

namespace Omni {

	struct RendererConfig {
		AppWindow* main_window;
		uint32 frames_in_flight;
		bool vsync;
	};

	struct RendererCapabilities {
		bool mesh_shading;

		struct {
			bool support;
			uint32 shader_handle_size;
			uint32 shader_handle_alignment;
			uint32 shader_base_alignment;
		} ray_tracing;
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
		static uint32 GetDeviceOptimalTaskWorkGroupSize();
		static uint32 GetDeviceOptimalMeshWorkGroupSize();
		static uint32 GetDeviceOptimalComputeWorkGroupSize();
		static RendererCapabilities GetCapabilities();
		static Ref<ImageSampler> GetNearestSampler();
		static Ref<ImageSampler> GetLinearSampler();
		static Ref<Image> GetSwapchainImage();
		static Ref<DeviceCmdBuffer> GetCmdBuffer();

		static void LoadShaderPack();
		static void Submit(RenderFunction func);

		static void BeginFrame();
		static void EndFrame();
		static void BeginRender(const std::vector<Ref<Image>> attachments, uvec3 render_area, ivec2 offset, fvec4 clear_value, bool clear_depth = true);
		static void EndRender(Ref<Image> target);
		static void WaitDevice(); // to be used ONLY while shutting down the engine.
		static void BindSet(Ref<DescriptorSet> set, Ref<Pipeline> pipeline, uint8 index);
		static void BindSet(Ref<DescriptorSet> set, Ref<RTPipeline> pipeline, uint8 index);
		static void CopyToSwapchain(Ref<Image> image);
		static void InsertBarrier(const PipelineBarrierInfo& barrier_info);

		static void ClearImage(Ref<Image> image, const fvec4& value);
		static void RenderMeshTasks(Ref<Pipeline> pipeline, const glm::uvec3 dimensions, MiscData data);
		static void RenderMeshTasksIndirect(Ref<Pipeline> pipeline, Ref<DeviceBuffer> params, MiscData data);
		static void RenderQuads(Ref<Pipeline> pipeline, MiscData data);
		static void RenderQuads(Ref<Pipeline> pipeline, uint32 amount, MiscData data);
		static void DispatchCompute(Ref<Pipeline> pipeline, const glm::uvec3& dimensions, MiscData data);
		static void RenderUnindexed(Ref<Pipeline> pipeline, Ref<DeviceBuffer> vertex_buffer, MiscData data);
		static void DispatchRayTracing(Ref<RTPipeline> pipeline, const glm::uvec3& grid, MiscData data);

		static void Render();
		static void RenderImGui();

	private:
		static RendererAPI* s_RendererAPI;
	};

}
