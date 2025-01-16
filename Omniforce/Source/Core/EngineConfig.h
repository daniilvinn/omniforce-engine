#pragma once

#include <Foundation/Common.h>

#include <Filesystem/Filesystem.h>

namespace Omni {
	using nlohmann::json;

	class OMNIFORCE_API EngineConfig {
	public:
		static bool Load() {
			OMNIFORCE_ASSERT_TAGGED(m_Config == nullptr, "Config is already loaded");
			OMNIFORCE_CORE_INFO("Entering config init stage");

			m_Config = new json;

			try {
				if (!FileSystem::CheckDirectory("config.json")) {
					throw std::exception("Failed to locate `config.json`");
				}

				std::ifstream config_filestream("config.json");

				*m_Config = json::parse(config_filestream);
			}
			catch (const std::exception& e) {
				OMNIFORCE_CORE_CRITICAL("{}", e.what());
				OMNIFORCE_CORE_CRITICAL("Failed to load engine config");
				return false;
			}

			OMNIFORCE_CORE_INFO("Loaded engine config");
			return true;
		}

		static bool Save() {

		}

		template<typename T>
		static T Get(std::string_view key) {

		}

		template<typename T>
		static void Set(std::string_view key, const T& value) {

		}

	private:
		inline static json* m_Config = nullptr;
	};

}