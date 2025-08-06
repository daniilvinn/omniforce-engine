#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Omni {

	class CodeGenerator {
	public:
		CodeGenerator(const std::filesystem::path& output_dir, const std::filesystem::path& working_dir);
		~CodeGenerator() = default;

		// Main generation function
		void GenerateCode(const std::unordered_map<std::string, nlohmann::ordered_json>& generated_data_cache);

		// Module assembly
		void AssembleModules(const std::vector<std::string>& pending_module_reassemblies);

	private:
		// Code generation helpers
		void GenerateImplementation(const std::string& type_name, const nlohmann::ordered_json& parsed_type);
		void GeneratePrologue(std::ofstream& stream, const std::string& type_module_name, const nlohmann::ordered_json& parsed_type);
		void GenerateContents(std::ofstream& stream, const std::string& type_name, const nlohmann::ordered_json& parsed_type);
		void GenerateEnumContents(std::ofstream& stream, const std::string& type_name, const nlohmann::ordered_json& parsed_type);
		void GenerateStructContents(std::ofstream& stream, const std::string& type_name, const nlohmann::ordered_json& parsed_type);
		void GenerateEpilogue(std::ofstream& stream, bool is_shader_input, const std::string& type_name);

		// Dependency resolution
		std::unordered_set<std::string> ResolveDependencies(const nlohmann::ordered_json& parsed_type, const std::string& current_module_name);
		std::string GetFieldModuleName(const std::string& field_type_name);

		// Utility functions
		void PrintEmptyLine(std::ofstream& stream);
		std::string GetShaderType(const std::string& source_type);
		std::string ExtractArraySubscript(std::string& field_type);
		std::string ToPascalCase(const std::string& input);

	private:
		std::filesystem::path m_output_dir;
		std::filesystem::path m_working_dir;
		std::unordered_map<std::string, nlohmann::ordered_json> m_module_caches;
	};

} 