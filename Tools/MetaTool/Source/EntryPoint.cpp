#pragma once

#include "OptionParser.h"
#include "MetaTool.h"

using namespace Omni;

int main(int argc, char** argv) {

	MetaTool meta_tool;

	meta_tool.Setup();
	meta_tool.TraverseAST();
	meta_tool.DumpCache();
	meta_tool.GenerateCode();
	meta_tool.AssembleModules();
	meta_tool.PrintStatistics();
	meta_tool.CleanUp();

	return 0;
}