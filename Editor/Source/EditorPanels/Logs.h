#pragma once 

#include <Foundation/Common.h>
#include "EditorPanel.h"

#include <spdlog/spdlog.h>

namespace Omni {

	struct EditorLogPanelEntry {
		std::string message = "";
		Logger::Level severity = Logger::Level::LEVEL_NONE;
		LogSource source = LogSource::CLIENT;
	};

	constexpr std::string_view LogSourceToString(LogSource source) {
		switch (source)
		{
		case Omni::LogSource::CORE:			return "Engine";
		case Omni::LogSource::CLIENT:		return "Game";
		default:							std::unreachable();
		}
	}

	class LogsPanel : public EditorPanel {
	public:
		LogsPanel(Scene* ctx);
		~LogsPanel();

		void Update() override;
		void PushMessage(const EditorLogPanelEntry& msg);

	private:
		Shared<spdlog::logger> m_Logger;

		// The simplest implementation, though probably the worst one
		// TODO: implement ring buffer
		std::vector<EditorLogPanelEntry> m_Messages;

		struct Filters {
			bool trace = true;
			bool info = true;
			bool warn = true;
			bool error = true;
			bool critical = true;
			bool engine = true;
			bool game = true;
		} m_FilterTogglesStatus;

		BitMask m_Filters = 0b1111111;
	};

}