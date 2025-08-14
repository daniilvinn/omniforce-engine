#include <Asset/Importers/ImageImporter.h>

#include <Rendering/UI/ImGuiRenderer.h>

#include "EditorCamera.h"
#include "../ViewportOverlayPanel.h"
#include "../ViewportPanel.h"
#include "Services/EditorContext.h"

#include <imgui.h>

namespace Omni {

    ViewportOverlayPanel::ViewportOverlayPanel(Scene* ctx, ViewportPanel* viewport_panel)
        : EditorPanel(ctx)
        , m_ViewportPanel(viewport_panel)
    {
        m_IsOpen = true;
        
        std::filesystem::path fade_path = "Resources/Textures/Fade.png";

        ImageSourceImporter importer;
        std::vector<byte> data = importer.ImportFromSource(fade_path);
        ImageSourceMetadata* additional_data = importer.GetMetadata(fade_path);

        ImageSpecification spec = ImageSpecification::Default();
        spec.extent = { (uint32)additional_data->width, (uint32)additional_data->height, 1 };
        spec.format = ImageFormat::RGBA32_SRGB;
        spec.mip_levels = 1;
        spec.pixels = std::move(data);
        OMNI_DEBUG_ONLY_CODE(spec.debug_name = "ViewportOverLayFadeImage");

        m_FadeImage = Image::Create(&g_PersistentAllocator, spec);
    }

	void ViewportOverlayPanel::Update()
	{
		if (!m_IsOpen || !m_ViewportPanel) return;

		// Get the position and size from the viewport panel
		ImVec2 viewport_cursor_pos = m_ViewportPanel->GetViewportCursorPos();
		ImVec2 viewport_window_pos = m_ViewportPanel->GetViewportWindowPos();
		
		// Calculate viewport content area bounds
		ImVec2 viewport_content_min = ImVec2(
			viewport_window_pos.x + viewport_cursor_pos.x,
			viewport_window_pos.y + viewport_cursor_pos.y - 1.0f
		);

		// Get actual viewport content size from the viewport panel
		ImVec2 viewport_content_size = m_ViewportPanel->GetViewportContentSize();

		// Create fade overlay window at the top of viewport
		ImGuiWindowFlags fade_window_flags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoInputs; // Make it non-interactive

		// Position the fade window at the top of the viewport
		ImGui::SetNextWindowPos(viewport_content_min, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(viewport_content_size.x, 50.0f), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.0f);

		// Remove window padding to eliminate any offset
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		if (ImGui::Begin("Viewport Fade", nullptr, fade_window_flags))
		{
			// Render the fade image to fill the entire window
			UI::RenderImage(
				m_FadeImage, m_Context->GetRenderer()->GetSamplerLinear(),
				ImVec2(viewport_content_size.x, 50.0f),
				0
			);
		}
		ImGui::End();
		
		// Restore window padding
		ImGui::PopStyleVar();

		// Create control overlay window positioned below the fade
		ImVec2 controls_screen_pos = ImVec2(
			viewport_content_min.x + 8.0f,
			viewport_content_min.y + 8.0f // 100px fade + 8px spacing
		);

		ImGuiWindowFlags controls_window_flags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;

		ImGui::SetNextWindowPos(controls_screen_pos, ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.0f);

		if (ImGui::Begin("Viewport Controls", &m_IsOpen, controls_window_flags))
		{
			// Play/Stop button
			bool in_runtime = m_EditorContext && m_EditorContext->IsInRuntime();
			if (ImGui::Button(in_runtime ? "Stop" : "Play", ImVec2(70.0f, 0.0f)))
			{
				//ToggleRuntime(); // TODO: Implement runtime toggle
			}

			ImGui::SameLine();

			// View mode combo
			ImGui::SetNextItemWidth(110);
			ImGui::Combo("##viewport_viewmode", &m_ViewMode, "Lit\0Unlit\0Wireframe\0\0");

			ImGui::SameLine();

			// Exposure slider
			ImGui::SetNextItemWidth(140);
			ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 4.0f, "%.2f");
		}
		ImGui::End();
	}

}