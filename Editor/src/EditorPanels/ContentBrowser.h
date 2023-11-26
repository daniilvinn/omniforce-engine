#pragma once

#include "EditorPanel.h"

#include <Core/Timer.h>

#include <filesystem>

namespace Omni {

	class ContentBrowser : public EditorPanel {
	public:
		ContentBrowser(Scene* ctx);
		~ContentBrowser() {};

		void SetContext(Scene* ctx) override;

		void Render() override;

	private:
		void FetchCurrentDirectory();

	private:
		std::filesystem::path m_WorkingDirectory;
		std::filesystem::path m_CurrentDirectory;

		robin_hood::unordered_map<std::string, Shared<Image>> m_IconMap;
		Timer m_DirectoryFetchTimer;
		std::vector<std::filesystem::path> m_CurrentDirectoryEntries;

		bool m_CreateDirectoryWindowActive = false;

	};

}