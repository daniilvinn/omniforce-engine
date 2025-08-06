#pragma once

#include <Foundation/Common.h>

#include <RHI/ForwardDecl.h>

namespace Omni {
	class OMNIFORCE_API RendererAPI {
	public:
		RendererAPI() {};
		virtual ~RendererAPI() {};

		virtual RendererConfig GetConfig() const = 0;
		virtual uint32 GetCurrentFrameIndex() const = 0;
		virtual uint32 GetDeviceMinimalUniformBufferAlignment() const = 0;
		virtual uint32 GetDeviceMinimalStorageBufferAlignment() const = 0;
		virtual uint32 GetDeviceOptimalTaskWorkGroupSize() const = 0;;
		virtual uint32 GetDeviceOptimalMeshWorkGroupSize() const = 0;;
		virtual uint32 GetDeviceOptimalComputeWorkGroupSize() const = 0;

		virtual RendererCapabilities GetCapabilities() const = 0;
		virtual Ref<Swapchain> GetSwapchain() = 0;
		virtual Ref<DeviceCmdBuffer> GetCmdBuffer() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void BeginRender(const std::vector<Ref<Image>> attachments, uvec3 render_area, ivec2 render_offset, fvec4 clear_color, bool clear_depth) = 0;
		virtual void EndRender(Ref<Image> target) = 0;
		virtual void WaitDevice() = 0;
		virtual void BindSet(Ref<DescriptorSet> set, Ref<Pipeline> pipeline, uint8 index) = 0;
		virtual void BindSet(Ref<DescriptorSet> set, Ref<RTPipeline> pipeline, uint8 index) = 0;
		virtual void CopyToSwapchain(Ref<Image> image) = 0;
		virtual void InsertBarrier(const PipelineBarrierInfo& barrier) = 0;

		virtual void BeginCommandRecord() = 0;
		virtual void EndCommandRecord() = 0;
		virtual void ExecuteCurrentCommands() = 0;

		virtual void ClearImage(Ref<Image> image, const fvec4& value) = 0;
		virtual void RenderMeshTasks(Ref<Pipeline> pipeline, const glm::uvec3& dimensions, MiscData data) = 0;
		virtual void RenderMeshTasksIndirect(Ref<Pipeline> pipeline, Ref<DeviceBuffer> params, MiscData data) = 0;
		virtual void RenderQuad(Ref<Pipeline> pipeline, MiscData data) = 0;
		virtual void RenderQuad(Ref<Pipeline> pipeline, uint32 amount, MiscData data) = 0;
		virtual void DispatchCompute(Ref<Pipeline> pipeline, const glm::uvec3& dimensions, MiscData data) = 0;
		virtual void RenderUnindexed(Ref<Pipeline> pipeline, Ref<DeviceBuffer> vertex_buffer, MiscData data) = 0;
		virtual void DispatchRayTracing(Ref<RTPipeline> pipeline, const glm::uvec3& grid, MiscData data) = 0;

		virtual void RenderImGui() = 0;

	private:
		

	};

};