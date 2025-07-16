#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Omni {

	class CacheManager {
	public:
		CacheManager(const std::filesystem::path& workingDir);
		~CacheManager() = default;

		// Cache validation and comparison
		bool IsTargetCached(const std::string& targetFilename);
		bool ShouldRegenerateTarget(const std::string& tuPath, const nlohmann::ordered_json& currentData);
		std::vector<std::string> GetPendingModuleReassemblies(const std::unordered_map<std::string, nlohmann::ordered_json>& generatedDataCache);

		// Cache dumping
		void DumpTargetCache(const std::string& tuPath, const nlohmann::ordered_json& data);
		void DumpTypeModuleCaches(const std::unordered_map<std::string, nlohmann::ordered_json>& generatedDataCache);

		// Cache cleanup
		void CleanupDeletedTypes(const std::unordered_map<std::string, nlohmann::ordered_json>& generatedDataCache);

	private:
		// Helper functions
		nlohmann::ordered_json LoadTargetCache(const std::string& targetFilename);
		bool ModuleNeedsReassembly(const std::string& moduleName, const std::string& typeName);
		void CreateCacheDirectories();

	private:
		std::filesystem::path m_WorkingDir;
		std::unordered_map<std::string, nlohmann::ordered_json> m_ModuleCaches;
	};

} 