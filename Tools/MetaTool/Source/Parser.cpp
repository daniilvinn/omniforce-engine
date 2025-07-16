#include "Parser.h"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace Omni {

	Parser::Parser(const std::filesystem::path& workingDir, uint32_t numThreads, CacheManager* cacheManager)
		: m_WorkingDir(workingDir)
		, m_NumThreads(numThreads)
		, m_TargetsGenerated(0)
		, m_TargetsSkipped(0)
		, m_CacheManager(cacheManager)
	{
		SetupParserArgs();
		SetupIndexes();
	}

	Parser::~Parser() {
		for (auto& index : m_Indexes) {
			clang_disposeIndex(index);
		}
	}

	std::unordered_map<std::string, nlohmann::ordered_json> Parser::ParseTargets(const std::vector<std::filesystem::path>& parseTargets) {
		std::vector<std::thread> threads(m_NumThreads);

		for (uint32_t threadIdx = 0; threadIdx < m_NumThreads; threadIdx++) {
			// Setup task parameters
			uint32_t numTuForThread = parseTargets.size() / m_NumThreads;
			uint32_t numTuRemainder = parseTargets.size() % m_NumThreads;

			uint32_t startTargetIndex = threadIdx * numTuForThread + std::min(threadIdx, numTuRemainder);
			uint32_t endTargetIndex = startTargetIndex + numTuForThread + (threadIdx < numTuRemainder ? 1 : 0);

			// Kick a job for each hardware thread
			threads[threadIdx] = std::thread([this, threadIdx, startTargetIndex, endTargetIndex, &parseTargets]() {
				ParseTargetsWorker(threadIdx, startTargetIndex, endTargetIndex, parseTargets);
			});
		}

		// Wait for parsing jobs
		for (auto& thread : threads) {
			thread.join();
		}

		return m_GeneratedDataCache;
	}

	void Parser::ParseTargetsWorker(uint32_t threadIdx, uint32_t startTargetIndex, uint32_t endTargetIndex, 
								   const std::vector<std::filesystem::path>& parseTargets) {
		// Kill redundant threads
		if (parseTargets.size() < m_NumThreads) {
			if (threadIdx >= parseTargets.size()) {
				return;
			}
		}

		for (uint32_t i = startTargetIndex; i < endTargetIndex; i++) {
			std::filesystem::path tuPath = parseTargets[i];
			std::string targetFilename = std::filesystem::path(tuPath).filename().string();

			// Parse translation unit
			CXTranslationUnit tu = ParseTranslationUnit(tuPath, threadIdx);
			if (!tu) {
				continue;
			}

			// Traverse AST
			ASTVisitor visitor;
			nlohmann::ordered_json tuParsingResult = visitor.TraverseTranslationUnit(tu);

			// Check if code generation is needed
			std::string validationTargetFilename = tuPath.filename().string();
			bool shouldRegenerate = m_CacheManager->ShouldRegenerateTarget(tuPath.string(), tuParsingResult);

			if (shouldRegenerate) {
				std::lock_guard<std::mutex> lock(m_CacheMutex);
				m_GeneratedDataCache[tuPath.string()] = tuParsingResult;
				m_TargetsGenerated++;
			} else {
				m_TargetsSkipped++;
			}

			clang_disposeTranslationUnit(tu);
		}
	}

	CXTranslationUnit Parser::ParseTranslationUnit(const std::filesystem::path& tuPath, uint32_t threadIdx) {
		CXTranslationUnit tu = nullptr;

		CXErrorCode errorCode = clang_parseTranslationUnit2(
			m_Indexes[threadIdx],
			tuPath.string().c_str(),
			m_ParserArgs.data(),
			m_ParserArgs.size(),
			nullptr,
			0,
			CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_PrecompiledPreamble | CXTranslationUnit_IncludeAttributedTypes,
			&tu
		);

		if (errorCode != CXError_Success) {
			std::cerr << "Failed to run MetaTool; error code: " << errorCode << std::endl;
			exit(-2);
		}

		ValidateTranslationUnit(tu);
		return tu;
	}

	void Parser::ValidateTranslationUnit(CXTranslationUnit translationUnit) {
		int numDiagnosticMessages = clang_getNumDiagnostics(translationUnit);

		if (numDiagnosticMessages) {
			std::cout << "There are " << numDiagnosticMessages << " MetaTool diagnostic messages" << std::endl;
		}

		bool foundError = false;

		for (uint32_t currentMessage = 0; currentMessage < numDiagnosticMessages; ++currentMessage) {
			CXDiagnostic diagnostic = clang_getDiagnostic(translationUnit, currentMessage);
			CXString errorString = clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions());

			std::string tmp(clang_getCString(errorString));
			clang_disposeString(errorString);

			if (tmp.find("error") != std::string::npos) {
				foundError = true;
			}

			std::cerr << tmp << std::endl;
		}

		if (foundError) {
			std::cerr << "Failed to run MetaTool" << std::endl;
			exit(-1);
		}
	}

	void Parser::SetupParserArgs() {
		m_ParserArgs = {
			CLANG_PARSER_ARGS
		};

		// Erase empty compiler definitions
		std::erase_if(m_ParserArgs, [](const char* arg) {
			return std::string(arg) == std::string("-D");
		});
	}

	void Parser::SetupIndexes() {
		m_Indexes.resize(m_NumThreads);

		for (auto& index : m_Indexes) {
			index = clang_createIndex(0, 0);
		}
	}

} 