#include "../ViewportPanel.h"

#include <Rendering/UI/ImGuiRenderer.h>
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Scene/Component.h>
#include <Asset/AssetManager.h>
#include <Asset/Importers/ModelImporter.h>

#include "EditorCamera.h"
#include "Services/EditorContext.h"
#include "Services/GizmoController.h"

namespace Omni {

    void ViewportPanel::Update()
    {
        if (!m_IsOpen) return;

        // Zero padding around the image area
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        // Make viewport window background black to match renderer output (override style)
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,1));
        if (ImGui::Begin("Viewport", &m_IsOpen)) {
            UpdateViewportState_();

            // Render scene output to the viewport window
            ImVec2 size = ImGui::GetContentRegionAvail();
            ImVec2 overlay_position = ImGui::GetCursorPos();
            overlay_position.x += 8.0f;
            overlay_position.y += 8.0f;

            UI::RenderImage(m_Context->GetFinalImage(), m_Context->GetRenderer()->GetSamplerLinear(), size, 0, true);

            // Ensure camera aspect ratio matches the viewport size
            if (auto camera = m_Context->GetCamera(); camera)
                camera->SetAspectRatio(size.x / size.y);

            // Drag-and-drop for assets
            HandleDragAndDrop_();

            // Overlay: controls bar (auto-size, no background)
            {
                ImGui::SetCursorPos(overlay_position);
                ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0));
                ImGui::BeginChild("##vp_overlay", ImVec2(0, 0), false, overlayFlags);
                bool in_runtime = m_EditorContext && m_EditorContext->IsInRuntime();
                
                ImGui::SameLine();
                static int viewMode = 0; // 0 Lit, 1 Unlit, 2 Wireframe (placeholder)
                ImGui::SetNextItemWidth(110);
                ImGui::Combo("##vp_viewmode", &viewMode, "Lit\0Unlit\0Wireframe\0\0");
                ImGui::SameLine();
                static float exposure = 1.0f;
                ImGui::SetNextItemWidth(140);
                ImGui::SliderFloat("Exposure", &exposure, 0.1f, 4.0f, "%.2f");
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }

            // Draw gizmos only in editor mode
            if (m_EditorContext && !m_EditorContext->IsInRuntime() && m_Gizmo)
                m_Gizmo->UpdateAndDraw();
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::UpdateViewportState_()
    {
        // Compute viewport bounds in screen space for gizmo rect
        ImVec2 minRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 maxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 offset = ImGui::GetWindowPos();
        fvec2 bounds[2] = { { minRegion.x + offset.x, minRegion.y + offset.y }, { maxRegion.x + offset.x, maxRegion.y + offset.y } };
        if (m_EditorContext)
            m_EditorContext->SetViewportBounds(bounds);

        // Track focus for camera controls - use hover + focused combination for better detection
        if (m_EditorContext) {
            bool is_focused = ImGui::IsWindowFocused() || (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0));
            m_EditorContext->SetViewportFocused(is_focused);
        }
    }

    void ViewportPanel::HandleDragAndDrop_()
    {
        if (!ImGui::BeginDragDropTarget())
            return;

        ImGuiDragDropFlags targetFlags = 0;
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("content_browser_item", targetFlags);
        if (payload) {
            // Payload data includes the trailing null terminator (sender sends size+1)
            const char* cpath = reinterpret_cast<const char*>(payload->Data);
            std::filesystem::path filename = std::filesystem::path(std::string(cpath));
            if (filename.extension() == ".gltf" || filename.extension() == ".glb") {
                // Import model and spawn entities in the current scene
                AssetManager* assetManager = AssetManager::Get();
                ModelImporter importer;
                Ref<Model> model = AssetManager::Get()->GetAsset<Model>(importer.Import(filename));

                Entity root = m_Context->CreateEntity();
                auto& map = model->GetMap();
                for (auto& entry : map) {
                    Entity child = m_Context->CreateChildEntity(root);
                    child.GetComponent<TagComponent>().tag = assetManager->GetAsset<Material>(entry.second)->GetName();
                    child.AddComponent<MeshComponent>(MeshComponent{ entry.first, entry.second });
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

}


