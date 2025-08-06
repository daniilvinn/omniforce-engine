#include "ASTVisitor.h"
#include <iostream>
#include <regex>
#include <spdlog/fmt/fmt.h>
#include <string>

namespace Omni {

	ASTVisitor::ASTVisitor() : m_CurrentParsingResult(nullptr) {}

	ASTVisitor::~ASTVisitor() = default;

	nlohmann::ordered_json ASTVisitor::TraverseTranslationUnit(CXTranslationUnit translation_unit) {
		nlohmann::ordered_json parsing_result = nlohmann::ordered_json::object();
		m_CurrentParsingResult = &parsing_result;

		CXCursor root_cursor = clang_getTranslationUnitCursor(translation_unit);
		clang_visitChildren(root_cursor, VisitCursor, &parsing_result);

		return parsing_result;
	}

	CXChildVisitResult ASTVisitor::VisitCursor(CXCursor cursor, CXCursor parent, CXClientData client_data) {
		// Check if we are in Omniforce source file
		CXSourceLocation current_cursor_location = clang_getCursorLocation(cursor);
		CXSourceLocation parent_cursor_location = clang_getCursorLocation(parent);

		if (!IsFromMainFile(current_cursor_location) && !IsFromMainFile(parent_cursor_location)) {
			return CXChildVisit_Continue;
		}

		nlohmann::ordered_json& parsing_result = *(nlohmann::ordered_json*)client_data;

		// Check if we are on attribute
		if (clang_isAttribute(clang_getCursorKind(cursor))) {
			std::string type_name = GetCursorSpelling(parent);
			parsing_result[type_name] = nlohmann::ordered_json::object();
			nlohmann::ordered_json& parsed_type = parsing_result[type_name];

			// Parse meta attributes first
			ASTVisitor visitor;
			visitor.ParseMetaAttributes(cursor, parsed_type);

			// Check if this is an enum or struct/class
			CXCursorKind parent_kind = clang_getCursorKind(parent);
			if (parent_kind == CXCursor_EnumDecl) {
				// Handle enum
				ParseEnumValues(parent, parsed_type);
			} else if (parent_kind == CXCursor_StructDecl || parent_kind == CXCursor_ClassDecl) {
				// Handle struct/class
				CXType type = clang_getCursorType(parent);

				// Traverse its fields to generate code
				clang_Type_visitFields(type, VisitField, &parsed_type);
			}
		}

		return CXChildVisit_Recurse;
	}

	CXVisitorResult ASTVisitor::VisitField(CXCursor field_cursor, CXClientData client_data) {
		nlohmann::ordered_json& field_output = *(nlohmann::ordered_json*)client_data;
		
		// Initialize Fields object only if it doesn't exist
		if (!field_output.contains("Fields")) {
			field_output["Fields"] = nlohmann::ordered_json::object();
		}

		// Use static methods instead of creating a new visitor instance
		std::string field_type_name = ParseFieldType(field_cursor);
		std::string field_name = GetCursorSpelling(field_cursor);

		// Create field data structure
		nlohmann::ordered_json field_data = CreateFieldData(field_type_name, field_name);
		
		// Handle bit fields
		if (clang_Cursor_isBitField(field_cursor)) {
			uint32_t bit_width = clang_getFieldDeclBitWidth(field_cursor);
			field_data["BitWidth"] = bit_width;
		}
		
		// Parse field-level meta attributes if present
		ParseFieldMetaAttributes(field_cursor, field_data);

		field_output["Fields"][field_name] = field_data;

		return CXVisit_Continue;
	}

	nlohmann::ordered_json ASTVisitor::CreateFieldData(const std::string& field_type, const std::string& field_name) {
		nlohmann::ordered_json field_data = nlohmann::ordered_json::object();
		field_data["Type"] = field_type;
		field_data["Name"] = field_name;
		field_data["Meta"] = nlohmann::ordered_json::object();
		return field_data;
	}

	void ASTVisitor::ParseFieldMetaAttributes(CXCursor field_cursor, nlohmann::ordered_json& field_data) {
		// Check if the field has any attributes
		clang_visitChildren(field_cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) {
			if (clang_isAttribute(clang_getCursorKind(cursor))) {
				ParseMetaAttributes(cursor, *(nlohmann::ordered_json*)client_data);
			}
			return CXChildVisit_Continue;
		}, &field_data);
		
		// Set default visibility to public if not specified
		if (!field_data["Meta"].contains("Visibility")) {
			field_data["Meta"]["Visibility"] = "public";
		}
	}

	std::string ASTVisitor::ParseFieldType(CXCursor field_cursor) {
		CXType field_type = clang_getCursorType(field_cursor);
		std::string type_name = GetTypeSpelling(field_type);

		// Handle template types using Clang's template API
		if (clang_Type_getNumTemplateArguments(field_type) > 0) {
			return ParseTemplateType(field_type, type_name);
		}

		return GetShaderType(type_name);
	}

	std::string ASTVisitor::ParseTemplateType(CXType type, const std::string& type_name) {
		// Use Clang's template API instead of manual string parsing
		if (type_name.find("BDA") == 0) {
			return ParseBDA(type);
		}
		else if (type_name.find("ShaderRuntimeArray") == 0) {
			return ParseShaderRuntimeArray(type);
		}
		else if (IsShaderConditionalType(type_name)) {
			return ParseShaderConditional(type);
		}

		// For other template types, just get the base type
		return GetShaderType(type_name);
	}

	bool ASTVisitor::IsShaderConditionalType(const std::string& type_name) {
		return type_name.find("ShaderConditional") == 0;
	}

	std::string ASTVisitor::ParseBDA(CXType type) {
		// Use Clang's template argument API
		if (clang_Type_getNumTemplateArguments(type) >= 1) {
			CXType template_arg = clang_Type_getTemplateArgumentAsType(type, 0);
			std::string bda_type = GetShaderType(GetTypeSpelling(template_arg));
			return bda_type + "*";
		}
		return "void*"; // Fallback
	}

	std::string ASTVisitor::ParseShaderRuntimeArray(CXType type) {
		// Use Clang's template argument API
		if (clang_Type_getNumTemplateArguments(type) >= 1) {
			CXType template_arg = clang_Type_getTemplateArgumentAsType(type, 0);
			std::string array_type = GetShaderType(GetTypeSpelling(template_arg));
			return array_type + "[]";
		}
		return "void[]"; // Fallback
	}

	std::string ASTVisitor::ParseShaderConditional(CXType type) {
		// Use Clang's template argument API for proper parsing
		int num_template_args = clang_Type_getNumTemplateArguments(type);
		
		if (num_template_args >= 1) {
			// First template argument: type
			CXType arg0 = clang_Type_getTemplateArgumentAsType(type, 0);
			std::string conditional_type_name = GetShaderType(GetTypeSpelling(arg0));

			return fmt::format("ShaderConditional<{}>", conditional_type_name);
		}

		return "ShaderConditional<void>"; // Fallback
	}



	void ASTVisitor::ParseEnumValues(CXCursor enum_cursor, nlohmann::ordered_json& parsed_type) {
		// Initialize enum values array
		parsed_type["EnumValues"] = nlohmann::ordered_json::array();
		
		// Visit all enum constants
		clang_visitChildren(enum_cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) {
			if (clang_getCursorKind(cursor) == CXCursor_EnumConstantDecl) {
				std::string enum_value_name = GetCursorSpelling(cursor);
				int64_t enum_value = clang_getEnumConstantDeclValue(cursor);
				
				nlohmann::ordered_json enum_value_data = nlohmann::ordered_json::object();
				enum_value_data["Name"] = enum_value_name;
				enum_value_data["Value"] = enum_value;
				
				(*(nlohmann::ordered_json*)client_data)["EnumValues"].emplace_back(enum_value_data);
			}
			return CXChildVisit_Continue;
		}, &parsed_type);
	}

	void ASTVisitor::ParseMetaAttributes(CXCursor cursor, nlohmann::ordered_json& parsed_type) {
		std::string input = GetCursorSpelling(cursor);
		std::regex regex(R"(\s*([\w]+)\s*(?:=\s*\"([^\"]*)\")?)");

		std::sregex_iterator iterator_begin(input.begin(), input.end(), regex);
		std::sregex_iterator iterator_end;

		// Parse metadata
		parsed_type["Meta"] = nlohmann::ordered_json::object();

		for (auto it = iterator_begin; it != iterator_end; ++it) {
			std::string meta_entry_name = (*it)[1].str();
			parsed_type["Meta"][meta_entry_name] = nlohmann::ordered_json::array();

			if ((*it)[2].matched) {
				parsed_type["Meta"][meta_entry_name].emplace_back((*it)[2].str());
			}
		}

		// Handle Private attribute - convert to Visibility
		if (parsed_type["Meta"].contains("Private")) {
			parsed_type["Meta"]["Visibility"] = "private";
		}
		
		// Handle Internal attribute - convert to Visibility
		if (parsed_type["Meta"].contains("Internal")) {
			parsed_type["Meta"]["Visibility"] = "internal";
		}

		// Mark as MetaTool-annotated type if it has ShaderExpose
		bool metatool_annotated_type = parsed_type["Meta"].contains("ShaderExpose");
		parsed_type["Meta"]["MetaToolAnnotated"] = metatool_annotated_type;
	}

	std::string ASTVisitor::GetCursorSpelling(CXCursor cursor) {
		CXString spelling = clang_getCursorSpelling(cursor);
		std::string result = clang_getCString(spelling);
		clang_disposeString(spelling);
		return result;
	}

	std::string ASTVisitor::GetTypeSpelling(CXType type) {
		CXString spelling = clang_getTypeSpelling(type);
		std::string result = clang_getCString(spelling);
		clang_disposeString(spelling);
		return result;
	}

	bool ASTVisitor::IsFromMainFile(CXSourceLocation location) {
		return clang_Location_isFromMainFile(location);
	}

	std::string ASTVisitor::GetShaderType(const std::string& source_type) {
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

} 