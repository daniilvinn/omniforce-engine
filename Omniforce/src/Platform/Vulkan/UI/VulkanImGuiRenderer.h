#pragma once

#include <Renderer/UI/ImGuiRenderer.h>

#include "../VulkanCommon.h"
#include "imgui.h"

namespace Omni {

	class VulkanImGuiRenderer : public ImGuiRenderer {
	public:
		VulkanImGuiRenderer();
		~VulkanImGuiRenderer();

		void Launch(void* window_handle) override;
		void Destroy() override;

		void BeginFrame() override;
		void EndFrame() override;
		void OnRender() override;

	private:
		ImFont* m_MainFont;

	};
}