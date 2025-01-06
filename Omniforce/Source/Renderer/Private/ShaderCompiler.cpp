#include <Foundation/Common.h>
#include <Renderer/ShaderCompiler.h>

#include <Filesystem/Filesystem.h>
#include <Renderer/Private/ShaderSourceIncluder.h>

#include <shaderc/shaderc.hpp>

namespace Omni {

	shaderc_shader_kind convert(const ShaderStage& stage) {
		switch (stage)
		{
		case ShaderStage::VERTEX:		return shaderc_vertex_shader;
		case ShaderStage::FRAGMENT:		return shaderc_fragment_shader;
		case ShaderStage::COMPUTE:		return shaderc_compute_shader;
		case ShaderStage::TASK:			return shaderc_task_shader;
		case ShaderStage::MESH:			return shaderc_mesh_shader;
		default:						std::unreachable();
		}
	}

	ShaderCompiler::ShaderCompiler()
	{
		if(OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG)
			m_GlobalOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
		else
			m_GlobalOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

		const std::vector<std::string> system_include_paths = {
			"Resources/Shaders"
		};

		m_GlobalOptions.SetIncluder(std::make_unique<ShaderSourceIncluder>(system_include_paths));
		m_GlobalOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		m_GlobalOptions.SetTargetSpirv(shaderc_spirv_version_1_6);
		m_GlobalOptions.SetPreserveBindings(true);
	}

	bool ShaderCompiler::ReadShaderFile(std::filesystem::path path, std::stringstream* out)
	{
		Ref<File> file = FileSystem::ReadFile(&g_TransientAllocator, path, 0);
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

	ShaderCompilationResult ShaderCompiler::Compile(std::string& source, const std::string& filename, const ShaderMacroTable& macros)
	{
		OMNIFORCE_CORE_INFO("Compiling shader {}", filename);

		ShaderCompilationResult compilation_result = { .valid = true };
		shaderc::CompileOptions local_options = m_GlobalOptions;

		if (OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG) {
			local_options.SetGenerateDebugInfo();
			//local_options.SetOptimizationLevel(shaderc_optimization_level_zero);
		}

		for (auto& macro : macros)
			local_options.AddMacroDefinition(macro.first, macro.second);

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
					}
					else if (current_parsing_line.find("fragment") != std::string::npos) {
						current_parsing_stage = ShaderStage::FRAGMENT;
					}
					else if (current_parsing_line.find("compute") != std::string::npos) {
						current_parsing_stage = ShaderStage::COMPUTE;
					}
					else if (current_parsing_line.find("task") != std::string::npos) {
						current_parsing_stage = ShaderStage::TASK;
					}
					else if (current_parsing_line.find("mesh") != std::string::npos) {
						current_parsing_stage = ShaderStage::MESH;
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
				if(current_parsing_line != "\n" || current_parsing_line != "")
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
				OMNIFORCE_CORE_ERROR("Failed to preprocess shader {0}.{1}: {2}", filename, Utils::ShaderStageToString(stage_source.first), preprocessing_result.GetErrorMessage());
				compilation_result.valid = false;
				continue;
			}

			stage_source.second = std::string(preprocessing_result.begin());

			shaderc::CompilationResult result = m_Compiler.CompileGlslToSpv(stage_source.second, convert(stage_source.first), filename.c_str(), local_options);
			if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
				OMNIFORCE_CORE_ERROR("Failed to compile shader {0}.{1}:\n{2}", filename, Utils::ShaderStageToString(stage_source.first), result.GetErrorMessage());
				compilation_result.valid = false;
			}

			std::vector<uint32> stage_compilation_result(result.begin(), result.end());
			binaries.emplace(stage_source.first, std::move(stage_compilation_result));
		}

		compilation_result.bytecode = std::move(binaries);
		OMNIFORCE_ASSERT_TAGGED(compilation_result.valid, "Shader compilation failed");

		return compilation_result;
	}
}