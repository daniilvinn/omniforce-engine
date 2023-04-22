#pragma once

#include <iostream>

#include <Foundation/Macros.h>
#include <Memory/Pointers.hpp>

#include <spdlog/spdlog.h>

namespace Omni {

	class OMNIFORCE_API Logger 
	{
	public:
		enum class Level : uint8_t 
		{
			LEVEL_TRACE = 0,
			LEVEL_INFO,
			LEVEL_WARN,
			LEVEL_ERROR,
			LEVEL_CRITICAL,
			LEVEL_NONE
		};

		static void Init(Level level);
		static void Shutdown();
		static Logger* Get() { return s_Instance; }
		static Shared<spdlog::logger> GetCoreLooger() { return s_Instance->m_CoreLogger; };
		static Shared<spdlog::logger> GetClientLooger() { return s_Instance->m_ClientLogger; };

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
		void WriteLogFile();

	
		Logger(Level level);
		~Logger();

	
		static Logger* s_Instance;
		Shared<spdlog::logger> m_CoreLogger;
		Shared<spdlog::logger> m_ClientLogger;
		Shared<spdlog::logger> m_FileLogger;
		std::string m_LogBuffer;
	};

#ifdef OMNIFORCE_DEBUG
	#define OMNIFORCE_INITIALIZE_LOG_SYSTEM(log_level) Omni::Logger::Init(log_level);
	#define OMNIFORCE_WRITE_LOGS_TO_FILE() Omni::Logger::Get()->WriteLogFile();

	#define OMNIFORCE_CORE_TRACE(...)			Omni::Logger::GetCoreLooger()->trace(__VA_ARGS__)
	#define OMNIFORCE_CORE_INFO(...)			Omni::Logger::GetCoreLooger()->info(__VA_ARGS__)
	#define OMNIFORCE_CORE_WARNING(...)			Omni::Logger::GetCoreLooger()->warn(__VA_ARGS__)
	#define OMNIFORCE_CORE_ERROR(...)			Omni::Logger::GetCoreLooger()->error(__VA_ARGS__)
	#define OMNIFORCE_CORE_CRITICAL(...)		Omni::Logger::GetCoreLooger()->critical(__VA_ARGS__)

	#define OMNIFORCE_CLIENT_TRACE(...)			Omni::Logger::GetClientLooger()->trace(__VA_ARGS__)
	#define OMNIFORCE_CLIENT_INFO(...)			Omni::Logger::GetClientLooger()->info(__VA_ARGS__)
	#define OMNIFORCE_CLIENT_WARNING(...)		Omni::Logger::GetClientLooger()->warn(__VA_ARGS__)
	#define OMNIFORCE_CLIENT_ERROR(...)			Omni::Logger::GetClientLooger()->error(__VA_ARGS__)
	#define OMNIFORCE_CLIENT_CRITICAL(...)		Omni::Logger::GetClientLooger()->critical(__VA_ARGS__)
#else
	#define OMNIFORCE_INITIALIZE_LOG_SYSTEM(log_level)
	#define OMNIFORCE_WRITE_LOGS_TO_FILE()

	#define OMNIFORCE_CORE_TRACE(...)
	#define OMNIFORCE_CORE_INFO(...)
	#define OMNIFORCE_CORE_WARNING(...)
	#define OMNIFORCE_CORE_ERROR(...)
	#define OMNIFORCE_CORE_CRITICAL(...)

	#define CURSED_CLIENT_TRACE(...)
	#define CURSED_CLIENT_INFO(...)
	#define CURSED_CLIENT_WARNING(...)
	#define CURSED_CLIENT_ERROR(...)
	#define CURSED_CLIENT_CRITICAL(...)
#endif

}