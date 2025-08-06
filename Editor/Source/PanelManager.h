#pragma once

#include <robin_hood.h>

#include "EditorPanels/EditorPanel.h"


namespace Omni {

	class PanelManager {
	public:
		static void Init();
		~PanelManager();

		void Update();

		void SetContext(Scene* ctx) {
			m_Context = ctx;
			for (auto panel : m_Panels)
				panel.second->SetContext(ctx);
		}

		void AddPanel(std::string_view key, EditorPanel* panel)
		{
			m_Panels.emplace(key.data(), panel);
		}

		void RemovePanel(std::string_view key)
		{
			m_Panels.erase(key.data());
		}

		EditorPanel* GetPanel(std::string_view key)
		{
			return m_Panels.at(key.data());
		}

		void AddSimplePanel(std::string_view key, std::function<void()> exec)
		{
			m_SimplePanels.emplace(key.data(), exec);
		}

		void RemoveSimplePanel(std::string_view key)
		{
			m_SimplePanels.erase(key.data());
		}

	private:
		PanelManager();

	private:
		inline static PanelManager* m_Instance = nullptr;
		Scene* m_Context = nullptr;

		robin_hood::unordered_node_map<std::string, EditorPanel*> m_Panels;
		robin_hood::unordered_node_map<std::string, std::function<void()>> m_SimplePanels;

	};

}