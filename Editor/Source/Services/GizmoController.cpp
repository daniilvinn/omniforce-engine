#include "GizmoController.h"

#include "EditorContext.h"
#include "SelectionService.h"

#include <Scene/Scene.h>
#include <Scene/Component.h>
#include <Scene/Camera.h>
#include "../EditorCamera.h"
#include <Core/Input/Input.h>
#include <Core/Input/KeyCode.h>
#include <Core/Utils.h>

namespace Omni::EditorServices {

    void GizmoController::UpdateAndDraw()
    {
        if (!m_Context) return;

        SelectionService* selection = m_Context->GetSelectionService();
        if (!selection) return;
        if (!selection->HasSelection()) return;

        Scene* scene = m_Context->GetCurrentScene();
        Ref<EditorCamera> camera = m_Context->GetEditorCamera();
        if (!scene || !camera) return;

        TRSComponent trs = selection->GetSelected().GetWorldTransform();

        glm::mat4 model = Utils::ComposeMatrix(trs.translation, trs.rotation, trs.scale);
        glm::mat4 view = camera->GetViewMatrix();
        glm::mat4 proj = camera->BuildNonReversedProjection();

        const fvec2* bounds = m_Context->GetViewportBounds();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(
            bounds[0].x,
            bounds[0].y,
            bounds[1].x - bounds[0].x,
            bounds[1].y - bounds[0].y
        );

        bool needs_snapping = Input::KeyPressed(KeyCode::KEY_LEFT_ALT);
        float snap_value[2] = { 0.0f, 0.0f };
        if (needs_snapping) {
            if (m_CurrentOperation & ImGuizmo::OPERATION::ROTATE_Z) {
                snap_value[0] = 45.0f;
            } else {
                snap_value[0] = 0.5f;
                snap_value[1] = 0.5f;
            }
        }

        ImGuizmo::Manipulate(
            (float*)&view,
            (float*)&proj,
            m_CurrentOperation,
            ImGuizmo::MODE::WORLD,
            (float*)&model,
            nullptr,
            snap_value
        );

        if (ImGuizmo::IsUsing()) {
            Entity entity = selection->GetSelected();
            TRSComponent& trs_component = entity.GetComponent<TRSComponent>();

            if (entity.GetComponent<HierarchyNodeComponent>().parent.Valid()) {
                Entity parent_entity = entity.GetParent();
                TRSComponent parent_trs = parent_entity.GetWorldTransform();
                glm::mat4 parent_transform = Utils::ComposeMatrix(parent_trs.translation, parent_trs.rotation, glm::vec3(1.0f));
                model = glm::inverse(parent_transform) * model;
            }

            Utils::DecomposeMatrix(model, &trs_component.translation, &trs_component.rotation, &trs_component.scale);
        }
    }

}


