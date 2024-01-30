#include "../ContentBrowser.h"

#include <Asset/AssetManager.h>
#include <Asset/Importers/ImageImporter.h>
#include <Asset/AssetType.h>
#include <Filesystem/Filesystem.h>

#include <Renderer/UI/ImGuiRenderer.h>

#include <fstream>
#include <filesystem>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <tinyfiledialogs/tinyfiledialogs.h>

namespace Omni {

	const char* AssetTypeToIcon(AssetType type){
		switch (type)
		{
		case AssetType::MESH_SRC:		return "3d-modeling.png";
		case AssetType::MATERIAL:		return "document.png";
		case AssetType::AUDIO_SRC:		return "document.png";
		case AssetType::IMAGE_SRC:		return "image-gallery.png";
		case AssetType::OMNI_IMAGE:		return "image-gallery.png";
		case AssetType::OMNI_MESH:		return "3d_modeling.png";
		case AssetType::UNKNOWN:		return "document.png";
		}
	}

	ContentBrowser::ContentBrowser(Scene* ctx)
		: EditorPanel(ctx)
	{
		m_IsOpen = true;
		m_WorkingDirectory = FileSystem::GetWorkingDirectory();
		m_CurrentDirectory = m_WorkingDirectory;

		std::filesystem::directory_iterator i("Resources/icons");

		for (auto& entry : i) {
			ImageSourceImporter importer;
			std::vector<byte> data = importer.ImportFromSource(entry.path());
			ImageSourceMetadata* additional_data = importer.GetMetadata(entry.path());

			ImageSpecification spec = ImageSpecification::Default();
			spec.extent = { (uint32)additional_data->width, (uint32)additional_data->height, 1 };
			spec.format = ImageFormat::RGBA32_UNORM;
			spec.mip_levels = 1;
			spec.pixels = std::move(data);
			spec.mip_levels = 1;

			Shared<Image> image = Image::Create(spec);

			m_IconMap.emplace(entry.path().filename().string(), image);
			
			delete additional_data;
		}
	}

	ContentBrowser::~ContentBrowser()
	{
		for (auto& icon : m_IconMap) {
			icon.second->Destroy();
		}
	}

	void ContentBrowser::SetContext(Scene* ctx)
	{
		m_WorkingDirectory = FileSystem::GetWorkingDirectory() /= "assets";
		m_CurrentDirectory = m_WorkingDirectory;
	}

	void ContentBrowser::Render()
	{
		auto texture_registry = AssetManager::Get()->GetAssetRegistry();

		if (m_IsOpen) {
			ImGui::Begin("Content browser", &m_IsOpen);

			if (ImGui::BeginPopupContextWindow("##cb_create_directory_popup", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {

				if (ImGui::MenuItem("Create directory")) {
					m_CreateDirectoryWindowActive = true;
				}
				ImGui::EndPopup();
			}
			if(m_CreateDirectoryWindowActive) {
				ImGui::Begin("Create directory", &m_CreateDirectoryWindowActive, ImGuiWindowFlags_NoDocking);

				ImGui::Text("Directory name: ");

				ImGui::SameLine();
				static std::string text;
				ImGui::InputText("##cb_create_dir_window", &text);

				if (ImGui::Button("Create")) {
					m_CreateDirectoryWindowActive = false;
					std::filesystem::create_directory(m_CurrentDirectory / std::filesystem::path(text));
					FetchCurrentDirectory();
					text.clear();
				}

				ImGui::SameLine();
				if (ImGui::Button("Close")) {
					m_CreateDirectoryWindowActive = false;
				}

				ImGui::End();
			}

			ImGui::BeginDisabled(m_CurrentDirectory == m_WorkingDirectory);
			if (UI::RenderImageButton(m_IconMap["btn_back.png"], m_Context->GetRenderer()->GetSamplerLinear(), { 18,18 })) {
					m_CurrentDirectory = m_CurrentDirectory.parent_path();
					FetchCurrentDirectory();
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			auto relative = std::filesystem::relative(m_CurrentDirectory, m_WorkingDirectory).string();
			std::replace(relative.begin(), relative.end(), '\\', '/');

			// HACK
			if (relative.length() == 1 && relative[0] == '.')
				relative = "Project root";

			ImGui::SetCursorPos({ ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 5 });
			ImGui::Text(relative.c_str());

			ImGui::Separator();

			if (m_DirectoryFetchTimer.ElapsedMilliseconds() > 100) {
				FetchCurrentDirectory();
				m_DirectoryFetchTimer.Reset();
			}

			const float padding = 16.0f;
			const float thumbnail_size = 96.0f;
			const float cell_size = thumbnail_size + padding;

			float cb_panel_width = ImGui::GetContentRegionAvail().x;
			uint32 column_count = std::clamp((uint32)cb_panel_width / (uint32)cell_size, 1u, 15u);

			ImGui::BeginTable("##cb_content_table", column_count);
			ImGui::TableNextRow();
			uint32 item_id = column_count;
			bool filesystem_refetch_requested = false;

			for (auto& entry : m_CurrentDirectoryEntries) {
				if (!(item_id % column_count))
					ImGui::TableNextRow();

				ImGui::TableNextColumn();

				ImGui::PushID(fmt::format("cb_item_{}", entry.string()).c_str());

				if (std::filesystem::is_directory(entry)) {
					ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
					UI::RenderImageButton(m_IconMap["folder.png"], m_Context->GetRenderer()->GetSamplerLinear(), { thumbnail_size, thumbnail_size });
					ImGui::PopStyleColor();
				}
				else if (entry.extension() == ".txt") {
					UI::RenderImage(m_IconMap["document.png"], m_Context->GetRenderer()->GetSamplerLinear(), {thumbnail_size, thumbnail_size});
				}
				else {
					UI::RenderImage(m_IconMap[AssetTypeToIcon(FileExtensionToAssetType(entry.extension().string()))], m_Context->GetRenderer()->GetSamplerLinear(), { thumbnail_size, thumbnail_size });
				}

				ImGuiDragDropFlags drag_and_drop_flags = ImGuiDragDropFlags_None;
				drag_and_drop_flags |= ImGuiDragDropFlags_SourceAllowNullID;

				if (ImGui::BeginDragDropSource(drag_and_drop_flags))
				{
					if (!(drag_and_drop_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
						ImGui::Text(entry.string().c_str());
					ImGui::SetDragDropPayload("content_browser_item", entry.string().c_str(), entry.string().size());
					ImGui::EndDragDropSource();
				}

				ImGui::PopID();

				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup(fmt::format("##cb_item_actions_{}", entry.string()).c_str());
				}

				else if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && std::filesystem::is_directory(entry)) {
					m_CurrentDirectory /= entry.filename();
					filesystem_refetch_requested = true;
				}

				if (ImGui::BeginPopup(fmt::format("##cb_item_actions_{}", entry.string()).c_str())) {
					if (ImGui::MenuItem("Delete")) {
						std::filesystem::remove_all(entry);
						filesystem_refetch_requested = true;
					}
					if ((uint8)FileExtensionToAssetType(entry.extension().string()) < 4) {
						if (ImGui::MenuItem("Import")) {
							m_ConvertAssetWindowActive = true;
						}
					}
					ImGui::EndPopup();
				}

				ImGui::TextWrapped(entry.filename().string().c_str());

				item_id++;

			}
			ImGui::EndTable();

			if(filesystem_refetch_requested)
				FetchCurrentDirectory();

			ImGui::End();
		}
	}

	void ContentBrowser::FetchCurrentDirectory()
	{
		m_CurrentDirectoryEntries.clear();
		std::filesystem::directory_iterator it(m_CurrentDirectory);
		for (auto& entry : it) {
			if (entry.is_directory())
				m_CurrentDirectoryEntries.insert(m_CurrentDirectoryEntries.begin(), entry.path());
			else
				m_CurrentDirectoryEntries.push_back(entry.path());
		}
	}

}