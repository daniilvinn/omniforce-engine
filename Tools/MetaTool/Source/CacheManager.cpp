#include "CacheManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <spdlog/fmt/fmt.h>

namespace Omni {

	CacheManager::CacheManager(const std::filesystem::path& workingDir)
		: m_WorkingDir(workingDir)
		, m_ModuleCaches()
	{
		CreateCacheDirectories();
	}

	bool CacheManager::IsTargetCached(const std::string& targetFilename) {
		std::filesystem::path cachePath = m_WorkingDir / "Cache" / fmt::format("{}.cache", targetFilename);
		return std::filesystem::exists(cachePath);
	}

	bool CacheManager::ShouldRegenerateTarget(const std::string& tuPath, const nlohmann::ordered_json& currentData) {
		std::string targetFilename = std::filesystem::path(tuPath).filename().string();
		
		if (!IsTargetCached(targetFilename)) {
			return true;
		}

		nlohmann::ordered_json targetCache = LoadTargetCache(targetFilename);
		return targetCache != currentData;
	}

	std::vector<std::string> CacheManager::GetPendingModuleReassemblies(const std::unordered_map<std::string, nlohmann::ordered_json>& generatedDataCache) {
		std::vector<std::string> pendingModuleReassemblies;

		for (const auto& [tuPath, parsingResult] : generatedDataCache) {
			for (const auto& parsedTypeIter : parsingResult.items()) {
				std::string typeName = parsedTypeIter.key();
				const nlohmann::ordered_json& parsedType = parsedTypeIter.value();

				// Check if it's a shader-exposed type
				if (!parsedType["Meta"].contains("ShaderExpose")) {
					continue;
				}

				// Get module name
				std::vector<std::string> moduleEntryValues;
				parsedType["Meta"]["Module"].get_to(moduleEntryValues);
				std::string typeModuleName = moduleEntryValues[0];

				// Check if module needs reassembly
				if (ModuleNeedsReassembly(typeModuleName, typeName)) {
					bool modulePending = std::find(pendingModuleReassemblies.begin(), pendingModuleReassemblies.end(), typeModuleName) != pendingModuleReassemblies.end();
					if (!modulePending) {
						pendingModuleReassemblies.emplace_back(typeModuleName);
					}
				}
			}
		}

		return pendingModuleReassemblies;
	}

	void CacheManager::DumpTargetCache(const std::string& tuPath, const nlohmann::ordered_json& data) {
		std::filesystem::path targetFile(std::filesystem::path(tuPath).filename());
		std::ofstream stream(m_WorkingDir / "Cache" / (targetFile.string() + ".cache"));
		stream << data.dump(4);
		stream.close();
	}

	void CacheManager::DumpTypeModuleCaches(const std::unordered_map<std::string, nlohmann::ordered_json>& generatedDataCache) {
		for (const auto& [tuPath, parsingResult] : generatedDataCache) {
			for (const auto& typeNode : parsingResult.items()) {
				const nlohmann::ordered_json& typeCacheNode = typeNode.value();

				// Skip type module cache generation if it is not MetaTool-annotated type
				if (typeCacheNode.contains("Meta")) {
					if (!typeCacheNode["Meta"]["MetaToolAnnotated"].get<bool>()) {
						continue;
					}
				}
				else {
					continue;
				}

				std::string typeModuleName = typeCacheNode["Meta"]["Module"].get<std::array<std::string, 1>>()[0];

				nlohmann::ordered_json typeModuleCache = nlohmann::ordered_json::object();
				typeModuleCache.emplace("Module", typeModuleName);

				std::ofstream typeModuleCacheStream(m_WorkingDir / "Cache" / "TypeModules" / std::filesystem::path(typeNode.key() + ".cache"));
				typeModuleCacheStream << typeModuleCache.dump(4);
				typeModuleCacheStream.close();
			}
		}
	}

	void CacheManager::CleanupDeletedTypes(const std::unordered_map<std::string, nlohmann::ordered_json>& generatedDataCache) {
		// This would implement cleanup logic for deleted types
		// For now, it's a placeholder for future implementation
	}

	nlohmann::ordered_json CacheManager::LoadTargetCache(const std::string& targetFilename) {
		std::filesystem::path cachePath = m_WorkingDir / "Cache" / fmt::format("{}.cache", targetFilename);
		
		if (!std::filesystem::exists(cachePath)) {
			return nlohmann::ordered_json::object();
		}

		std::ifstream cacheStream(cachePath);
		if (!cacheStream.is_open()) {
			return nlohmann::ordered_json::object();
		}

		return nlohmann::ordered_json::parse(cacheStream);
	}

	bool CacheManager::ModuleNeedsReassembly(const std::string& moduleName, const std::string& typeName) {
		// If cache does not exist, we inevitably need to assemble the module
		std::filesystem::path moduleCachePath = m_WorkingDir / "Cache" / "Modules" / fmt::format("{}.cache", moduleName);
		bool moduleCacheExists = std::filesystem::exists(moduleCachePath);

		if (!moduleCacheExists) {
			return true;
		}

		// Load module cache if needed
		if (!m_ModuleCaches.contains(moduleName)) {
			std::ifstream moduleCacheStream(moduleCachePath);
			if (moduleCacheStream.is_open()) {
				m_ModuleCaches[moduleName] = nlohmann::ordered_json::parse(moduleCacheStream);
			}
		}

		// Check if module reassembly is needed
		return !m_ModuleCaches[moduleName].contains(typeName);
	}

	void CacheManager::CreateCacheDirectories() {
		std::filesystem::create_directories(m_WorkingDir / "Cache" / "Modules");
		std::filesystem::create_directories(m_WorkingDir / "Cache" / "TypeModules");
	}

} 