#pragma once

#include <iostream>

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

namespace Omni {

	enum class LogSource : uint8 {
		CORE = BIT(0),
		CLIENT = BIT(1)
	};

	class OMNIFORCE_API Logger 
	{
	public:
		enum class Level : uint8_t 
		{
			LEVEL_TRACE			= BIT(0),
			LEVEL_INFO			= BIT(1),
			LEVEL_WARN			= BIT(2),
			LEVEL_ERROR			= BIT(3),
			LEVEL_CRITICAL		= BIT(4),
			LEVEL_NONE			= BIT(5),
			MAX_ENUM			= BIT(6)
		};

		static constexpr std::string_view LogLevelToString(Level level) {
			switch (level)
			{
			case Omni::Logger::Level::LEVEL_TRACE:			return "Trace";
			case Omni::Logger::Level::LEVEL_INFO:			return "Info";
			case Omni::Logger::Level::LEVEL_WARN:			return "Warning";
			case Omni::Logger::Level::LEVEL_ERROR:			return "Error";
			case Omni::Logger::Level::LEVEL_CRITICAL:		return "Critical";
			case Omni::Logger::Level::LEVEL_NONE:			return "Null";
			default:										std::unreachable();
			}
		}

		static void Init(Level level);
		static void Shutdown();
		static Logger* Get() { return s_Instance; }
		static Shared<spdlog::logger> GetCoreLogger() { return s_Instance->m_CoreLogger; };
		static Shared<spdlog::logger> GetClientLogger() { return s_Instance->m_ClientLogger; };
		static Shared<spdlog::logger> GetLoggerByName(std::string_view name) { return spdlog::get(name.data()); }
		//static Shared<spdlog::logger> GetLoggerByName(std::string_view name) { return s_Instance->m_AdditionalLoggers.at(name.data()); }
		static void AddLogger(Shared<spdlog::logger> logger) { spdlog::register_logger(logger); }
		static void SetClientLoggerSink(Shared<spdlog::sinks::base_sink<std::mutex>> sink) { s_Instance->m_ClientLogger->sinks().push_back(sink); }
		
		// Core logging API
		template<typename... Args>
		void Trace(Args... args)			{ m_CoreLogger->trace(args...); }

		template<typename... Args>
		void Info(Args... args)				{ m_CoreLogger->info(args...); }
		
		template<typename... Args>
		void Warn(Args... args)				{ m_CoreLogger->warn(args...); }

		template<typename... Args>
		void Error(Args... args)			{ m_CoreLogger->error(args...); }

		template<typename... Args>
		void Critical(Args... args)			{ m_CoreLogger->critical(args...); }


		// Client logging API
		template<typename... Args>
		void ClientTrace(Args... args)		{ m_ClientLogger->trace(args...); }

		template<typename... Args>
		void ClientInfo(Args... args)		{ m_ClientLogger->info(args...); }

		template<typename... Args>
		void ClientWarn(Args... args)		{ m_ClientLogger->warn(args...); }

		template<typename... Args>
		void ClientError(Args... args)		{ m_ClientLogger->error(args...); }

		template<typename... Args>
		void ClientCritical(Args... args)	{ m_ClientLogger->critical(args...); }

		// Flushing logs into a file could be not efficient enough, 
		// since I recreate file sink every time engine initiates logging into a file
		void WriteLogFile() const;

	
		Logger(Level level);
		~Logger();

	
		static Logger* s_Instance;
		Shared<spdlog::logger> m_CoreLogger;
		Shared<spdlog::logger> m_ClientLogger;
		Shared<spdlog::logger> m_FileLogger;
		std::string m_LogBuffer;
	};

#ifndef OMNIFORCE_RELEASE
	#define OMNIFORCE_INITIALIZE_LOG_SYSTEM(log_level) Omni::Logger::Init(log_level);
	#define OMNIFORCE_WRITE_LOGS_TO_FILE()		Omni::Logger::Get()->WriteLogFile();

	#define OMNIFORCE_CORE_TRACE(...)			Omni::Logger::GetCoreLogger()->trace(__VA_ARGS__)
	#define OMNIFORCE_CORE_INFO(...)			Omni::Logger::GetCoreLogger()->info(__VA_ARGS__)
	#define OMNIFORCE_CORE_WARNING(...)			Omni::Logger::GetCoreLogger()->warn(__VA_ARGS__)
	#define OMNIFORCE_CORE_ERROR(...)			Omni::Logger::GetCoreLogger()->error(__VA_ARGS__)
	#define OMNIFORCE_CORE_CRITICAL(...)		Omni::Logger::GetCoreLogger()->critical(__VA_ARGS__)

	#define OMNIFORCE_CLIENT_TRACE(...)			Omni::Logger::GetClientLogger()->trace(__VA_ARGS__)
	#define OMNIFORCE_CLIENT_INFO(...)			Omni::Logger::GetClientLogger()->info(__VA_ARGS__)
	#define OMNIFORCE_CLIENT_WARNING(...)		Omni::Logger::GetClientLogger()->warn(__VA_ARGS__)
	#define OMNIFORCE_CLIENT_ERROR(...)			Omni::Logger::GetClientLogger()->error(__VA_ARGS__)
	#define OMNIFORCE_CLIENT_CRITICAL(...)		Omni::Logger::GetClientLogger()->critical(__VA_ARGS__)
#else
	#define OMNIFORCE_INITIALIZE_LOG_SYSTEM(log_level) Omni::Logger::Init(log_level);
	#define OMNIFORCE_WRITE_LOGS_TO_FILE()			   Omni::Logger::Get()->WriteLogFile();

	#define OMNIFORCE_CORE_TRACE(...)
	#define OMNIFORCE_CORE_INFO(...)
	#define OMNIFORCE_CORE_WARNING(...)
	#define OMNIFORCE_CORE_ERROR(...)
	#define OMNIFORCE_CORE_CRITICAL(...)

	#define OMNIFORCE_CLIENT_TRACE(...)
	#define OMNIFORCE_CLIENT_INFO(...)
	#define OMNIFORCE_CLIENT_WARNING(...)
	#define OMNIFORCE_CLIENT_ERROR(...)
	#define OMNIFORCE_CLIENT_CRITICAL(...)
#endif

#define OMNIFORCE_CUSTOM_LOGGER_TRACE(logger_name, ...)		Omni::Logger::GetLoggerByName(logger_name)->trace(__VA_ARGS__)
#define OMNIFORCE_CUSTOM_LOGGER_INFO(logger_name, ...)		Omni::Logger::GetLoggerByName(logger_name)->info(__VA_ARGS__)
#define OMNIFORCE_CUSTOM_LOGGER_WARN(logger_name, ...)		Omni::Logger::GetLoggerByName(logger_name)->warn(__VA_ARGS__)
#define OMNIFORCE_CUSTOM_LOGGER_ERROR(logger_name, ...)		Omni::Logger::GetLoggerByName(logger_name)->error(__VA_ARGS__)
#define OMNIFORCE_CUSTOM_LOGGER_CRITICAL(logger_name, ...)	Omni::Logger::GetLoggerByName(logger_name)->critical(__VA_ARGS__)

}