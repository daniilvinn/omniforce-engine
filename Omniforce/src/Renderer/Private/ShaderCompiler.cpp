#include "../ShaderCompiler.h"

#include <Filesystem/Filesystem.h>
#include <Log/Logger.h>
#include "ShaderSourceIncluder.h"

#include <shaderc/shaderc.hpp>

#include <fstream>

namespace Omni {

	shaderc_shader_kind convert(const ShaderStage& stage) {
		switch (stage)
		{
		case ShaderStage::VERTEX:		return shaderc_vertex_shader;
		case ShaderStage::FRAGMENT:		return shaderc_fragment_shader;
		case ShaderStage::COMPUTE:		return shaderc_compute_shader;
		default:						std::unreachable();
		}
	}

	ShaderCompiler::ShaderCompiler()
	{
		if(OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG)
			m_GlobalOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
		else
			m_GlobalOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

		m_GlobalOptions.SetIncluder(std::make_unique<ShaderSourceIncluder>());
	}

	bool ShaderCompiler::ReadShaderFile(std::filesystem::path path, std::stringstream* out)
	{
		Shared<File> file = FileSystem::ReadFile(path, 0);
		if (!file->GetData() && file->GetSize())
			return false;

		out->write((const char*)file->GetData(), file->GetSize());
		file->Release();

		return true;
	}

	void ShaderCompiler::AddGlobalMacro(const std::string& key, const std::string& value)
	{
		m_GlobalMacros.emplace(key, value);
		m_GlobalOptions.AddMacroDefinition(key, value);
	}

	ShaderCompilationResult ShaderCompiler::Compile(std::string& source, const std::string& filename)
	{
		ShaderCompilationResult compilation_result = { .valid = true };
		shaderc::CompileOptions local_options = m_GlobalOptions;

		std::map<ShaderStage, std::string> separated_sources;
		std::map<ShaderStage, std::vector<uint32>> binaries;

		std::istringstream input_stream(source);
		std::string current_parsing_line;
		uint32 current_line_number = 1;
		ShaderStage current_parsing_stage = ShaderStage::UNKNOWN;

		bool pragma_lang_found = false;

		// Parse and preprocess stage
		while (std::getline(input_stream, current_parsing_line)) {
			if (current_parsing_line.find("#pragma") != std::string::npos) {
				if (current_parsing_line.find(" lang") != std::string::npos) {
					if (current_parsing_line.find("glsl") != std::string::npos) {
						local_options.SetSourceLanguage(shaderc_source_language_glsl);
						pragma_lang_found = true;
					}
					else if (current_parsing_line.find("hlsl") != std::string::npos) {
						local_options.SetSourceLanguage(shaderc_source_language_hlsl);
						pragma_lang_found = true;
					}
				}
				else if (current_parsing_line.find("stage") != std::string::npos) {
					if (current_parsing_line.find("vertex") != std::string::npos) {
						current_parsing_stage = ShaderStage::VERTEX;
						// \n to ensure line similarity between shader file and actual parsed source
						separated_sources.emplace(ShaderStage::VERTEX, "\n"); 
					}
					else if (current_parsing_line.find("fragment") != std::string::npos) {
						current_parsing_stage = ShaderStage::FRAGMENT;
						separated_sources.emplace(ShaderStage::FRAGMENT, "\n");
					}
					else if (current_parsing_line.find("compute") != std::string::npos) {
						current_parsing_stage = ShaderStage::COMPUTE;
						separated_sources.emplace(ShaderStage::COMPUTE, "\n");
					}
					else {
						compilation_result.valid = false;
						OMNIFORCE_CORE_ERROR("Failed to parse shader, invalid stage at line {0}", current_line_number);
						return compilation_result;
					}
				}
				else {
					compilation_result.valid = false;
					OMNIFORCE_CORE_ERROR("Failed to parse shader, invalid #pragma at line {0}", current_line_number);
					return compilation_result;
				}
			}
			else {
				separated_sources[current_parsing_stage].append(current_parsing_line + '\n');
				current_line_number++;
			}
		};

		if (!pragma_lang_found) {
			OMNIFORCE_CORE_ERROR("#pragma lang in {0} was not found", filename);
			compilation_result.valid = false;
			return compilation_result;
		}

		// Compilation stage
		for (auto& stage_source : separated_sources) {
			shaderc::PreprocessedSourceCompilationResult preprocessing_result = m_Compiler.PreprocessGlsl(
				stage_source.second,
				convert(stage_source.first),
				filename.c_str(),
				local_options
			);

			if (preprocessing_result.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success) {
				OMNIFORCE_CORE_ERROR("Failed to preprocess shader {0}.{1}: {2}", filename, Utils::StageToString(stage_source.first), preprocessing_result.GetErrorMessage());
			}

			stage_source.second = std::string(preprocessing_result.begin());

			shaderc::CompilationResult result = m_Compiler.CompileGlslToSpv(stage_source.second, convert(stage_source.first), filename.c_str(), local_options);
			if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
				OMNIFORCE_CORE_ERROR("Failed to compile shader {0}.{1}:\n{2}", filename, Utils::StageToString(stage_source.first), result.GetErrorMessage());
				compilation_result.valid = false;
			}

			std::vector<uint32> stage_compilation_result(result.begin(), result.end());
			binaries.emplace(stage_source.first, std::move(stage_compilation_result));
		}

		compilation_result.bytecode = std::move(binaries);
		

		return compilation_result;
	}
}