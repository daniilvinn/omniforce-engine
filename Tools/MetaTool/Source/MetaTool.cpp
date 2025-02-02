#include "MetaTool.h"

#include <iostream>
#include <regex>
#include <fstream>
#include <mutex>

#include <spdlog/fmt/fmt.h>

namespace Omni {

	MetaTool::MetaTool()
		: m_Index(nullptr)
		, m_TranslationUnits()
		, m_WorkingDir(METATOOL_WORKING_DIRECTORY)
		, m_GeneratedDataCache()
	{
	}

	void MetaTool::Setup()
	{
		std::ifstream parse_targets_stream(m_WorkingDir / "ParseTargets.txt");
		std::string parse_target;

		while (std::getline(parse_targets_stream, parse_target)) {
			m_ParseTargets.push_back(parse_target);
		}

		if (m_ParseTargets.size() == 0) {
			std::cout << "[WARNING] MetaTool: attempted to launch 0 MetaTool actions" << std::endl;
			exit(0);
		}
		else {
			std::cout << "Running " << m_ParseTargets.size() << " MetaTool action(s)" << std::endl;
		}

		std::filesystem::create_directory(m_WorkingDir / "DummyOut");
		std::filesystem::create_directory(m_WorkingDir / "Cache");
		std::filesystem::create_directory(m_WorkingDir / "Generated");

		m_Index = clang_createIndex(0, 0);

		std::vector<const char*> args = {
			CLANG_PARSER_ARGS
		};

		std::erase_if(args, [](const char* arg) {
			return std::string(arg) == std::string("-D");
		});

		std::mutex mtx;

		#pragma omp parallel for
		for(int i = 0; i < m_ParseTargets.size(); i++) {
			std::filesystem::path TU_path = m_ParseTargets[i];
			CXTranslationUnit TU = nullptr;

			CXErrorCode error_code = clang_parseTranslationUnit2(
				m_Index,
				TU_path.string().c_str(),
				args.data(),
				args.size(),
				nullptr,
				0,
				CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_PrecompiledPreamble | CXTranslationUnit_Incomplete | CXTranslationUnit_IncludeAttributedTypes,
				&TU
			);

			if (error_code != CXError_Success) {
				std::cerr << "Failed to run MetaTool; error code: " << error_code << std::endl;
				exit(-2);
			}

			Validate(TU);

			mtx.lock();
			m_TranslationUnits.emplace(TU_path.string(), TU);
			mtx.unlock();
		}

	}

	void MetaTool::TraverseAST()
	{
		for (auto& [path, TU] : m_TranslationUnits) {
			CXCursor root_cursor = clang_getTranslationUnitCursor(TU);

			ParsingResult TU_parsing_result = {};

			clang_visitChildren(root_cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) {
				CXSourceLocation current_cursor_location = clang_getCursorLocation(cursor);
				CXSourceLocation parent_cursor_location = clang_getCursorLocation(parent);

				if (!clang_Location_isFromMainFile(current_cursor_location) && !clang_Location_isFromMainFile(parent_cursor_location)) {
					return CXChildVisit_Continue;
				}

				ParsingResult* parsing_result = (ParsingResult*)client_data;

				if (clang_isAttribute(clang_getCursorKind(cursor))) {
					ParsedType parsed_type = {};

					parsed_type.type_name = clang_getCString(clang_getCursorSpelling(parent));

					CXType type = clang_getCursorType(parent);

					clang_Type_visitFields(type, [](CXCursor field_cursor, CXClientData field_client_data) {
						ParsedType* field_output = (ParsedType*)field_client_data;

						field_output->members.push_back({
							clang_getCString(clang_getTypeSpelling(clang_getCursorType(field_cursor))),
							clang_getCString(clang_getCursorSpelling(field_cursor))
						});

						return CXVisit_Continue;
					}, &parsed_type);

					std::string input = clang_getCString(clang_getCursorSpelling(cursor));

					std::regex regular_expression("\\s*,\\s*|\\s+");

					std::sregex_token_iterator iterator_begin(input.begin(), input.end(), regular_expression, -1);
					std::sregex_token_iterator iterator_end;

					for (iterator_begin; iterator_begin != iterator_end; ++iterator_begin) {
						MetaEntry meta_entry = {};
						meta_entry.key = *iterator_begin;
						parsed_type.meta.push_back(meta_entry);
					}

					parsing_result->types.push_back(parsed_type);
				}

				return CXChildVisit_Recurse;
			}, (void*)&TU_parsing_result);

			m_ParsingResult.emplace(path, TU_parsing_result);

			for (auto& parsed_type : m_ParsingResult[path].types) {
				m_GeneratedDataCache.emplace(path, json::object());

				m_GeneratedDataCache[path].emplace(parsed_type.type_name, json::object());
				json& parsed_type_node = m_GeneratedDataCache[path][parsed_type.type_name];
				parsed_type_node.emplace("Meta", parsed_type.meta);
				parsed_type_node.emplace("Types", json::object());

				json& fields = parsed_type_node["Types"];

				for (auto& member : parsed_type.members) {
					fields.emplace(member.name, member.type_name);
				}
			}
		}
	}

	void MetaTool::GenerateCode()
	{
		for (auto& [TU_path, parsing_result] : m_ParsingResult) {
			for (auto& parsed_type : parsing_result.types) {
				// Return if no "ShaderExpose" annotation - no code generation needed
				if (std::find(parsed_type.meta.begin(), parsed_type.meta.end(), "ShaderExpose") == parsed_type.meta.end()) {
					continue;
				}

				std::ofstream stream(m_WorkingDir / "Generated" / std::filesystem::path(parsed_type.type_name + ".slang"));

				// Generate prologue
				{
					stream << "#pragma once" << std::endl;
					PrintEmptyLine(stream);
					stream << fmt::format("module {};", parsed_type.type_name) << std::endl;
					PrintEmptyLine(stream);
					stream << "namespace Omni {" << std::endl;
				}
				// Generate contents
				{
					PrintEmptyLine(stream);
					stream << fmt::format("public struct {} {{", parsed_type.type_name) << std::endl;
					for (auto& member : parsed_type.members) {
						stream << fmt::format("\t{} {};", GetShaderType(member.type_name), member.name) << std::endl;
					}
					stream << "};" << std::endl;
					PrintEmptyLine(stream);
				}
				// Generate epilogue
				{
					stream << "}" << std::endl;
				}
			}
		}
	}

	void MetaTool::DumpResults()
	{
		for (auto& parse_target : m_ParseTargets) {
			std::filesystem::path target_file(parse_target.filename());

			std::ofstream stream(m_WorkingDir / "DummyOut" / (target_file.string() + ".gen"));
			stream.close();

			stream.open(m_WorkingDir / "Cache" / (target_file.string() + ".cache"));
			stream << m_GeneratedDataCache[parse_target.string()].dump(4);
			stream.close();
		}
	}

	void MetaTool::CleanUp()
	{
		for (auto& [TU_path, TU] : m_TranslationUnits) {
			clang_disposeTranslationUnit(TU);
		}
		clang_disposeIndex(m_Index);
	}

	void MetaTool::Validate(CXTranslationUnit translation_unit)
	{
		int num_diagnostic_messages = clang_getNumDiagnostics(translation_unit);

		if (num_diagnostic_messages)
			std::cout << "There are " << num_diagnostic_messages << "MetaTool diagnostic messages" << std::endl;

		bool found_error = false;

		for (uint32_t current_message = 0; current_message < num_diagnostic_messages; ++current_message) {
			CXDiagnostic diagnotic = clang_getDiagnostic(translation_unit, current_message);
			CXString error_string = clang_formatDiagnostic(diagnotic, clang_defaultDiagnosticDisplayOptions());

			std::string tmp(clang_getCString(error_string));

			clang_disposeString(error_string);

			if (tmp.find("error:") != std::string::npos) {
				found_error = true;
			}

			std::cerr << tmp << std::endl;
		}

		if (found_error) {
			std::cerr << "Failed to run MetaTool" << std::endl;
			exit(-1);
		}
	}

	void MetaTool::PrintEmptyLine(std::ofstream& stream)
	{
		stream << std::endl;
	}

	constexpr std::string MetaTool::GetShaderType(const std::string& source_type) const
	{
		// Vector types
		if (source_type == "glm::vec2") {
			return "float2";
		}
		else if (source_type == "glm::vec3") {
			return "float3";
		}
		else if (source_type == "glm::vec4") {
			return "float4";
		}
		else if (source_type == "glm::uvec2") {
			return "uint2";
		}
		else if (source_type == "glm::uvec3") {
			return "uint3";
		}
		else if (source_type == "glm::uvec4") {
			return "uint4";
		}
		else if (source_type == "glm::ivec2") {
			return "int2";
		}
		else if (source_type == "glm::ivec3") {
			return "int3";
		}
		else if (source_type == "glm::ivec4") {
			return "int4";
		}
		else if (source_type == "glm::half2") {
			return "half2";
		}
		else if (source_type == "glm::half3") {
			return "half3";
		}
		else if (source_type == "glm::half4") {
			return "half4";
		}
		// Scalar types
		else if (source_type == "uint64") {
			return "uint64_t";
		}
		else if (source_type == "float64") {
			return "double";
		}
		else if (source_type == "uint32") {
			return "uint";
		}
		else if (source_type == "float32") {
			return "float";
		}
		else if (source_type == "uint16") {
			return "uint16_t";
		}
		else if (source_type == "glm::half") {
			return "half";
		}
		else if (source_type == "uint8") {
			return "uint8_t";
		}
		else if (source_type == "byte") {
			return "uint8_t";
		}
		else if (source_type == "bool") {
			return "bool";
		}
		else {
			return source_type;
		}

	}

}