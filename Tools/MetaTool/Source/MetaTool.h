#pragma once

#include "OptionParser.h"

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

		void DumpResults();

		void CleanUp();

	private:
		void Validate(CXTranslationUnit translation_unit);

		void PrintEmptyLine(std::ofstream& stream);

		std::string GetShaderType(const std::string& source_type) const;

	private:
		using StringPath = std::string;
		using CacheType = nlohmann::ordered_json;

		std::vector<std::filesystem::path> m_ParseTargets;
		std::filesystem::path m_WorkingDir;
		std::filesystem::path m_OutputDir;
		uint32_t m_NumThreads;

		std::unordered_map<StringPath, CacheType> m_GeneratedDataCache;
		std::vector<std::string> m_PendingModuleReassemblies;
		std::unordered_map<std::string, CacheType> m_ModuleCaches;

		std::vector<const char*> m_ParserArgs;
		std::vector<CXIndex> m_Index;
		std::unordered_map<StringPath, CXTranslationUnit> m_TranslationUnits;

		struct RunStatistics {
			std::atomic_uint32_t targets_generated = 0;
			std::atomic_uint32_t targets_skipped = 0;
		} m_RunStatistics;

		std::mutex m_Mutex;

	};

}