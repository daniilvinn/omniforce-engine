#pragma once

#include "EditorPanel.h"

#include <Scene/Entity.h>

#include <glm/glm.hpp>

namespace Omni {

	class PropertiesPanel : public EditorPanel {
	public:
		PropertiesPanel(Scene* ctx) : EditorPanel(ctx) { m_IsOpen = true; };

		// If no entities selected, then nothing to show
		void Update() override;

		void SetEntity(Entity entity, bool selected);

	private:
		Entity m_Entity;
		bool m_Selected = false;
	};

}