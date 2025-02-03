#include "MetaTool.h"

#include <iostream>
#include <regex>
#include <fstream>
#include <mutex>
#include <thread>

#include <spdlog/fmt/fmt.h>

namespace Omni {

	MetaTool::MetaTool()
		: m_Index()
		, m_TranslationUnits()
		, m_WorkingDir(METATOOL_WORKING_DIRECTORY)
		, m_GeneratedDataCache()
		, m_NumThreads(std::thread::hardware_concurrency())
	{}

	void MetaTool::Setup()
	{
		std::ifstream parse_targets_stream(m_WorkingDir / "ParseTargets.txt");
		std::string parse_target;

		while (std::getline(parse_targets_stream, parse_target)) {
			m_ParseTargets.push_back(parse_target);
		}

		if (m_ParseTargets.size() == 0) {
			exit(0);
		}

		std::cout << "MetaTool: running " << m_ParseTargets.size() << " action(s)" << std::endl;

		std::filesystem::create_directory(m_WorkingDir / "DummyOut");
		std::filesystem::create_directory(m_WorkingDir / "Cache");
		std::filesystem::create_directory(m_WorkingDir / "Generated");


		std::vector<const char*> args = {
			CLANG_PARSER_ARGS
		};

		std::erase_if(args, [](const char* arg) {
			return std::string(arg) == std::string("-D");
		});

		std::mutex mtx;

		std::vector<std::thread> threads(m_NumThreads);
		m_Index.resize(m_NumThreads);

		for (int thread_idx = 0; thread_idx < m_NumThreads; thread_idx++) {

			int32_t num_TU_for_thread = m_ParseTargets.size() / m_NumThreads;
			int32_t num_TU_remainder = m_ParseTargets.size() % m_NumThreads;

			int32_t start_target_index = thread_idx * num_TU_for_thread + std::min(thread_idx, num_TU_remainder);
			int32_t end_target_index = start_target_index + num_TU_for_thread + (thread_idx < num_TU_remainder ? 1 : 0);

			// Kick a job for each hardware thread
			threads[thread_idx] = std::thread([&, thread_idx, start_target_index, end_target_index]() {

				// Kill redundant threads
				if (m_ParseTargets.size() < m_NumThreads) {
					if (thread_idx >= m_ParseTargets.size()) {
						return;
					}
				}

				m_Index[thread_idx] = clang_createIndex(0, 0);

				for(int i = start_target_index; i < end_target_index; i++) {

					std::filesystem::path TU_path = m_ParseTargets[i];
					CXTranslationUnit TU = nullptr;

					CXErrorCode error_code = clang_parseTranslationUnit2(
						m_Index[thread_idx],
						TU_path.string().c_str(),
						args.data(),
						args.size(),
						nullptr,
						0,
						CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_PrecompiledPreamble | CXTranslationUnit_IncludeAttributedTypes,
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
			});
		}

		// Wait for parsing jobs
		for (int thread_idx = 0; thread_idx < m_NumThreads; thread_idx++) {
			threads[thread_idx].join();
		}

	}

	void MetaTool::TraverseAST()
	{
		for (auto& [path, TU] : m_TranslationUnits) {
			std::string target_filename = std::filesystem::path(path).filename().string();
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
					std::regex regex(R"(\s*([\w]+)\s*(?:=\s*\"([^\"]*)\")?)");

					std::sregex_iterator iterator_begin(input.begin(), input.end(), regex);
					std::sregex_iterator iterator_end;

					for (iterator_begin; iterator_begin != iterator_end; ++iterator_begin) {
						MetaEntry meta_entry = {};
						meta_entry.key = (*iterator_begin)[1].str();
						if ((*iterator_begin)[2].matched) {
							meta_entry.values.push_back((*iterator_begin)[2].str());
						}
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

				parsed_type_node.emplace("Meta", json::object());
				json& metadata_node = parsed_type_node["Meta"];

				for (auto& meta_entry : parsed_type.meta) {
					metadata_node.emplace(meta_entry.key, meta_entry.values);
				}

				parsed_type_node.emplace("Fields", json::object());
				json& fields = parsed_type_node["Fields"];

				for (auto& member : parsed_type.members) {
					fields.emplace(member.name, member.type_name);
				}
			}

			std::filesystem::path cache_path = m_WorkingDir / "Cache" / fmt::format("{}.cache", target_filename);
			bool target_is_cached = std::filesystem::exists(cache_path);
			if (!target_is_cached) {
				m_RunStatistics.targets_generated++;
				continue;
			}

			std::ifstream cache_stream(cache_path);
			json target_cache;
			target_cache << cache_stream;

			if (target_cache == m_GeneratedDataCache[path]) {
				m_ParsingResult.erase(path);
				m_GeneratedDataCache.erase(path);
				m_RunStatistics.targets_skipped++;
				continue;
			}
			else {
				m_RunStatistics.targets_generated++;
			}

		}
	}

	void MetaTool::GenerateCode()
	{
		for (auto& [TU_path, parsing_result] : m_ParsingResult) {
			for (auto& parsed_type : parsing_result.types) {
				// Return if no "ShaderExpose" annotation - no code generation needed
				auto shader_expose_meta_iterator = std::find_if(parsed_type.meta.begin(), parsed_type.meta.end(), 
					[](const MetaEntry& entry) {
						return entry.key == "ShaderExpose";
					}
				);

				if (shader_expose_meta_iterator == parsed_type.meta.end()) {
					continue;
				}

				// Try to acquire shader module name for exposed struct

				auto shader_module_meta_iterator = std::find_if(parsed_type.meta.begin(), parsed_type.meta.end(),
					[](const MetaEntry& entry) {
						return entry.key == "Module";
					}
				);

				if (shader_module_meta_iterator == parsed_type.meta.end()) {
					std::cerr << "MetaTool: failed to run parsing action on shader exposed \"" 
						<< parsed_type.type_name << "\" type - the type is exposed but no shader module was specified" << std::endl;
					exit(-3);
				}

				std::string type_module_name = (*shader_module_meta_iterator).values[0];

				std::ofstream stream(m_WorkingDir / "Generated" / std::filesystem::path(parsed_type.type_name + ".slang"));

				// Generate prologue
				{
					stream << "#pragma once" << std::endl;
					PrintEmptyLine(stream);
					stream << fmt::format("module {};", type_module_name) << std::endl;
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
		std::cout << fmt::format("MetaTool: {} target(s) generated, {} target(s) skipped", m_RunStatistics.targets_generated, m_RunStatistics.targets_skipped) << std::endl;
	}

	void MetaTool::CleanUp()
	{
		for (auto& [TU_path, TU] : m_TranslationUnits) {
			clang_disposeTranslationUnit(TU);
		}
		
		for (uint32_t i = 0; i < m_NumThreads; i++) {
			clang_disposeIndex(m_Index[i]);
		}
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
		else if (source_type == "glm::hvec2") {
			return "half2";
		}
		else if (source_type == "glm::hvec3") {
			return "half3";
		}
		else if (source_type == "glm::hvec4") {
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