#pragma once

#include "EditorPanels/EditorPanel.h"

#include <imgui.h>

namespace Omni {

	class ViewportPanel;

	// Renders the viewport overlay controls (Play/Stop, View Mode, Exposure) at a position retrieved from ViewportPanel
	class ViewportOverlayPanel : public EditorPanel {
	public:
		ViewportOverlayPanel(Scene* ctx, ViewportPanel* viewport_panel);
		void Update() override;

	private:
		ViewportPanel* m_ViewportPanel = nullptr;
		int m_ViewMode = 0; // 0 Lit, 1 Unlit, 2 Wireframe
		float m_Exposure = 1.0f;
        Ref<Image> m_FadeImage;
	};

}