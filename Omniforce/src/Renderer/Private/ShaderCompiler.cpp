#include "../ShaderCompiler.h"

#include <Filesystem/Filesystem.h>
#include <Log/Logger.h>

#include <shaderc/shaderc.hpp>

#include <fstream>
#include <chrono>
#include <any>

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

	std::string StageToString(const ShaderStage& stage) {
		switch (stage)
		{
		case ShaderStage::VERTEX:		return "vertex";
		case ShaderStage::FRAGMENT:		return "fragment";
		case ShaderStage::COMPUTE:		return "compute";
		case ShaderStage::UNKNOWN:		return "unknown";
		}
	}

	ShaderCompiler::ShaderCompiler()
	{
		//m_Options.SetIncluder();
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

	bool ShaderCompiler::Compile(const std::stringstream& source, const ShaderStage& stage, std::string_view filename, std::vector<uint32>* out)
	{
		shaderc::CompileOptions options;
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		options.SetSourceLanguage(shaderc_source_language_glsl);

		shaderc::CompilationResult result = m_Compiler.CompileGlslToSpv(
			source.str(),
			convert(stage),
			filename.data(),
			options
		);

		if (result.GetCompilationStatus() != shaderc_compilation_status_success) 
		{
			OMNIFORCE_CORE_ERROR("Failed to compile shader ({0}):", filename.data());
			for (int32 i = 0; i < result.GetNumErrors(); i++)
				OMNIFORCE_CORE_ERROR("{0}", result.GetErrorMessage());
			return false;
		}

		std::vector<uint32> binary_data(result.begin(), result.end());

		*out = std::move(binary_data);

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

		for (auto& stage : separated_sources) {
			OMNIFORCE_CORE_WARNING("{0}\n\n", stage.second);
		}

		// Compilation stage
		for (auto& stage_source : separated_sources) {
			shaderc::CompilationResult result = m_Compiler.CompileGlslToSpv(stage_source.second, convert(stage_source.first), filename.c_str());
			if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
				OMNIFORCE_CORE_ERROR("Failed to compile shader {0}.{1}:\n{2}", filename, StageToString(stage_source.first), result.GetErrorMessage());
				compilation_result.valid = false;
			}

			std::vector<uint32> stage_compilation_result(result.begin(), result.end());
			binaries.emplace(stage_source.first, std::move(stage_compilation_result));
		}

		compilation_result.bytecode = std::move(binaries);
			
		return compilation_result;
	}
}