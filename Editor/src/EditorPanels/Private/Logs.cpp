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
		editor_logger->set_level(spdlog::level::trace);
		game_logger->set_level(spdlog::level::trace);

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

			ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH  | ImGuiTableFlags_RowBg;

			// Filters
			if (ImGui::RadioButton("Trace", m_FilterTogglesStatus.trace)) {
				m_FilterTogglesStatus.trace = !m_FilterTogglesStatus.trace;
				m_Filters ^= BIT(0);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Info", m_FilterTogglesStatus.info)) {
				m_FilterTogglesStatus.info = !m_FilterTogglesStatus.info;
				m_Filters ^= BIT(1);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Warn", m_FilterTogglesStatus.warn)) {
				m_FilterTogglesStatus.warn = !m_FilterTogglesStatus.warn;
				m_Filters ^= BIT(2);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Error", m_FilterTogglesStatus.error)) {
				m_FilterTogglesStatus.error = !m_FilterTogglesStatus.error;
				m_Filters ^= BIT(3);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Critical", m_FilterTogglesStatus.critical)) {
				m_FilterTogglesStatus.critical = !m_FilterTogglesStatus.critical;
				m_Filters ^= BIT(4);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Engine", m_FilterTogglesStatus.engine)) {
				m_FilterTogglesStatus.engine = !m_FilterTogglesStatus.engine;
				m_Filters ^= BIT(5);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Game", m_FilterTogglesStatus.game)) {
				m_FilterTogglesStatus.game = !m_FilterTogglesStatus.game;
				m_Filters ^= BIT(6);
			}


			// Logs
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

				for (auto& msg : m_Messages) {

					bool show_message = (BitMask)msg.severity & m_Filters;
					show_message = show_message && ((((BitMask)msg.source) << 5) & m_Filters);
					if(!show_message)
						continue;

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