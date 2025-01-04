#include <Foundation/Log/Logger.h>

#include <Core/Utils.h>

#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Omni {

	Omni::Logger* Logger::s_Instance;

	static constexpr spdlog::level::level_enum OmniToSpdlogLevel(const Logger::Level& level) {
		switch (level)
		{
		case Omni::Logger::Level::LEVEL_TRACE:
			return spdlog::level::trace;
			break;
		case Omni::Logger::Level::LEVEL_INFO:
			return spdlog::level::info;
			break;
		case Omni::Logger::Level::LEVEL_WARN:
			return spdlog::level::warn;
			break;
		case Omni::Logger::Level::LEVEL_ERROR:
			return spdlog::level::err;
			break;
		case Omni::Logger::Level::LEVEL_CRITICAL:
			return spdlog::level::critical;
			break;
		case Omni::Logger::Level::LEVEL_NONE:
			return spdlog::level::off;
			break;
		default:
			std::unreachable();
		}
	}

	Logger::Logger(Level level)
	{;
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

		spdlog::sinks_init_list sink_list = { console_sink };

		m_CoreLogger = std::make_shared<spdlog::logger>("Omniforce", sink_list);
		m_ClientLogger = std::make_shared<spdlog::logger>("Client", sink_list);
		
		m_CoreLogger->set_pattern("%^[%T][%n][%l]: %v%$");
		m_ClientLogger->set_pattern("%^[%T][%n][%l]: %v%$");

		m_CoreLogger->set_level(OmniToSpdlogLevel(level));
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

	void Logger::WriteLogFile() const
	{
		std::string filename = std::string("logs/crash_log_") + Utils::EvaluateDatetime() + ".txt";
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename);

		auto distribute_sink = std::static_pointer_cast<spdlog::sinks::dist_sink_mt>(m_FileLogger->sinks()[0]);
		auto ring_buffer = std::static_pointer_cast<spdlog::sinks::ringbuffer_sink_mt>(m_CoreLogger->sinks()[0]);

		std::vector<std::string> messages = ring_buffer->last_formatted(128);

		distribute_sink->add_sink(file_sink);
		m_FileLogger->set_pattern("%v");

		OMNIFORCE_CORE_WARNING("Generating engine dump: {}", filename);

		for (auto& msg : messages) {
			m_FileLogger->critical(msg);
		}

		distribute_sink->remove_sink(file_sink);
	}

}