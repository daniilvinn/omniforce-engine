#pragma once

#include "EditorPanel.h"

#include <filesystem>

namespace Omni {

	class ContentBrowser : public EditorPanel {
	public:
		ContentBrowser(Scene* ctx);
		~ContentBrowser() {};

		void SetContext(Scene* ctx) override;

		void Render() override;

	private:
		std::filesystem::path m_WorkingDirectory;
		std::filesystem::path m_CurrentDirectory;

	};

}