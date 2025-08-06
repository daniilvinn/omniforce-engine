#pragma once

#include "ASTVisitor.h"
#include "CacheManager.h"
#include <clang-c/Index.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

namespace Omni {

	class Parser {
	public:
		Parser(const std::filesystem::path& workingDir, uint32_t numThreads, CacheManager* cacheManager);
		~Parser();

		// Main parsing function
		std::unordered_map<std::string, nlohmann::ordered_json> ParseTargets(const std::vector<std::filesystem::path>& parseTargets);

		// Statistics
		uint32_t GetTargetsGenerated() const { return m_TargetsGenerated; }
		uint32_t GetTargetsSkipped() const { return m_TargetsSkipped; }

	private:
		// Thread worker function
		void ParseTargetsWorker(uint32_t threadIdx, uint32_t startTargetIndex, uint32_t endTargetIndex, 
							   const std::vector<std::filesystem::path>& parseTargets);

		// Translation unit parsing
		CXTranslationUnit ParseTranslationUnit(const std::filesystem::path& tuPath, uint32_t threadIdx);
		void ValidateTranslationUnit(CXTranslationUnit translationUnit);

		// Setup
		void SetupParserArgs();
		void SetupIndexes();

	private:
		std::filesystem::path m_WorkingDir;
		uint32_t m_NumThreads;
		
		std::vector<const char*> m_ParserArgs;
		std::vector<CXIndex> m_Indexes;
		
		std::unordered_map<std::string, nlohmann::ordered_json> m_GeneratedDataCache;
		std::mutex m_CacheMutex;
		
		std::atomic_uint32_t m_TargetsGenerated;
		std::atomic_uint32_t m_TargetsSkipped;
		
		CacheManager* m_CacheManager;
	};

} 