#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include "EditorPanels/Logs.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

namespace Omni {

	// Editor sink
	class EditorConsoleEngineSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		explicit EditorConsoleEngineSink(LogsPanel* ctx) : m_Context(ctx) {}

		virtual ~EditorConsoleEngineSink() = default;

		EditorConsoleEngineSink(const EditorConsoleEngineSink& other) = delete;
		EditorConsoleEngineSink& operator=(const EditorConsoleEngineSink& other) = delete;

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);

			m_Message.message = fmt::to_string(formatted);
			m_Message.severity = GetMessageSeverityLevel(msg.level);
			m_Message.source = LogSource::CORE;

			flush_();
		}

		void flush_() override
		{
			if (m_Message.severity == Logger::Level::LEVEL_NONE)
				return;
			
			m_Context->PushMessage(m_Message);
			
		}

	private:
		Logger::Level GetMessageSeverityLevel(spdlog::level::level_enum level)
		{
			switch (level)
			{
			case spdlog::level::trace:		return Logger::Level::LEVEL_TRACE;
			case spdlog::level::debug:		return Logger::Level::LEVEL_TRACE;
			case spdlog::level::info:		return Logger::Level::LEVEL_INFO;
			case spdlog::level::warn:		return Logger::Level::LEVEL_WARN;
			case spdlog::level::err:		return Logger::Level::LEVEL_ERROR;
			case spdlog::level::critical:	return Logger::Level::LEVEL_CRITICAL;
			default:						return Logger::Level::LEVEL_NONE;
			}

			return Logger::Level::LEVEL_NONE;;
		}

	private:
		LogsPanel* m_Context;
		EditorLogPanelEntry m_Message;
	};


	// Game sink
	class EditorConsoleClientSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		explicit EditorConsoleClientSink(LogsPanel* ctx) : m_Context(ctx) {}

		virtual ~EditorConsoleClientSink() = default;

		EditorConsoleClientSink(const EditorConsoleClientSink& other) = delete;
		EditorConsoleClientSink& operator=(const EditorConsoleClientSink& other) = delete;

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);

			m_Message.message = fmt::to_string(formatted);
			m_Message.severity = GetMessageSeverityLevel(msg.level);
			m_Message.source = LogSource::CLIENT;

			flush_();
		}

		void flush_() override
		{
			if (m_Message.severity == Logger::Level::LEVEL_NONE)
				return;

			m_Context->PushMessage(m_Message);
		}

	private:
		Logger::Level GetMessageSeverityLevel(spdlog::level::level_enum level)
		{
			switch (level)
			{
			case spdlog::level::trace:		return Logger::Level::LEVEL_TRACE;
			case spdlog::level::debug:		return Logger::Level::LEVEL_TRACE;
			case spdlog::level::info:		return Logger::Level::LEVEL_INFO;
			case spdlog::level::warn:		return Logger::Level::LEVEL_WARN;
			case spdlog::level::err:		return Logger::Level::LEVEL_ERROR;
			case spdlog::level::critical:	return Logger::Level::LEVEL_CRITICAL;
			default:						return Logger::Level::LEVEL_NONE;
			}

			return Logger::Level::LEVEL_NONE;;
		}

	private:
		LogsPanel* m_Context;
		EditorLogPanelEntry m_Message;
	};

}