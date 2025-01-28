#pragma once

#include "OptionParser.h"
#include <iostream>
#include <fstream>
#include <clang-c/Index.h>

using namespace Omni;

void CheckAndPrintDiagnosticMessages(CXTranslationUnit translationUnit) {
	int num_diagnostic_messages = clang_getNumDiagnostics(translationUnit);
	if(num_diagnostic_messages)
		printf("There are %i MetaTool diagnostic messages\n", num_diagnostic_messages);

	bool found_error = false;
	for (unsigned int current_message = 0; current_message < num_diagnostic_messages; ++current_message) {
		CXDiagnostic diagnotic = clang_getDiagnostic(translationUnit, current_message);
		CXString error_string = clang_formatDiagnostic(diagnotic, clang_defaultDiagnosticDisplayOptions());

		std::string tmp(clang_getCString(error_string));

		clang_disposeString(error_string);

		if (tmp.find("error:") != std::string::npos) {
			found_error = true;
		}

		std::cerr << tmp << std::endl;
	}

	if (found_error) {
		std::cerr << "Failed to run MetaTool: please resolve these issues and try again" << std::endl;
		exit(-1);
	}
}

int main(int argc, char** argv) {
	OptionParser option_parser(argc, argv);

	if (!option_parser.IsValid())
		return -1;

	CXIndex index = clang_createIndex(0, 0);

	std::vector<const char*> args = {
		CLANG_PARSER_ARGS
	};

	std::erase_if(args, [](const char* arg) {
		return std::string(arg) == std::string("-D");
	});

	CXTranslationUnit unit;

	CXErrorCode error_code = clang_parseTranslationUnit2(
		index,
		option_parser.GetPath().c_str(),
		args.data(),
		args.size(),
		nullptr,
		0,
		CXTranslationUnit_None,
		&unit
	);

	if (error_code != CXError_Success) {
		std::cerr << "Failed to run MetaTool; code error: " << error_code << std::endl;
		exit(-2);
	}

	CheckAndPrintDiagnosticMessages(unit);

	CXCursor rootCursor = clang_getTranslationUnitCursor(unit);

	clang_visitChildren(rootCursor, [](CXCursor cursor, CXCursor parent, CXClientData clientData) {
		return CXChildVisit_Break;
	}, nullptr);

	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);

	std::filesystem::path target_file(option_parser.GetPath());

	std::ofstream stream(std::filesystem::path(DUMMY_FILE_OUTPUT_DIR) / (target_file.filename().string() + ".gen"));

	return 0;
}