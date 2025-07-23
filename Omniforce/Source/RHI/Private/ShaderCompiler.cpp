#include <Foundation/Common.h>
#include <RHI/ShaderCompiler.h>

#include <Filesystem/Filesystem.h>
#include <RHI/Private/ShaderSourceIncluder.h>
#include <Core/Utils.h>

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

	ShaderStage convert(const SlangStage stage) {
		switch (stage)
		{
		case SLANG_STAGE_VERTEX:				return ShaderStage::VERTEX;
		case SLANG_STAGE_FRAGMENT:				return ShaderStage::FRAGMENT;
		case SLANG_STAGE_MESH:					return ShaderStage::MESH;
		case SLANG_STAGE_AMPLIFICATION:			return ShaderStage::TASK;
		case SLANG_STAGE_COMPUTE:				return ShaderStage::COMPUTE;
		case SLANG_STAGE_RAY_GENERATION:		return ShaderStage::RAYGEN;
		case SLANG_STAGE_CLOSEST_HIT:			return ShaderStage::CLOSESTHIT;
		case SLANG_STAGE_ANY_HIT:				return ShaderStage::ANYHIT;
		case SLANG_STAGE_MISS:					return ShaderStage::MISS;
		case SLANG_STAGE_INTERSECTION:			return ShaderStage::INTERSECTION;
		case SLANG_STAGE_CALLABLE:				return ShaderStage::CALLABLE;
		case SLANG_STAGE_NONE:					return ShaderStage::UNKNOWN;
		default:								std::unreachable();
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

		if(m_GlobalSession == nullptr && m_LocalSession == nullptr) {
			Timer timer;
			SlangResult slang_result = slang::createGlobalSession(m_GlobalSession.writeRef());
			OMNIFORCE_ASSERT_TAGGED(SLANG_SUCCEEDED(slang_result), "Failed to create shader compiler global session");

			OMNIFORCE_CORE_INFO("Created shader compiler global session. Taken time: {}s", timer.ElapsedMilliseconds() / 1000.0f);

			// Create local session
			slang::TargetDesc target_description = {};
			target_description.format = SLANG_SPIRV;
			target_description.profile = m_GlobalSession->findProfile("sm_6_8");
			target_description.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
			target_description.forceGLSLScalarBufferLayout = true;

			const char* IncludeDir[] = { "Resources/Shaders" };

			slang::SessionDesc session_description = {};
			session_description.targets = &target_description;
			session_description.targetCount = 1;
			session_description.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
			session_description.searchPaths = IncludeDir;
			session_description.searchPathCount = 1;

			m_GlobalSession->createSession(session_description, m_LocalSession.writeRef());
		}
	}

	void ShaderCompiler::Init()
	{
		g_ShaderCompiler = CreatePtr<ShaderCompiler>(&g_PersistentAllocator);
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

	bool ShaderCompiler::LoadModule(const stdfs::path& path, const ShaderMacroTable& macros /*= {}*/)
	{
		std::string path_string = path.string();

		// Load code
		slang::IModule* slang_module = nullptr;
		{
			Slang::ComPtr<slang::IBlob> diagnostics_blob;
			slang_module = m_LocalSession->loadModule(path_string.c_str(), diagnostics_blob.writeRef());

			uint32 x = m_LocalSession->getLoadedModuleCount();

			if (!slang_module) {
				return ValidateSlangResult(-1, diagnostics_blob);
			}
		}
		
		m_LoadedModules.emplace(path.stem().string().c_str(), slang_module);
		return true;
	}

	ShaderEntryPointCode ShaderCompiler::GetEntryPointCode(IAllocator* allocator, const std::string& entry_point_full_name)
	{
		ShaderEntryPointCode compilation_result = {};
		compilation_result.valid = false;
		compilation_result.stage = ShaderStage::UNKNOWN;

		// Split entry point name;
		// Entry point name must in this format: SWRaster.EntryScanline
		// Where `SWRaster` is a module name and `EntryScanline` is an entrypoint
		Array<std::string> module_and_ep = Utils::SplitString(allocator, entry_point_full_name, '.');
		OMNIFORCE_ASSERT_TAGGED(module_and_ep.Size() == 2, "Failed to split entry point name");

		ByteArray output_code(allocator);

		// Check if module is loaded
		std::string& module_name = module_and_ep[0];
		std::string& entry_point_name = module_and_ep[1];
		OMNIFORCE_ASSERT_TAGGED(m_LoadedModules.contains(module_name), "Module is not loaded");

		// Find entry point by its name in found module
		slang::IModule* slang_module = m_LoadedModules.at(module_and_ep[0]);

		Slang::ComPtr<slang::IEntryPoint> entry_point;
		slang_module->findEntryPointByName(entry_point_name.c_str(), entry_point.writeRef());

		Slang::ComPtr<slang::IComponentType> composed_program;
		{
			Slang::ComPtr<slang::IBlob> diagnostics;
			SlangResult result = entry_point->link(composed_program.writeRef(), diagnostics.writeRef());

			if (!ValidateSlangResult(result, diagnostics)) {
				return compilation_result;
			}
		}

		SlangStage stage = composed_program->getLayout()->findEntryPointByName(entry_point_name.c_str())->getStage();

		// Generate code
		Slang::ComPtr<slang::IBlob> spirv_code;
		{
			Slang::ComPtr<slang::IBlob> diagnostics;
			SlangResult result = composed_program->getEntryPointCode(
				0,
				0,
				spirv_code.writeRef(),
				diagnostics.writeRef());

			if (!ValidateSlangResult(result, diagnostics)) {
				return compilation_result;
			}
		}

		output_code.Resize(spirv_code->getBufferSize());
		memcpy(output_code.Raw(), spirv_code->getBufferPointer(), output_code.Size());

		compilation_result.bytecode = std::move(output_code);
		compilation_result.stage = convert(composed_program->getLayout()->getEntryPointByIndex(0)->getStage());
		compilation_result.valid = true;

		return compilation_result;
	}

	std::vector<DescriptorBinding> ShaderCompiler::GetDescriptorSetLayout(const std::string& module_name)
	{
		std::vector<DescriptorBinding> layout;
		slang::IModule* slang_module = m_LoadedModules.at(module_name);

		slang::ProgramLayout* slang_layout = slang_module->getLayout();

		auto ConvertSlangDescriptorType = [](slang::BindingType type) -> DescriptorBindingType {
			switch (type)
			{
			case slang::BindingType::CombinedTextureSampler:				return DescriptorBindingType::SAMPLED_IMAGE;
			case slang::BindingType::RayTracingAccelerationStructure:		return DescriptorBindingType::ACCELERATION_STRUCTURE;
			case slang::BindingType::MutableTexture:						return DescriptorBindingType::STORAGE_IMAGE;
			}

			OMNIFORCE_ASSERT_TAGGED(false, "Not implemented fully");

		};

		for (uint32 i = 0; i < slang_layout->getParameterCount(); i++) {
			DescriptorBinding binding = {};
			slang::VariableLayoutReflection* parameter = slang_layout->getParameterByIndex(i);
			slang::TypeLayoutReflection* type_layout = parameter->getTypeLayout();

			binding.binding = parameter->getBindingIndex();
			binding.type = ConvertSlangDescriptorType(type_layout->getBindingRangeType(0));
			binding.array_count = std::max(type_layout->getTotalArrayElementCount(), 1ull);
			binding.flags = binding.type == DescriptorBindingType::SAMPLED_IMAGE && binding.array_count != 1 ? (BitMask)DescriptorFlags::PARTIALLY_BOUND : 0;

			layout.push_back(binding);
		}

		return layout;
	}

	bool ShaderCompiler::ValidateSlangResult(SlangResult result, Slang::ComPtr<slang::IBlob>& diagnosticsBlob)
	{
		bool result_success = SLANG_SUCCEEDED(result);
		if (!result_success) {
			OMNIFORCE_CORE_ERROR("Shader compiler error: {}", (const char*)diagnosticsBlob->getBufferPointer());
		}
		return result_success;
	}

}