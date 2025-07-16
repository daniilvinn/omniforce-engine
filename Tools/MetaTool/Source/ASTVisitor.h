#pragma once

#include <clang-c/Index.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace Omni {

	class ASTVisitor {
	public:
		ASTVisitor();
		~ASTVisitor();

		// Main traversal function
		nlohmann::ordered_json TraverseTranslationUnit(CXTranslationUnit translation_unit);

	private:
		// AST traversal callbacks
		static CXChildVisitResult VisitCursor(CXCursor cursor, CXCursor parent, CXClientData client_data);
		static CXVisitorResult VisitField(CXCursor field_cursor, CXClientData client_data);

		// Helper functions for parsing different field types
		static std::string ParseFieldType(CXCursor field_cursor);
		static std::string ParseTemplateType(CXType type, const std::string& type_name);
		static std::string ParseBDA(CXType type);
		static std::string ParseShaderRuntimeArray(CXType type);
		static std::string ParseShaderConditional(CXType type);

		// Meta attribute parsing
		static void ParseMetaAttributes(CXCursor cursor, nlohmann::ordered_json& parsed_type);
		static void ParseFieldMetaAttributes(CXCursor field_cursor, nlohmann::ordered_json& field_data);
		
		// Enum parsing
		static void ParseEnumValues(CXCursor enum_cursor, nlohmann::ordered_json& parsed_type);
		
		// Utility functions
		static std::string GetCursorSpelling(CXCursor cursor);
		static std::string GetTypeSpelling(CXType type);
		static bool IsFromMainFile(CXSourceLocation location);
		static std::string GetShaderType(const std::string& source_type);
		
		// Field data structure helpers
		static nlohmann::ordered_json CreateFieldData(const std::string& field_type, const std::string& field_name);
		static bool IsShaderConditionalType(const std::string& type_name);

	private:
		nlohmann::ordered_json* m_CurrentParsingResult;
	};

} 