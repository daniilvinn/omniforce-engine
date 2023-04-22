#pragma once

#include "RendererCommon.h"
#include <sstream>
#include <filesystem>
#include <map>
#include <shaderc/shaderc.hpp>

#include "Shader.h"

namespace Omni {

	struct OMNIFORCE_API ShaderCompilationResult {
		std::map<ShaderStage, std::vector<uint32>> bytecode;
		bool valid;

	};

	class OMNIFORCE_API ShaderCompiler {
	public:
		ShaderCompiler();

		/*
		*  @brief reads shader file.
		*  @return true if successful, otherwise false (e.g. file not present)
		*  @param [out] std::stringstream* out - a pointer to stringstream to write source code to.
		*/
		bool ReadShaderFile(std::filesystem::path path, std::stringstream* out);
		bool Compile(const std::stringstream& source, const ShaderStage& stage, std::string_view filename, std::vector<uint32>* out);
		void AddGlobalMacro(const std::string& key, const std::string& value);

		ShaderCompilationResult Compile(std::string& source, const std::string& filename);

	private:
		shaderc::Compiler m_Compiler;
		shaderc::CompileOptions m_GlobalOptions;
		std::map<std::string, std::string> m_GlobalMacros;

	};

}