#pragma once

#include "OptionParser.h"
#include "Timer.h"
#include "Parser.h"
#include "CodeGenerator.h"
#include "CacheManager.h"

#include <mutex>
#include <atomic>

#include <clang-c/Index.h>
#include <nlohmann/json.hpp>

using namespace nlohmann;

namespace Omni {

	class MetaTool {
	public:
		MetaTool();
		
		void Setup();
		void TraverseAST();
		void GenerateCode();
		void AssembleModules();
		void DumpCache();
		void CleanUp();
		void PrintStatistics();

	private:
		void LoadParseTargets();
		void CreateDirectories();

	private:
		std::vector<std::filesystem::path> m_ParseTargets;
		std::filesystem::path m_WorkingDir;
		std::filesystem::path m_OutputDir;
		uint32_t m_NumThreads;

		std::unique_ptr<CacheManager> m_CacheManager;
		std::unique_ptr<Parser> m_Parser;
		std::unique_ptr<CodeGenerator> m_CodeGenerator;

		std::unordered_map<std::string, nlohmann::ordered_json> m_GeneratedDataCache;
		std::vector<std::string> m_PendingModuleReassemblies;

		struct RunStatistics {
			Timer session_timer;
			std::atomic_uint32_t targets_generated = 0;
			std::atomic_uint32_t targets_skipped = 0;
		} m_RunStatistics;

	};

}