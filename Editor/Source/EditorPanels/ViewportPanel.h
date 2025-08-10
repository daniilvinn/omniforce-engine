#pragma once

#include "EditorPanels/EditorPanel.h"

namespace Omni { class Image; class ImageSampler; }

namespace Omni::EditorServices { class GizmoController; }

namespace Omni {

    // Renders the scene output, handles focus and DnD, and triggers gizmo drawing.
    class ViewportPanel : public EditorPanel {
    public:
        ViewportPanel(Scene* ctx, EditorServices::GizmoController* gizmo)
            : EditorPanel(ctx), m_Gizmo(gizmo) { m_IsOpen = true; }

        void Update() override;

    private:
        void HandleDragAndDrop_();
        void UpdateViewportState_();

    private:
        EditorServices::GizmoController* m_Gizmo = nullptr;
    };

}



