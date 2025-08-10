#pragma once

#include <Foundation/Common.h>
#include <Scene/Entity.h>

namespace Omni::EditorServices {

    // Lightweight selection state holder. Panels can subscribe to changes later if needed.
    class SelectionService {
    public:
        void SetSelected(Entity entity, bool isSelected) {
            m_SelectedEntity = entity;
            m_HasSelection = isSelected && entity.Valid();
        }

        Entity GetSelected() const { return m_SelectedEntity; }
        bool HasSelection() const { return m_HasSelection; }

        void Clear() { m_SelectedEntity = { (entt::entity)0, nullptr }; m_HasSelection = false; }

    private:
        Entity m_SelectedEntity = { (entt::entity)0, nullptr };
        bool m_HasSelection = false;
    };

}


