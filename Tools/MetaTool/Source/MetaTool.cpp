#include "MetaTool.h"

#include <iostream>
#include <fstream>
#include <thread>

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

namespace Omni {

	MetaTool::MetaTool()
		: m_WorkingDir(METATOOL_WORKING_DIRECTORY)
		, m_OutputDir(METATOOL_OUTPUT_DIRECTORY)
		, m_NumThreads(std::thread::hardware_concurrency())
		, m_GeneratedDataCache()
		, m_PendingModuleReassemblies()
	{}

	void MetaTool::Setup() {
		LoadParseTargets();
		CreateDirectories();

		// Initialize components - CacheManager first, then others that depend on it
		m_CacheManager = std::make_unique<CacheManager>(m_WorkingDir);
		m_Parser = std::make_unique<Parser>(m_WorkingDir, m_NumThreads, m_CacheManager.get());
		m_CodeGenerator = std::make_unique<CodeGenerator>(m_OutputDir, m_WorkingDir);

		std::cout << "MetaTool: running " << m_ParseTargets.size() << " action(s)" << std::endl;
	}

	void MetaTool::TraverseAST() {
		// Use the Parser to handle AST traversal
		m_GeneratedDataCache = m_Parser->ParseTargets(m_ParseTargets);
		
		// Update statistics
		m_RunStatistics.targets_generated = m_Parser->GetTargetsGenerated();
		m_RunStatistics.targets_skipped = m_Parser->GetTargetsSkipped();
	}

	void MetaTool::GenerateCode() {
		// Use the CodeGenerator to handle code generation
		m_CodeGenerator->GenerateCode(m_GeneratedDataCache);
	}

	void MetaTool::AssembleModules() {
		// Get pending module reassemblies from cache manager
		m_PendingModuleReassemblies = m_CacheManager->GetPendingModuleReassemblies(m_GeneratedDataCache);
		
		// Use the CodeGenerator to assemble modules
		m_CodeGenerator->AssembleModules(m_PendingModuleReassemblies);
	}

	void MetaTool::DumpCache() {
		// Use the CacheManager to dump caches
		for (const auto& [tuPath, parsingResult] : m_GeneratedDataCache) {
			m_CacheManager->DumpTargetCache(tuPath, parsingResult);
		}
		
		m_CacheManager->DumpTypeModuleCaches(m_GeneratedDataCache);
	}

	void MetaTool::CleanUp() {
		// Cleanup is handled by RAII in the component classes
		m_Parser.reset();
		m_CodeGenerator.reset();
		m_CacheManager.reset();
	}

	void MetaTool::PrintStatistics() {
		std::cout << fmt::format("MetaTool: {} target(s) generated, {} target(s) skipped", 
			m_RunStatistics.targets_generated.load(), m_RunStatistics.targets_skipped.load()) << std::endl;
		std::cout << fmt::format("MetaTool: session finished. Time taken: {:.3f}s", 
			m_RunStatistics.session_timer.ElapsedMilliseconds() / 1000.0f);
	}

	void MetaTool::LoadParseTargets() {
		// Load parse targets
		std::ifstream parseTargetsStream(m_WorkingDir / "ParseTargets.txt");
		std::string parseTarget;

		while (std::getline(parseTargetsStream, parseTarget)) {
			m_ParseTargets.push_back(parseTarget);
		}

		// Exit if no parse targets
		if (m_ParseTargets.empty()) {
			exit(0);
		}
	}

	void MetaTool::CreateDirectories() {
		std::filesystem::create_directories(m_WorkingDir / "DummyOut");
		std::filesystem::create_directories(m_WorkingDir / "Cache" / "Modules");
		std::filesystem::create_directories(m_WorkingDir / "Cache" / "TypeModules");
		std::filesystem::create_directories(m_OutputDir / "Gen");
	}

}