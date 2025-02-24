#include "MetaTool.h"

#include <iostream>
#include <regex>
#include <fstream>
#include <thread>

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

namespace Omni {

	MetaTool::MetaTool()
		: m_Index()
		, m_TranslationUnits()
		, m_WorkingDir(METATOOL_WORKING_DIRECTORY)
		, m_OutputDir(METATOOL_OUTPUT_DIRECTORY)
		, m_GeneratedDataCache()
		, m_NumThreads(std::thread::hardware_concurrency())
	{}

	void MetaTool::Setup()
	{
		// Load parse targets
		std::ifstream parse_targets_stream(m_WorkingDir / "ParseTargets.txt");
		StringPath parse_target;

		while (std::getline(parse_targets_stream, parse_target)) {
			m_ParseTargets.push_back(parse_target);
		}

		// Exit if no parse targets (maybe throw a warning in this case?)
		if (m_ParseTargets.empty()) {
			exit(0);
		}

		std::cout << "MetaTool: running " << m_ParseTargets.size() << " action(s)" << std::endl;

		std::filesystem::create_directories(m_WorkingDir / "DummyOut");
		std::filesystem::create_directories(m_WorkingDir / "Cache" / "Modules");
		std::filesystem::create_directories(m_OutputDir / "Gen");

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
					StringPath target_filename = std::filesystem::path(TU_path).filename().string();

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

						CacheType TU_parsing_result = CacheType::object();

						clang_visitChildren(root_cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) {
							// Check if we are in Omniforce source file
							CXSourceLocation current_cursor_location = clang_getCursorLocation(cursor);
							CXSourceLocation parent_cursor_location = clang_getCursorLocation(parent);

							if (!clang_Location_isFromMainFile(current_cursor_location) && !clang_Location_isFromMainFile(parent_cursor_location)) {
								return CXChildVisit_Continue;
							}

							// Start traversal
							CacheType& parsing_result = *(CacheType*)client_data;

							// Check if we are on attribute
							if (clang_isAttribute(clang_getCursorKind(cursor))) {
								const char* type_name = clang_getCString(clang_getCursorSpelling(parent));

								parsing_result.emplace(type_name, CacheType::object());
								CacheType& parsed_type = parsing_result.at(type_name);

								// If so, acquire parent type (that is a struct or a class)
								CXType type = clang_getCursorType(parent);

								// Traverse its fields to generate code
								clang_Type_visitFields(type, [](CXCursor field_cursor, CXClientData field_client_data) {
									CacheType& field_output = *(CacheType*)field_client_data;
									field_output.emplace("Fields", CacheType::object());

									std::string field_type_name = clang_getCString(clang_getTypeSpelling(clang_getCursorType(field_cursor)));
									std::string field_name = clang_getCString(clang_getCursorSpelling(field_cursor));

									// If it is BDA type, convert it to a pointer
									if (field_type_name.find("BDA") == 0) {
										size_t bda_type_begin_index = field_type_name.find('<') + 1;
										size_t bda_type_end_index = field_type_name.find('>');

										std::string bda_type = field_type_name.substr(bda_type_begin_index, bda_type_end_index - bda_type_begin_index) + '*';
										field_type_name = bda_type;
									}

									field_output["Fields"].emplace(
										field_name,
										field_type_name
									);

									return CXVisit_Continue;
								}, &parsed_type);

								// Parse attributes
								std::string input = clang_getCString(clang_getCursorSpelling(cursor));
								std::regex regex(R"(\s*([\w]+)\s*(?:=\s*\"([^\"]*)\")?)");

								std::sregex_iterator iterator_begin(input.begin(), input.end(), regex);
								std::sregex_iterator iterator_end;

								// Parse metadata
								parsed_type.emplace("Meta", CacheType::object());

								for (iterator_begin; iterator_begin != iterator_end; ++iterator_begin) {
									std::string meta_entry_name = (*iterator_begin)[1].str();
									parsed_type["Meta"].emplace(meta_entry_name, CacheType::array());

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
					// ==========================================================
					// Check if code generation is needed, compute run statistics
					// ==========================================================
					{ 
						StringPath validation_target_filename = TU_path.filename().string();

						// If target has no cache, then it we must generate it
						std::filesystem::path cache_path = m_WorkingDir / "Cache" / fmt::format("{}.cache", validation_target_filename);
						bool target_is_cached = std::filesystem::exists(cache_path);

						if (!target_is_cached) {
							m_RunStatistics.targets_generated++;
							continue;
						}

						// Compare caches. Skip code generation stage if caches match
						std::ifstream cache_stream(cache_path);

						CacheType target_cache;
						target_cache << cache_stream;

						for (const auto& cached_type_node : target_cache.items()) {
							std::string cached_type_name = cached_type_node.key();
							CacheType cached_type = cached_type_node.value();

							// If type was deleted, need to also delete its shader implementation
							if (m_GeneratedDataCache[TU_path.string()].contains(cached_type_name)) {
								if (m_GeneratedDataCache[TU_path.string()][cached_type_name]["Meta"] == cached_type["Meta"]) {
									continue;
								}
							}

							bool shader_exposed_annotation_found = false;
							bool shader_module_annotation_found = false;

							for (auto& meta_entry : cached_type["Meta"].items()) {
								shader_exposed_annotation_found = shader_exposed_annotation_found || meta_entry.key() == "ShaderExpose";
								shader_module_annotation_found = shader_module_annotation_found || meta_entry.key() == "Module";
							}

							if (!shader_exposed_annotation_found) {
								continue;
							}

							// Try to acquire shader module name for exposed struct
							if (!shader_module_annotation_found) {
								std::cerr << "MetaTool: failed to run parsing action on shader exposed \""
									<< cached_type_name << "\" type - the type is exposed but no shader module was specified" << std::endl;
								exit(-3);
							}

							std::vector<std::string> module_entry_values;
							cached_type["Meta"]["Module"].get_to(module_entry_values);

							if (module_entry_values.size() != 1) {
								std::cerr << "\'Module\' meta entry may only have one value" << std::endl;
							}

							std::string type_module_name = module_entry_values[0];

							// Delete the file
							std::filesystem::remove(m_OutputDir / "Gen" / "Implementations" / type_module_name / std::filesystem::path(cached_type_name + ".slang"));
						}

						if (target_cache == m_GeneratedDataCache[TU_path.string()]) {
							m_GeneratedDataCache.erase(TU_path.string());
							m_RunStatistics.targets_skipped++;
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
		// Generate implementation code
		try {
			for (auto& [TU_path, parsing_result] : m_GeneratedDataCache) {
				for (auto& parsed_type_iter : parsing_result.items()) {
					std::string type_name = parsed_type_iter.key();
					CacheType& parsed_type = parsing_result[parsed_type_iter.key()];

					// Return if no "ShaderExpose" annotation - no code generation needed
					bool shader_exposed_annotation_found = false;
					bool shader_module_annotation_found = false;

					for (auto& meta_entry : parsed_type["Meta"].items()) {
						shader_exposed_annotation_found = shader_exposed_annotation_found || meta_entry.key() == "ShaderExpose";
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

					// If cache does not exist, we inevitably need to assemble the module
					std::filesystem::path module_cache_path =  m_WorkingDir / "Cache" / "Modules" / fmt::format("{}.cache", type_module_name);
					bool module_cache_exists = std::filesystem::exists(module_cache_path);

					if (!module_cache_exists) {
						bool module_pending = std::find(m_PendingModuleReassemblies.begin(), m_PendingModuleReassemblies.end(), type_module_name) != m_PendingModuleReassemblies.end();

						if (!module_pending) {
							m_PendingModuleReassemblies.emplace_back(type_module_name);
						}
					}
					else {
						// Load module cache if needed
						if (!m_ModuleCaches.contains(type_module_name) && !module_cache_exists) {
							std::ifstream module_cache_stream(module_cache_path);
							m_ModuleCaches[type_module_name] = CacheType::parse(module_cache_stream);
						}
					
						// Check if module reassembly is needed
						if (!m_ModuleCaches[type_module_name].contains(type_name)) {
							bool module_pending = std::find(m_PendingModuleReassemblies.begin(), m_PendingModuleReassemblies.end(), type_module_name) != m_PendingModuleReassemblies.end();

							if (!module_pending) {
								m_PendingModuleReassemblies.emplace_back(type_module_name);
							}
						}
					}

					std::filesystem::create_directories(m_OutputDir / "Gen" / "Implementations" / type_module_name);
					std::ofstream stream(m_OutputDir / "Gen" / "Implementations" / type_module_name / std::filesystem::path(type_name + ".slang"));

					// Generate prologue
					{
						stream << "#pragma once" << std::endl;
						PrintEmptyLine(stream);
						stream << fmt::format("implementing Gen.{};", type_module_name) << std::endl;
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

							std::string array_subscript;

							size_t array_subscript_begin_index = field_type.find_first_of('[');
							size_t bda_declaration_begin_index = field_type.find_first_of("BDA<");

							// Check if it is array type
							if (array_subscript_begin_index != std::string::npos) {
								array_subscript = field_type.substr(array_subscript_begin_index);
								field_type.erase(array_subscript_begin_index);
							}

							stream << fmt::format("\t{} {}{};", GetShaderType(field_type), field_name, array_subscript) << std::endl;
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
		catch (const std::exception& e) {
			std::cout << fmt::format("MetaTool run failed while generating code; error: {}", e.what()) << std::endl;
		}
	}

	void MetaTool::AssembleModules()
	{
		for (const auto& pending_module_reassembly_name : m_PendingModuleReassemblies) {
			std::ofstream module_stream(m_OutputDir / "Gen" / fmt::format("{}.slang", pending_module_reassembly_name));
			std::ofstream module_cache_stream(m_WorkingDir / "Cache" / "Modules" / fmt::format("{}.cache", pending_module_reassembly_name));

			CacheType module_cache = CacheType::array();

			module_stream << fmt::format("module {};", pending_module_reassembly_name) << std::endl;

			// Add all implementations to this module code and cache
			std::filesystem::directory_iterator implementation_iterator(m_OutputDir / "Gen" / "Implementations" / pending_module_reassembly_name);
			for (const auto& implementation : implementation_iterator) {
				const std::string implementation_stem = implementation.path().stem().string();
				module_cache.emplace_back(implementation_stem);

				module_stream << fmt::format("__include Implementations.{}.{};", pending_module_reassembly_name, implementation_stem) << std::endl;
			}

			module_cache_stream << module_cache.dump(4);
		}
	}

	void MetaTool::DumpResults()
	{
		for (auto& parse_target : m_ParseTargets) {
			std::filesystem::path target_file(parse_target.filename());

			//std::ofstream stream(m_WorkingDir / "DummyOut" / (target_file.string() + ".gen"));
			//stream.close();

			std::ofstream stream(m_WorkingDir / "Cache" / (target_file.string() + ".cache"));
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

	// Does NOT support matrices yet
	std::string MetaTool::GetShaderType(const std::string& source_type) const
	{
		static std::unordered_map<std::string, std::string> shader_type_lookup_table = {
			{ "glm::vec",	"float" },
			{ "glm::uvec",	"uint" },
			{ "glm::ivec",	"int" },
			{ "glm::hvec",	"half" },

			{ "uint64_t",	"uint64" },
			{ "uint32",		"uint" },
			{ "float64",	"double" },
			{ "float32",	"float" },
			{ "uint16",		"uint16_t" },
			{ "float16",	"half" },
			{ "uint8",		"uint8_t" },
			{ "byte",		"uint8_t" }
		};

		// Check if it is a primitive type
		if (shader_type_lookup_table.contains(source_type)) {
			return shader_type_lookup_table[source_type];
		}

		// Check if it is a vector type
		if (source_type.find("glm::") != std::string::npos) {
			char num_vector_components = source_type[source_type.size() - 1];
			std::string vector_scalar_type = source_type.substr(0, source_type.size() - 1);

			return fmt::format("{}{}", shader_type_lookup_table[vector_scalar_type], num_vector_components);
		}
		else {
			return source_type;
		}

	}

}