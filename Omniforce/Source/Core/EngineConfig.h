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
			OMNIFORCE_CORE_WARNING("Using built-in `new` operator instead of persistent allocator");

			try {
				if (!FileSystem::CheckDirectory("Config.json")) {
					throw std::exception("Failed to locate `Config.json`");
				}

				std::ifstream in_stream("Config.json");

				*m_Config = json::parse(in_stream);
			}
			catch (const std::exception& e) {
				OMNIFORCE_CORE_CRITICAL("{}", e.what());
				OMNIFORCE_CORE_CRITICAL("Failed to load engine config");
				return false;
			}

			OMNIFORCE_CORE_INFO("Loaded engine config");
			return true;
		}

		static void Save() {
			std::ofstream out_stream("Config.json");

			if (!out_stream.is_open()) {
				OMNIFORCE_CORE_ERROR("Failed to open a file stream for engine config");
			}

			try {
				out_stream << m_Config->dump(4);
			}
			catch (std::exception* e) {
				OMNIFORCE_CORE_ERROR("Failed to save engine config; error: {}", e->what());
			}

			OMNIFORCE_CORE_INFO("Saved engine config");
		}

		template<typename T>
		static T Get(std::string_view key) {
			OMNIFORCE_ASSERT_TAGGED(key.size(), "Invalid config entry key");
			Array<std::string_view> blocks(&g_PersistentAllocator);

			uint64 start = 0;
			uint64 end = key.find('.');

			while (end != std::string::npos) {
				blocks.Add(key.substr(start, end - start));
				start = end + 1;
				end = key.find('.', start);
			}
			blocks.Add(key.substr(start));

			T value = T();

			// Start traversal from the root
			json* node = m_Config;

			uint32 i = 0;

			for (const auto& subkey : blocks) {
				// If not at the last node
				if (i != blocks.Size() - 1) {
					// Sanity check
					OMNIFORCE_ASSERT_TAGGED(node->contains(subkey), "Tried to get a config value with non-existent key ");
				}
				node = &node->at(std::string(subkey));
				i++;
			}

			return node->get<T>();
		}

		template<typename T>
		static void Set(std::string_view key, const T& value) {
			OMNIFORCE_ASSERT_TAGGED(key.size(), "Invalid config entry key");
			Array<std::string_view> blocks(&g_PersistentAllocator);

			uint64 start = 0;
			uint64 end = key.find('.');

			while (end != std::string::npos) {
				blocks.Add(key.substr(start, end - start));
				start = end + 1;
				end = key.find('.', start);
			}
			blocks.Add(key.substr(start));

			// Start traversal from the root
			json* node = m_Config;

			uint32 i = 0;
			for (const auto& subkey : blocks) {
				// If not at the last node
				if (i != blocks.Size() - 1) {
					// Sanity check
					OMNIFORCE_ASSERT_TAGGED(node->contains(subkey), "Tried to get a config value with non-existent key ");
				}

				node = &node->at(std::string(subkey));
				i++;
			}

			*node = value;
		}

	private:
		inline static json* m_Config = nullptr;
	};

	template<typename T>
	class OMNIFORCE_API EngineConfigValue {
	public:
		EngineConfigValue(const std::string& name, const std::string& description)
			: m_Name(name)
			, m_Description(description)
		{
			OMNIFORCE_ASSERT_TAGGED(m_Name.size(), "Invalid config entry key");
			OMNIFORCE_ASSERT_TAGGED(m_Description.size(), "Invalid config entry description");
		}

		T Get() const {
			return EngineConfig::Get<T>(m_Name);
		}

		void Set(const T& value) {
			EngineConfig::Set(m_Name, value);
		}

		std::string_view GetName() const {
			return m_Name;
		}

		std::string_view GetDescription() const {
			return m_Description;
		}

	private:
		std::string m_Name;
		std::string m_Description;
	};

}