#include "MetaTool.h"

#include <iostream>
#include <regex>
#include <fstream>
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
		// Load parse targets
		std::ifstream parse_targets_stream(m_WorkingDir / "ParseTargets.txt");
		std::string parse_target;

		while (std::getline(parse_targets_stream, parse_target)) {
			m_ParseTargets.push_back(parse_target);
		}

		// Exit if no parse targets (maybe throw a warning in this case?)
		if (m_ParseTargets.size() == 0) {
			exit(0);
		}

		std::cout << "MetaTool: running " << m_ParseTargets.size() << " action(s)" << std::endl;

		std::filesystem::create_directory(m_WorkingDir / "DummyOut");
		std::filesystem::create_directory(m_WorkingDir / "Cache");
		std::filesystem::create_directory(m_WorkingDir / "Generated");

		// Setup parser args
		m_ParserArgs = {
			CLANG_PARSER_ARGS
		};

		// Erase empty compiler definitions
		std::erase_if(m_ParserArgs, [](const char* arg) {
			return std::string(arg) == std::string("-D");
		});

		// Prepare index storage
		m_Index.resize(m_NumThreads);

		for (auto& index : m_Index) {
			index = clang_createIndex(0, 0);
		}
	}

	void MetaTool::TraverseAST()
	{
		std::vector<std::thread> threads(m_NumThreads);

		for (uint32_t thread_idx = 0; thread_idx < m_NumThreads; thread_idx++) {

			// Setup task parameters
			uint32_t num_TU_for_thread = m_ParseTargets.size() / m_NumThreads;
			uint32_t num_TU_remainder = m_ParseTargets.size() % m_NumThreads;

			uint32_t start_target_index = thread_idx * num_TU_for_thread + std::min(thread_idx, num_TU_remainder);
			uint32_t end_target_index = start_target_index + num_TU_for_thread + (thread_idx < num_TU_remainder ? 1 : 0);

			// Kick a job for each hardware thread
			threads[thread_idx] = std::thread([&, thread_idx, start_target_index, end_target_index]() {

				// Kill redundant threads
				if (m_ParseTargets.size() < m_NumThreads) {
					if (thread_idx >= m_ParseTargets.size()) {
						return;
					}
				}

				for (uint32_t i = start_target_index; i < end_target_index; i++) {

					std::filesystem::path TU_path = m_ParseTargets[i];
					std::string target_filename = std::filesystem::path(TU_path).filename().string();
					CXTranslationUnit TU = nullptr;
					// ======================
					// Parse translation unit
					// ======================
					{
						CXErrorCode error_code = clang_parseTranslationUnit2(
							m_Index[thread_idx],
							TU_path.string().c_str(),
							m_ParserArgs.data(),
							m_ParserArgs.size(),
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
					}
					// ============
					// Traverse AST
					// ============
					{
						CXCursor root_cursor = clang_getTranslationUnitCursor(TU);

						json TU_parsing_result = json::object();

						clang_visitChildren(root_cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) {
							// Check if we are in Omniforce source file
							CXSourceLocation current_cursor_location = clang_getCursorLocation(cursor);
							CXSourceLocation parent_cursor_location = clang_getCursorLocation(parent);

							if (!clang_Location_isFromMainFile(current_cursor_location) && !clang_Location_isFromMainFile(parent_cursor_location)) {
								return CXChildVisit_Continue;
							}

							// Start traversal
							json& parsing_result = *(json*)client_data;

							// Check if we are on attribute
							if (clang_isAttribute(clang_getCursorKind(cursor))) {
								const char* type_name = clang_getCString(clang_getCursorSpelling(parent));

								parsing_result.emplace(type_name, json::object());
								json& parsed_type = parsing_result.at(type_name);

								// If so, acquire parent type (that is a struct or a class)
								CXType type = clang_getCursorType(parent);

								// Traverse its fields to generate code
								clang_Type_visitFields(type, [](CXCursor field_cursor, CXClientData field_client_data) {
									json& field_output = *(json*)field_client_data;
									field_output.emplace("Fields", json::object());

									field_output["Fields"].emplace(
										clang_getCString(clang_getCursorSpelling(field_cursor)),
										clang_getCString(clang_getTypeSpelling(clang_getCursorType(field_cursor)))
									);
									return CXVisit_Continue;
								}, &parsed_type);

								// Parse attributes
								std::string input = clang_getCString(clang_getCursorSpelling(cursor));
								std::regex regex(R"(\s*([\w]+)\s*(?:=\s*\"([^\"]*)\")?)");

								std::sregex_iterator iterator_begin(input.begin(), input.end(), regex);
								std::sregex_iterator iterator_end;

								// Parse metadata
								parsed_type.emplace("Meta", json::object());
								for (iterator_begin; iterator_begin != iterator_end; ++iterator_begin) {
									std::string meta_entry_name = (*iterator_begin)[1].str();
									parsed_type["Meta"].emplace(meta_entry_name, json::array());

									std::vector<std::string> meta_entry_values;
									
									if ((*iterator_begin)[2].matched) {
										parsed_type["Meta"][meta_entry_name].emplace_back((*iterator_begin)[2].str());
									}
								}
							}

							// Keep traversal going
							return CXChildVisit_Recurse;
						}, &TU_parsing_result);

						// Write traversal results
						m_Mutex.lock();
						{
							m_GeneratedDataCache[TU_path.string()] = TU_parsing_result;
							clang_disposeTranslationUnit(TU);
						}
						m_Mutex.unlock();
					}
					// Validate result, skip code generation for this target if matched with cache
					{
						std::filesystem::path cache_path = m_WorkingDir / "Cache" / fmt::format("{}.cache", target_filename);
						bool target_is_cached = std::filesystem::exists(cache_path);
						if (!target_is_cached) {
							m_RunStatistics.targets_generated++;
							continue;
						}

						std::ifstream cache_stream(cache_path);
						json target_cache;
						target_cache << cache_stream;

						if (target_cache == m_GeneratedDataCache[TU_path.string()]) {
							m_GeneratedDataCache.erase(TU_path.string());
							m_RunStatistics.targets_skipped++;
							continue;
						}
						else {
							m_RunStatistics.targets_generated++;
						}
					}
				}
			});
		}

		// Wait for parsing jobs
		for (int thread_idx = 0; thread_idx < m_NumThreads; thread_idx++) {
			threads[thread_idx].join();
		}
	}

	void MetaTool::GenerateCode()
	{
		for (auto& [TU_path, parsing_result] : m_GeneratedDataCache) {
			for (auto& parsed_type_iter : parsing_result.items()) {
				json& parsed_type = parsing_result[parsed_type_iter.key()];
				std::string type_name = parsed_type_iter.key();

				// Return if no "ShaderExpose" annotation - no code generation needed
				bool shader_exposed_annotation_found = false;
				bool shader_module_annotation_found = false;
				for (auto& meta_entry : parsed_type["Meta"].items()) {
					shader_exposed_annotation_found = shader_module_annotation_found || meta_entry.key() == "ShaderExpose";
					shader_module_annotation_found = shader_module_annotation_found || meta_entry.key() == "Module";
				}

				if (!shader_exposed_annotation_found) {
					continue;
				}

				// Try to acquire shader module name for exposed struct
				if (!shader_module_annotation_found) {
					std::cerr << "MetaTool: failed to run parsing action on shader exposed \"" 
						<< type_name << "\" type - the type is exposed but no shader module was specified" << std::endl;
					exit(-3);
				}

				std::vector<std::string> module_entry_values;
				parsed_type["Meta"]["Module"].get_to(module_entry_values);

				if (module_entry_values.size() != 1) {
					std::cerr << "\'Module\' meta entry may only have one value" << std::endl;
				}

				std::string type_module_name = module_entry_values[0];

				std::ofstream stream(m_WorkingDir / "Generated" / std::filesystem::path(type_name + ".slang"));

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
					stream << fmt::format("public struct {} {{", type_name) << std::endl;
					for (auto& member : parsed_type["Fields"].items()) {
						std::string field_type = member.value().get<std::string>();
						std::string field_name = member.key();

						stream << fmt::format("\t{} {};", GetShaderType(field_type), field_name) << std::endl;
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
		std::cout << fmt::format("MetaTool: {} target(s) generated, {} target(s) skipped", m_RunStatistics.targets_generated.load(), m_RunStatistics.targets_skipped.load()) << std::endl;
	}

	void MetaTool::CleanUp()
	{
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