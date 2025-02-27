#pragma once

#include "OptionParser.h"
#include "MetaTool.h"

using namespace Omni;

int main(int argc, char** argv) {

	MetaTool meta_tool;

	meta_tool.Setup();
	meta_tool.TraverseAST();
	meta_tool.CleanUp();
	meta_tool.DumpCache();
	meta_tool.GenerateCode();
	meta_tool.AssembleModules();

	return 0;
}