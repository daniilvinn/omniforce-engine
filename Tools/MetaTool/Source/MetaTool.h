#pragma once

#include "OptionParser.h"

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

		void DumpResults();

		void CleanUp();

	private:

		void Validate(CXTranslationUnit translation_unit);

		void PrintEmptyLine(std::ofstream& stream);

		constexpr std::string GetShaderType(const std::string& source_type) const;

	private:

		struct ParsedField {
			std::string type_name;
			std::string name;
		};

		struct MetaEntry {
			std::string key;
			std::vector<std::string> values;
		};

		struct ParsedType {
			ParsedType() {}

			std::string type_name;
			std::vector<MetaEntry> meta;
			std::vector<ParsedField> members;
		};

		struct ParsingResult {
			ParsingResult() {}

			std::vector<ParsedType> types;
		};

	private:
		std::vector<std::filesystem::path> m_ParseTargets;
		std::filesystem::path m_WorkingDir;
		
		std::unordered_map<std::string, json> m_GeneratedDataCache;

		CXIndex m_Index;
		std::unordered_map<std::string, CXTranslationUnit> m_TranslationUnits;

		// TU to ParsingResult map
		std::unordered_map<std::string, ParsingResult> m_ParsingResult;

		struct RunStatistics {
			uint32_t targets_generated = 0;
			uint32_t targets_skipped = 0;
		} m_RunStatistics;

	};

}