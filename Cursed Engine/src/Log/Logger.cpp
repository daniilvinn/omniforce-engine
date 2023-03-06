#include "Logger.h"
#include <Core/Utils.h>

#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Cursed {

	Cursed::Logger* Logger::s_Instance;

	static constexpr spdlog::level::level_enum CursedToSpdlogLevel(const Logger::Level& level) {
		switch (level)
		{
		case Cursed::Logger::Level::LEVEL_TRACE:
			return spdlog::level::trace;
			break;
		case Cursed::Logger::Level::LEVEL_INFO:
			return spdlog::level::info;
			break;
		case Cursed::Logger::Level::LEVEL_WARN:
			return spdlog::level::warn;
			break;
		case Cursed::Logger::Level::LEVEL_ERROR:
			return spdlog::level::err;
			break;
		case Cursed::Logger::Level::LEVEL_CRITICAL:
			return spdlog::level::critical;
			break;
		case Cursed::Logger::Level::LEVEL_NONE:
			return spdlog::level::off;
			break;
		default:
			break;
		}
	}

	Logger::Logger(Level level)
	{
		auto ring_buffer_sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(128);
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		auto distribute_sink = std::make_shared<spdlog::sinks::dist_sink_mt>();

		spdlog::sinks_init_list sink_list = { ring_buffer_sink, console_sink };

		m_CoreLogger = std::make_shared<spdlog::logger>("Cursed", sink_list);
		m_ClientLogger = std::make_shared<spdlog::logger>("Client", sink_list);
		m_FileLogger = std::make_shared<spdlog::logger>("Crash log", distribute_sink);
		
		m_CoreLogger->set_pattern("%^[%T][%n][%l]: %v%$");
		m_ClientLogger->set_pattern("%^[%T][%n][%l]: %v%$");

		m_CoreLogger->set_level(CursedToSpdlogLevel(level));
		m_ClientLogger->set_level(CursedToSpdlogLevel(level));
	}

	Logger::~Logger()
	{

	}

	void Logger::Init(Level level)
	{
		s_Instance = new Logger(level);
	}

	void Logger::Shutdown()
	{
		delete s_Instance;
	}

	void Logger::WriteLogFile()
	{
		std::string filename = std::string("logs/crash_log_") + Utils::EvaluateDatetime() + ".txt";
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename);

		auto distribute_sink = std::static_pointer_cast<spdlog::sinks::dist_sink_mt>(m_FileLogger->sinks()[0]);
		auto ring_buffer = std::static_pointer_cast<spdlog::sinks::ringbuffer_sink_mt>(m_CoreLogger->sinks()[0]);

		std::vector<std::string> messages = ring_buffer->last_formatted(128);

		distribute_sink->add_sink(file_sink);
		m_FileLogger->set_pattern("%v");

		for (auto& msg : messages) {
			m_FileLogger->critical(msg);
		}

		distribute_sink->remove_sink(file_sink);
	}

}