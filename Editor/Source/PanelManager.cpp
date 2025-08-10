#include "PanelManager.h"

#include "EditorPanels/SceneHierarchy.h"
#include "EditorPanels/Properties.h"
#include "EditorPanels/ContentBrowser.h"

namespace Omni {

	void PanelManager::Init()
	{
		m_Instance = new PanelManager;
	}

    PanelManager::PanelManager() {
        // Correct default registrations
        m_Panels.emplace("scene_hierarchy", new SceneHierarchyPanel(nullptr));
        m_Panels.emplace("properties", new PropertiesPanel(nullptr));
        m_Panels.emplace("content_browser", new ContentBrowser(nullptr));
    }

	PanelManager::~PanelManager()
	{
		m_Panels.clear();
		m_SimplePanels.clear();
	}

	void PanelManager::Update()
	{
		for (auto& panel : m_Panels)
			panel.second->Update();

		for (auto& exec : m_SimplePanels)
			exec.second();
	}

    // Ensure newly added panels immediately receive current contexts
    // Note: we keep AddPanel inline in header; this explanatory code is kept here for clarity.

}