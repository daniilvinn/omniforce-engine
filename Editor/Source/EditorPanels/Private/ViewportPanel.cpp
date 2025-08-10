#include "../ViewportPanel.h"

#include <Rendering/UI/ImGuiRenderer.h>
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Scene/Component.h>
#include <Asset/AssetManager.h>
#include <Asset/Importers/ModelImporter.h>

#include "Services/EditorContext.h"
#include "Services/GizmoController.h"
#include "EditorCamera.h" // not directly used here

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
            UI::RenderImage(m_Context->GetFinalImage(), m_Context->GetRenderer()->GetSamplerLinear(), size, 0, true);

            // Ensure camera aspect ratio matches the viewport size
            if (auto camera = m_Context->GetCamera(); camera)
                camera->SetAspectRatio(size.x / size.y);

            // Drag-and-drop for assets
            HandleDragAndDrop_();

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

        // Track focus for camera controls
        if (m_EditorContext)
            m_EditorContext->SetViewportFocused(ImGui::IsWindowFocused());
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


