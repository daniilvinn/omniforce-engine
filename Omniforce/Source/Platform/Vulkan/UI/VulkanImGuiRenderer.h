#pragma once

#include <Foundation/Common.h>
#include <Rendering/UI/ImGuiRenderer.h>

#include <Platform/Vulkan/VulkanCommon.h>

#include <imgui.h>

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