#pragma once

#include <Foundation/Common.h>
#include "EditorPanel.h"

#include <filesystem>

namespace Omni {

	class ContentBrowser : public EditorPanel {
	public:
		ContentBrowser(Scene* ctx);
		~ContentBrowser();

		void SetContext(Scene* ctx) override;

		void Update() override;

	private:
		void FetchCurrentDirectory();

	private:
		std::filesystem::path m_WorkingDirectory;
		std::filesystem::path m_CurrentDirectory;

		robin_hood::unordered_map<std::string, Ref<Image>> m_IconMap;
		Timer m_DirectoryFetchTimer;
		std::vector<std::filesystem::path> m_CurrentDirectoryEntries;

		bool m_CreateDirectoryWindowActive = false;
		bool m_ConvertAssetWindowActive = false;

	};

}