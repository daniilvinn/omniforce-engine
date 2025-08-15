#include <Asset/Importers/ImageImporter.h>

#include <Rendering/UI/ImGuiRenderer.h>
#include <Rendering/PathTracing/PathTracingSceneRenderer.h>

#include "../ViewportOverlayPanel.h"
#include "../ViewportPanel.h"
#include "Services/EditorContext.h"
#include "Services/SelectionService.h"
#include "PanelManager.h"
#include <Scene/Component.h>
#include "EditorCamera.h"

#include <imgui.h>

namespace Omni {

    ViewportOverlayPanel::ViewportOverlayPanel(Scene* ctx, ViewportPanel* viewport_panel)
        : EditorPanel(ctx)
        , m_ViewportPanel(viewport_panel)
    {
        m_IsOpen = true;
        
        // Initialize exposure from renderer if available
        if (auto renderer = GetPathTracingRenderer())
        {
            m_Exposure = renderer->GetExposure();
        }
        
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
		ImGui::EndDisabled();
		// Play/Stop button
		bool in_runtime = m_EditorContext && m_EditorContext->IsInRuntime();
		if (ImGui::Button(in_runtime ? "Stop" : "Play", ImVec2(70.0f, 0.0f)))
		{
			ToggleRuntime();
		}
		ImGui::BeginDisabled(m_Context->IsInRuntime());

		ImGui::SameLine();

		// View mode combo
		ImGui::SetNextItemWidth(110);
		ImGui::Combo("##viewport_viewmode", &m_ViewMode, "Lit\0Unlit\0Wireframe\0\0");

			ImGui::SameLine();

			// Exposure slider
			ImGui::SetNextItemWidth(140);
			if (ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 4.0f, "%.2f"))
			{
				// Update renderer exposure without triggering scene retracing
				if (auto renderer = GetPathTracingRenderer())
				{
					renderer->SetExposure(m_Exposure);
				}
			}
		}
		ImGui::End();
	}

	PathTracingSceneRenderer* ViewportOverlayPanel::GetPathTracingRenderer()
	{
		if (!m_Context) return nullptr;
		
		auto renderer = m_Context->GetRenderer();
		if (!renderer) return nullptr;
		
		// Cast to PathTracingSceneRenderer
		return dynamic_cast<PathTracingSceneRenderer*>(renderer.Raw());
	}

	void ViewportOverlayPanel::ToggleRuntime()
	{
		if (!m_EditorContext) return;
		
		bool enter_runtime = !m_EditorContext->IsInRuntime();
		m_EditorContext->SetInRuntime(enter_runtime);

		// Get selection service from editor context to preserve selection
		auto selection_service = m_EditorContext->GetSelectionService();
		Omni::UUID selected_node;
		if (selection_service && selection_service->HasSelection())
			selected_node = selection_service->GetSelected().GetComponent<UUIDComponent>();

		if (enter_runtime)
		{
			Scene* runtime = new Scene(m_EditorContext->GetEditorScene());
			m_EditorContext->SetRuntimeScene(runtime);
			runtime->LaunchRuntime();
			m_EditorContext->SetCurrentScene(runtime);
		}
		else
		{
			Scene* runtime = m_EditorContext->GetRuntimeScene();
			if (runtime)
			{
				runtime->ShutdownRuntime();
				delete runtime;
			}
			m_EditorContext->SetRuntimeScene(nullptr);
			m_EditorContext->SetCurrentScene(m_EditorContext->GetEditorScene());
			m_EditorContext->GetCurrentScene()->EditorSetCamera(m_EditorContext->GetEditorCamera());
		}

		if (selection_service && selection_service->HasSelection())
		{
			entt::entity entity_id = m_EditorContext->GetCurrentScene()->GetEntities().at(selected_node);
			selection_service->SetSelected(Entity(entity_id, m_EditorContext->GetCurrentScene()), true);
		}

		// Update context for all panels through PanelManager
		if (auto panel_manager = PanelManager::Get())
		{
			panel_manager->SetContext(m_EditorContext->GetCurrentScene());
		}
	}

}