#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Memory/Pointers.hpp>

#include "ForwardDecl.h"

namespace Omni {
	struct RendererConfig;

	class OMNIFORCE_API RendererAPI {
	public:
		RendererAPI() {};
		virtual ~RendererAPI() {};

		virtual RendererConfig GetConfig() const = 0;
		virtual uint32 GetCurrentFrameIndex() const = 0;
		virtual uint32 GetDeviceMinimalUniformBufferAlignment() const = 0;
		virtual uint32 GetDeviceMinimalStorageBufferAlignment() const = 0;
		virtual Shared<Swapchain> GetSwapchain() = 0;
		virtual Shared<DeviceCmdBuffer> GetCmdBuffer() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void BeginRender(const std::vector<Shared<Image>> attachments, uvec3 render_area, ivec2 render_offset, fvec4 clear_color) = 0;
		virtual void EndRender(Shared<Image> target) = 0;
		virtual void WaitDevice() = 0;
		virtual void BindSet(Shared<DescriptorSet> set, Shared<Pipeline> pipeline, uint8 index) = 0;
		virtual void CopyToSwapchain(Shared<Image> image) = 0;

		virtual void BeginCommandRecord() = 0;
		virtual void EndCommandRecord() = 0;
		virtual void ExecuteCurrentCommands() = 0;

		virtual void ClearImage(Shared<Image> image, const fvec4& value) = 0;
		virtual void RenderMeshTasks(Shared<Pipeline> pipeline, const glm::vec3& dimensions, MiscData data) = 0;
		virtual void RenderQuad(Shared<Pipeline> pipeline, MiscData data) = 0;
		virtual void RenderQuad(Shared<Pipeline> pipeline, uint32 amount, MiscData data) = 0;
		virtual void RenderImGui() = 0;

	private:
		

	};

};