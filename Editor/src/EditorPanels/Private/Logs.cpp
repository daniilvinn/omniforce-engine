#include "../Logs.h"

#include "../../EditorLogsSinks.h"
#include <Log/Logger.h>

#include <imgui.h>

namespace Omni {

	LogsPanel::LogsPanel(Scene* ctx)
		: EditorPanel(ctx)
	{
		m_IsOpen = true;
		
		Shared<EditorConsoleEngineSink> engine_sink = std::make_shared<EditorConsoleEngineSink>(this);
		Shared<EditorConsoleClientSink> game_sink = std::make_shared<EditorConsoleClientSink>(this);

		Shared<spdlog::logger> editor_logger = std::make_shared<spdlog::logger>("OmniEditor", engine_sink);
		Shared<spdlog::logger> game_logger = std::make_shared<spdlog::logger>("Game", game_sink);

		editor_logger->set_pattern("%v%$");
		game_logger->set_pattern("%v%$");

		Logger::AddLogger(editor_logger);
		Logger::AddLogger(game_logger);

		OMNIFORCE_CUSTOM_LOGGER_INFO("OmniEditor", "test from editor");
		OMNIFORCE_CUSTOM_LOGGER_INFO("Game", "test from game");

		m_Messages.reserve(257);
	}

	LogsPanel::~LogsPanel()
	{

	}

	void LogsPanel::Update()
	{
		if (m_IsOpen) {
			ImGui::Begin("Logs", &m_IsOpen);

			ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterV
				| ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg;


			ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.13f, 0.14f, 0.15f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.19f, 0.20f, 0.21f, 1.00f));
			if (ImGui::BeginTable("##log_panel_messages_table", 3, flags)) {

				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Severity");

				ImGui::TableNextColumn();
				ImGui::Text("Source");

				ImGui::TableNextColumn();
				ImGui::Text("Message");

				bool test = true;
				ImGui::RadioButton("Test", &test);

				for (auto& msg : m_Messages) {
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text(Logger::LogLevelToString(msg.severity).data());

					ImGui::TableNextColumn();
					ImGui::Text(LogSourceToString(msg.source).data());

					ImGui::TableNextColumn();
					ImGui::Text(msg.message.c_str());
				}

				ImGui::EndTable();
			}

			ImGui::PopStyleColor(2);
			
			ImGui::End();
		}

	}

	void LogsPanel::PushMessage(const EditorLogPanelEntry& msg)
	{
		m_Messages.push_back(msg);
		if (m_Messages.size() > 256)
			m_Messages.erase(m_Messages.begin());
	}

}