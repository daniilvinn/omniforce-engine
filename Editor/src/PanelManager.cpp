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
		m_Panels.emplace("scene_hierarchy", new PropertiesPanel(nullptr));
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
		for (auto panel : m_Panels)
			panel.second->Render();

		for (auto& exec : m_SimplePanels)
			exec.second();
	}

}