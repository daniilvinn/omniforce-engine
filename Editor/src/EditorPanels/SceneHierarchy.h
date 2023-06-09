#pragma once

#include "EditorPanel.h"
#include <Scene/Entity.h>

namespace Omni {

	class SceneHierarchyPanel : public EditorPanel {
	public:
		SceneHierarchyPanel(Scene* ctx) : EditorPanel(ctx) { m_IsOpen = true; };
		~SceneHierarchyPanel();

		void Render() override;
		void SetContext(Scene* ctx) override;

		Entity GetSelectedNode() const { return m_SelectedNode; }
		bool IsNodeSelected() const { return m_IsSelected; }

	private:
		void RenderHierarchyNode(Entity entity);

	private:
		Entity m_SelectedNode;
		bool m_IsSelected = false;

	};

}