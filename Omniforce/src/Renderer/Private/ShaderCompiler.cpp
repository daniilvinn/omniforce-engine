#include "../ShaderCompiler.h"

#include <Filesystem/Filesystem.h>
#include <Log/Logger.h>

#include <shaderc/shaderc.hpp>
#include <chrono>

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
		shaderc::Compiler m_Compiler;

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

}