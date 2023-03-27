#pragma once

#include "RendererCommon.h"
#include <sstream>
#include <filesystem>
#include <shaderc/shaderc.hpp>

#include "Shader.h"

namespace Omni {

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

	private:
		
	};

}