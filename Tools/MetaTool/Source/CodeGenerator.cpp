#include "CodeGenerator.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <spdlog/fmt/fmt.h>
#include <cctype>

namespace Omni {

	CodeGenerator::CodeGenerator(const std::filesystem::path& output_dir, const std::filesystem::path& working_dir)
		: m_output_dir(output_dir)
		, m_working_dir(working_dir)
		, m_module_caches()
	{}

	void CodeGenerator::GenerateCode(const std::unordered_map<std::string, nlohmann::ordered_json>& generated_data_cache) {
		
		try {
			for (const auto& [tu_path, parsing_result] : generated_data_cache) {
				for (const auto& parsed_type_iter : parsing_result.items()) {
					std::string type_name = parsed_type_iter.key();
					const nlohmann::ordered_json& parsed_type = parsed_type_iter.value();

					// Return if no "ShaderExpose" annotation - no code generation needed
					bool shader_exposed_annotation_found = false;
					bool shader_module_annotation_found = false;

					// Check if Meta exists before accessing it
					if (parsed_type.contains("Meta")) {
						for (const auto& meta_entry : parsed_type["Meta"].items()) {
							shader_exposed_annotation_found = shader_exposed_annotation_found || meta_entry.key() == "ShaderExpose";
							shader_module_annotation_found = shader_module_annotation_found || meta_entry.key() == "Module";
						}
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
					if (parsed_type.contains("Meta") && parsed_type["Meta"].contains("Module")) {
						parsed_type["Meta"]["Module"].get_to(module_entry_values);
					}

					if (module_entry_values.size() != 1) {
						std::cerr << "\'Module\' meta entry may only have one value" << std::endl;
					}

					std::string type_module_name = module_entry_values[0];
					bool is_shader_input = parsed_type.contains("Meta") && parsed_type["Meta"].contains("ShaderInput");

					// Check if module cache exists
					std::filesystem::path module_cache_path = m_working_dir / "Cache" / "Modules" / fmt::format("{}.cache", type_module_name);
					bool module_cache_exists = std::filesystem::exists(module_cache_path);

					if (!module_cache_exists) {
						// Load module cache if needed
						if (!m_module_caches.contains(type_module_name)) {
							std::ifstream module_cache_stream(module_cache_path);
							if (module_cache_stream.is_open()) {
								m_module_caches[type_module_name] = nlohmann::ordered_json::parse(module_cache_stream);
							}
						}
					}

					GenerateImplementation(type_name, parsed_type);
				}
			}
		}
		catch (const std::exception& e) {
			// Error handling - could be enhanced with proper logging
		}
	}

	void CodeGenerator::GenerateImplementation(const std::string& type_name, const nlohmann::ordered_json& parsed_type) {
		std::vector<std::string> module_entry_values;
		if (parsed_type.contains("Meta") && parsed_type["Meta"].contains("Module")) {
			parsed_type["Meta"]["Module"].get_to(module_entry_values);
		}
		std::string type_module_name = module_entry_values[0];
		bool is_shader_input = parsed_type.contains("Meta") && parsed_type["Meta"].contains("ShaderInput");

		std::filesystem::create_directories(m_output_dir / "Gen" / "Implementations" / type_module_name);
		std::ofstream stream(m_output_dir / "Gen" / "Implementations" / type_module_name / std::filesystem::path(type_name + ".slang"));

		GeneratePrologue(stream, type_module_name, parsed_type);
		GenerateContents(stream, type_name, parsed_type);
		GenerateEpilogue(stream, is_shader_input, type_name);

		stream.close();
	}

	void CodeGenerator::GeneratePrologue(std::ofstream& stream, const std::string& type_module_name, const nlohmann::ordered_json& parsed_type) {
		stream << "#pragma once" << std::endl;
		PrintEmptyLine(stream);
		stream << fmt::format("implementing Gen.{};", type_module_name) << std::endl;
		PrintEmptyLine(stream);

		// Detect and generate dependency imports (excluding the current module)
		std::unordered_set<std::string> dependencies = ResolveDependencies(parsed_type, type_module_name);

		// Generate imports
		for (const auto& dependency : dependencies) {
			stream << fmt::format("import Gen.{};", dependency) << std::endl;
		}

		PrintEmptyLine(stream);
		stream << "namespace Omni {" << std::endl;
	}

	void CodeGenerator::GenerateContents(std::ofstream& stream, const std::string& type_name, const nlohmann::ordered_json& parsed_type) {
		PrintEmptyLine(stream);
		
		// Check if this is an enum or struct
		if (parsed_type.contains("EnumValues")) {
			GenerateEnumContents(stream, type_name, parsed_type);
		} else {
			GenerateStructContents(stream, type_name, parsed_type);
		}
	}
	
	void CodeGenerator::GenerateEnumContents(std::ofstream& stream, const std::string& type_name, const nlohmann::ordered_json& parsed_type) {
		// Check if this is a bit flags enum
		bool is_bit_flags = parsed_type.contains("Meta") && parsed_type["Meta"].contains("BitFlags");
		
		// Generate [Flags] attribute if it's a bit flags enum
		if (is_bit_flags) {
			stream << "\t[Flags]" << std::endl;
		}
		
		stream << fmt::format("\tpublic enum {} {{", type_name) << std::endl;
		
		// Generate enum values
		for (const auto& enum_value : parsed_type["EnumValues"]) {
			std::string value_name = ToPascalCase(enum_value["Name"].get<std::string>());
			
			if (is_bit_flags) {
				// For bit flags, don't assign explicit values - let Slang handle it
				stream << fmt::format("\t\t{},", value_name) << std::endl;
			} else {
				// For regular enums, assign the explicit values
				int64_t value = enum_value["Value"].get<int64_t>();
				stream << fmt::format("\t\t{} = {},", value_name, value) << std::endl;
			}
		}
		
		stream << "\t};" << std::endl;
		PrintEmptyLine(stream);
	}
	
	void CodeGenerator::GenerateStructContents(std::ofstream& stream, const std::string& type_name, const nlohmann::ordered_json& parsed_type) {
		
		// Check for template arguments in Meta
		std::string template_args = "";
		if (parsed_type.contains("Meta") && parsed_type["Meta"].contains("TemplateArguments")) {
			const auto& arr = parsed_type["Meta"]["TemplateArguments"];
			if (!arr.empty()) {
				template_args = arr[0];
			}
		}

		// Generate struct declaration with template arguments if present
		if (!template_args.empty()) {
			stream << fmt::format("\tpublic struct {}<{}> {{", type_name, template_args) << std::endl;
		} else {
			stream << fmt::format("\tpublic struct {} {{", type_name) << std::endl;
		}
		
		if (!parsed_type.contains("Fields")) {
			stream << "\t};" << std::endl;
			PrintEmptyLine(stream);
			return;
		}
		
		for (const auto& member : parsed_type["Fields"].items()) {
			const nlohmann::ordered_json& field_data = member.value();
			std::string field_type = field_data["Type"].get<std::string>();
			std::string field_name = field_data["Name"].get<std::string>();

			// Get field visibility (default to public if not specified)
			std::string visibility = "public";
			if (field_data["Meta"].contains("Visibility")) {
				visibility = field_data["Meta"]["Visibility"].get<std::string>();
			}

			// Check if this is a conditional field
			bool is_conditional = field_data["Meta"].contains("Condition");
			std::string condition = "";
			
			if (is_conditional) {
				std::vector<std::string> condition_values;
				field_data["Meta"]["Condition"].get_to(condition_values);
				if (!condition_values.empty()) {
					condition = condition_values[0];
				}
			}

			std::string array_subscript = ExtractArraySubscript(field_type);

			// Check if this is a bit field
			bool is_bit_field = field_data.contains("BitWidth");
			
			// Generate conditional field with proper syntax
			if (is_conditional && !condition.empty()) {
				// Extract the base type from ShaderConditional<T>
				std::string base_type = field_type;
				if (field_type.find("ShaderConditional<") == 0) {
					size_t start = field_type.find('<') + 1;
					size_t end = field_type.find('>');
					if (start != std::string::npos && end != std::string::npos) {
						base_type = field_type.substr(start, end - start);
					}
				}
				stream << fmt::format("\t\t{} Conditional<{}, {}> {}{};", visibility, base_type, condition, field_name, array_subscript) << std::endl;
			} else if (is_bit_field) {
				// Handle bit fields
				uint32_t bit_width = field_data["BitWidth"].get<uint32_t>();
				stream << fmt::format("\t\t{} {} {} : {};", visibility, GetShaderType(field_type), field_name, bit_width) << std::endl;
			} else {
				stream << fmt::format("\t\t{} {} {}{};", visibility, GetShaderType(field_type), field_name, array_subscript) << std::endl;
			}
		}
		
		stream << "\t};" << std::endl;
		PrintEmptyLine(stream);
	}

	void CodeGenerator::GenerateEpilogue(std::ofstream& stream, bool is_shader_input, const std::string& type_name) {
		// Declare push constant in case if it is annotated with ShaderInput
		if (is_shader_input) {
			stream << "[[vk::push_constant]]" << std::endl;
			stream << fmt::format("public ConstantBuffer<{}> Input;", type_name) << std::endl;
			PrintEmptyLine(stream);
		}

		stream << "}" << std::endl;
	}

	std::unordered_set<std::string> CodeGenerator::ResolveDependencies(const nlohmann::ordered_json& parsed_type, const std::string& current_module_name) {
		std::unordered_set<std::string> dependencies;

		if (!parsed_type.contains("Fields")) {
			return dependencies;
		}

		for (const auto& member : parsed_type["Fields"].items()) {
			const nlohmann::ordered_json& field_data = member.value();
			std::string field_type_name = field_data["Type"].get<std::string>();

			std::string special_characters = "*[";
			size_t first_special_character_occurrence = field_type_name.find_first_of(special_characters);

			if (first_special_character_occurrence != std::string::npos) {
				field_type_name.erase(first_special_character_occurrence);
			}

			std::string field_module_name = GetFieldModuleName(field_type_name);
			if (!field_module_name.empty() && field_module_name != current_module_name) {
				dependencies.emplace(field_module_name);
			}
		}

		return dependencies;
	}

	std::string CodeGenerator::GetFieldModuleName(const std::string& field_type_name) {
		std::filesystem::path type_module_cache_path = m_working_dir / "Cache" / "TypeModules" / std::filesystem::path(field_type_name + ".cache");

		if (!std::filesystem::exists(type_module_cache_path)) {
			return "";
		}

		std::ifstream type_module_cache_stream(type_module_cache_path);
		if (!type_module_cache_stream.is_open()) {
			return "";
		}

		// A hack, for some reason it cannot parse json directly here
		std::string module_cache_data_string((std::istreambuf_iterator<char>(type_module_cache_stream)),
			std::istreambuf_iterator<char>());

		nlohmann::ordered_json type_module_cache = nlohmann::ordered_json::parse(module_cache_data_string);
		return type_module_cache["Module"].get<std::string>();
	}

	void CodeGenerator::AssembleModules(const std::vector<std::string>& pending_module_reassemblies) {
		for (const auto& pending_module_reassembly_name : pending_module_reassemblies) {
			std::ofstream module_stream(m_output_dir / "Gen" / fmt::format("{}.slang", pending_module_reassembly_name));
			std::ofstream module_cache_stream(m_working_dir / "Cache" / "Modules" / fmt::format("{}.cache", pending_module_reassembly_name));

			nlohmann::ordered_json module_cache = nlohmann::ordered_json::array();

			module_stream << fmt::format("module {};", pending_module_reassembly_name) << std::endl;

			// Add all implementations to this module code and cache
			std::filesystem::directory_iterator implementation_iterator(m_output_dir / "Gen" / "Implementations" / pending_module_reassembly_name);
			for (const auto& implementation : implementation_iterator) {
				const std::string implementation_stem = implementation.path().stem().string();
				module_cache.emplace_back(implementation_stem);

				module_stream << fmt::format("__include Gen.Implementations.{}.{};", pending_module_reassembly_name, implementation_stem) << std::endl;
			}

			module_cache_stream << module_cache.dump(4);
		}
	}

	void CodeGenerator::PrintEmptyLine(std::ofstream& stream) {
		stream << std::endl;
	}

	std::string CodeGenerator::GetShaderType(const std::string& source_type) {
		static std::unordered_map<std::string, std::string> shader_type_lookup_table = {
			{ "glm::vec",		"float"		},
			{ "glm::uvec",		"uint"		},
			{ "glm::ivec",		"int"		},
			{ "glm::hvec",		"half"		},
			{ "glm::i8vec",		"int8_t"	},
			{ "glm::i16vec",	"int16_t"	},
			{ "glm::u8vec",		"int16_t"	},
			{ "glm::u16vec",	"int16_t"	},

			{ "glm::mat4",		"float4x4"	},

			{ "uint64",			"uint64_t"	},
			{ "uint32",			"uint"		},
			{ "uint16",			"uint16_t"	},
			{ "uint8",			"uint8_t"	},
			{ "float64",		"double"	},
			{ "float32",		"float"		},
			{ "float16",		"half"		},
			{ "int32",			"int"		},
			{ "int16",			"int16_t"	},
			{ "int8",			"int8_t"	},
			{ "byte",			"uint8_t"	}
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

	std::string CodeGenerator::ToPascalCase(const std::string& input) {
		if (input.empty()) {
			return input;
		}

		std::string result = input;
		
		// Convert to lowercase first
		std::transform(result.begin(), result.end(), result.begin(), ::tolower);
		
		// Capitalize first letter
		if (!result.empty()) {
			result[0] = ::toupper(result[0]);
		}
		
		// Capitalize letters after underscores (convert snake_case to PascalCase)
		for (size_t i = 1; i < result.length(); ++i) {
			if (result[i] == '_' && i + 1 < result.length()) {
				result[i + 1] = ::toupper(result[i + 1]);
			}
		}
		
		// Remove underscores
		result.erase(std::remove(result.begin(), result.end(), '_'), result.end());
		
		// Handle consecutive uppercase letters (like "OPAQUE" -> "Opaque")
		for (size_t i = 1; i < result.length(); ++i) {
			if (std::isupper(result[i]) && std::isupper(result[i-1])) {
				result[i] = ::tolower(result[i]);
			}
		}
		
		return result;
	}

	std::string CodeGenerator::ExtractArraySubscript(std::string& field_type) {
		std::string array_subscript;
		size_t array_subscript_begin_index = field_type.find_first_of('[');

		// Check if it is array type
		if (array_subscript_begin_index != std::string::npos) {
			array_subscript = field_type.substr(array_subscript_begin_index);
			field_type.erase(array_subscript_begin_index);
		}

		return array_subscript;
	}

} 