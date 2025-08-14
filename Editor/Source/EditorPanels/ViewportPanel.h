#pragma once

#include "EditorPanels/EditorPanel.h"

#include <imgui.h>

namespace Omni { class Image; class ImageSampler; }

namespace Omni::EditorServices { class GizmoController; }

namespace Omni {

    // Renders the scene output, handles focus and DnD, and triggers gizmo drawing.
    class ViewportPanel : public EditorPanel {
    public:
        ViewportPanel(Scene* ctx, EditorServices::GizmoController* gizmo)
            : EditorPanel(ctx), m_Gizmo(gizmo) { m_IsOpen = true; }

        void Update() override;

        ImVec2 GetViewportCursorPos() const { return m_ViewportCursorPos; }
        ImVec2 GetViewportWindowPos() const { return m_ViewportWindowPos; }
        ImVec2 GetViewportContentSize() const { return m_ViewportContentSize; }

    private:
        void HandleDragAndDrop_();
        void UpdateViewportState_();

    private:
        EditorServices::GizmoController* m_Gizmo = nullptr;
        ImVec2 m_ViewportCursorPos = ImVec2(0, 0);
        ImVec2 m_ViewportWindowPos = ImVec2(0, 0);
        ImVec2 m_ViewportContentSize = ImVec2(0, 0);
    };

}



