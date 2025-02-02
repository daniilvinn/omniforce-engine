#pragma once

#include "OptionParser.h"
#include "MetaTool.h"

using namespace Omni;

int main(int argc, char** argv) {

	MetaTool meta_tool;

	meta_tool.Setup();
	meta_tool.TraverseAST();
	meta_tool.GenerateCode();
	meta_tool.DumpResults();
	meta_tool.CleanUp();

	return 0;
}