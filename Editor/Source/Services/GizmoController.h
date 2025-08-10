#pragma once

#include <Foundation/Common.h>
#include <imgui.h>
#include <ImGuizmo.h>

namespace Omni { class EditorCamera; }

namespace Omni::EditorServices {

    class EditorContext;

    // Handles ImGuizmo-based transform manipulation for the currently selected entity.
    class GizmoController {
    public:
        void SetContext(EditorContext* ctx) { m_Context = ctx; }

        void SetOperation(ImGuizmo::OPERATION op) { m_CurrentOperation = op; }
        ImGuizmo::OPERATION GetOperation() const { return m_CurrentOperation; }

        void UpdateAndDraw();

    private:
        EditorContext* m_Context = nullptr;
        ImGuizmo::OPERATION m_CurrentOperation = (ImGuizmo::OPERATION)0;
    };

}


