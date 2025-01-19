#pragma once

#include <Foundation/Common.h>

#include <Renderer/RendererCommon.h>
#include <Renderer/Image.h>

#include <imgui.h>

namespace Omni {
	class OMNIFORCE_API ImGuiRenderer {
	public:

		static ImGuiRenderer* Create();

		virtual void Launch(void* window_handle) = 0;
		virtual void Destroy() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void OnRender() = 0;
	};

	namespace UI {
		void OMNIFORCE_API UnregisterImage(Ref<Image> image);
		void OMNIFORCE_API RenderImage(Ref<Image> image, Ref<ImageSampler> sampler, ImVec2 size, uint32 image_layer = 0, bool flip = false);
		bool OMNIFORCE_API RenderImageButton(Ref<Image> image, Ref<ImageSampler> sampler, ImVec2 size, uint32 image_layer = 0, bool flip = false);
	}
}