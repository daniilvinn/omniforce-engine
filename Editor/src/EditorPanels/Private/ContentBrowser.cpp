#include "../ContentBrowser.h"

#include <Asset/AssetManager.h>
#include <Filesystem/Filesystem.h>

#include <fstream>
#include <filesystem>

#include <imgui.h>
#include <tinyfiledialogs/tinyfiledialogs.h>

namespace Omni {

	ContentBrowser::ContentBrowser(Scene* ctx)
		: EditorPanel(ctx)
	{
		m_IsOpen = true;
		m_WorkingDirectory = FileSystem::GetWorkingDirectory();
		m_CurrentDirectory = m_WorkingDirectory;
	}

	void ContentBrowser::SetContext(Scene* ctx)
	{
		m_WorkingDirectory = FileSystem::GetWorkingDirectory() /= "assets";
		m_CurrentDirectory = m_WorkingDirectory;
	}

	void ContentBrowser::Render()
	{
		auto texture_registry = AssetManager::Get()->GetTextureRegistry();

		if (m_IsOpen) {
			ImGui::Begin("Assets", &m_IsOpen);
			ImGui::BeginTable("content_browser", 2);
			ImGui::TableSetupColumn("test", ImGuiTableColumnFlags_WidthFixed, ImGui::GetWindowWidth() - 225, 0);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
			ImGui::Text("Asset registry");

			ImGui::TableNextColumn();
			if (ImGui::Button("Browse files", { 200.0f, 0.0f })) {
				const char* filters[] = {"*.png", "*.jpg", "*.jpeg"};

				char* filepath = tinyfd_openFileDialog(
					"Open file",
					std::filesystem::absolute("assets").string().c_str(),
					3,
					filters,
					NULL,
					false
				);

				if (filepath != NULL)
					return;

				AssetManager::Get()->LoadTexture(std::filesystem::path(filepath));
			}
			ImGui::EndTable();
			ImGui::Separator();

			ImGui::BeginDisabled(m_CurrentDirectory == m_WorkingDirectory);
				if (ImGui::Button("Go back"))
					m_CurrentDirectory = m_CurrentDirectory.parent_path();
			ImGui::EndDisabled();

			ImGui::SameLine();
			auto relative = std::filesystem::relative(m_CurrentDirectory, m_WorkingDirectory);
			ImGui::Text(relative.string().c_str());

			ImGui::Separator();

			std::filesystem::directory_iterator it(m_CurrentDirectory);
			for (auto& entry : it) {
				if (ImGui::Button(fmt::format("{}##cb_entry", entry.path().filename().string()).c_str())) {
					if (entry.is_directory())
						m_CurrentDirectory /= entry.path().filename();
				}
			}
			
			ImGui::End();
		}
	}
}