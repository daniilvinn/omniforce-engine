#include <Foundation/Common.h>
#include <RHI/ShaderLibrary.h>

#include <RHI/ShaderCompiler.h>
#include <RHI/Renderer.h>
#include <Filesystem/Filesystem.h>
#include <Threading/JobSystem.h>

namespace Omni {

	ShaderLibrary::ShaderLibrary()
	{
		m_GlobalMacros.push_back({ "_OMNI_SCENE_DESCRIPTOR_SET", "0" });
		m_GlobalMacros.push_back({ "_OMNI_PASS_DESCRIPTOR_SET", "1" });
		m_GlobalMacros.push_back({ "_OMNI_DRAW_CALL_DESCRIPTOR_SET", "2" });
		m_GlobalMacros.push_back({ "__OMNI_TASK_SHADER_PREFERRED_WORK_GROUP_SIZE", std::to_string(Renderer::GetDeviceOptimalTaskWorkGroupSize()) });
		m_GlobalMacros.push_back({ "__OMNI_MESH_SHADER_PREFERRED_WORK_GROUP_SIZE", std::to_string(Renderer::GetDeviceOptimalMeshWorkGroupSize()) });
		m_GlobalMacros.push_back({ "__OMNI_COMPILE_SHADER_FOR_GLSL", "" });
		m_GlobalMacros.shrink_to_fit();

		g_ShaderCompiler = CreatePtr<ShaderCompiler>(&g_PersistentAllocator);

		for (auto& global_macro : m_GlobalMacros)
			g_ShaderCompiler->AddGlobalMacro(global_macro.first, global_macro.second);

		g_ShaderCompiler->LoadModule("Resources/Shaders/GlobalParameters.slang");
		m_GlobalBindings = g_ShaderCompiler->GetDescriptorSetLayout("GlobalParameters");;
	}

	ShaderLibrary::~ShaderLibrary()
	{
		for (auto& [key, permutation_map] : m_Library)
			for(auto& [shader, macro_table] : permutation_map )
				shader->Destroy();

		m_Library.clear();
	}

	void ShaderLibrary::Init()
	{
		s_Instance = new ShaderLibrary;
	}

	void ShaderLibrary::Destroy()
	{
		delete s_Instance;
	}

	bool ShaderLibrary::LoadShader(const std::filesystem::path& path, const ShaderMacroTable& macros) {
		ShaderCompilationResult compilation_result;

		OMNIFORCE_ASSERT_TAGGED(macros.contains("__OMNI_PIPELINE_LOCAL_HASH"), "No pipeline ID macro provided");

		bool result = HasShader(path.filename().string(), macros);

		if (result)
		{
			OMNIFORCE_CORE_WARNING("Shader \"{0}\" is already loaded.", path.filename().string());
			return true;
		}

		result = FileSystem::CheckDirectory(path);

		if (!result)
		{
			OMNIFORCE_CORE_ERROR("Shader directory not found: {0}", path.string());
			return false;
		}
		else {
			std::string shader_source;
			std::string line;
			std::ifstream input_stream(path);
			while (std::getline(input_stream, line)) shader_source.append(line + '\n');

			compilation_result = g_ShaderCompiler->Compile(shader_source, path.filename().string(), macros);
		}

		if (!compilation_result.valid) return false;

		Ref<Shader> shader = Shader::Create(&g_PersistentAllocator, compilation_result.bytecode, path);

		m_Mutex.lock();
		std::string shader_filename = path.filename().string();

		if (m_Library.contains(shader_filename)) {
			m_Library[shader_filename].push_back({ shader, macros });
		}
		else {
			std::vector<std::pair<Ref<Shader>, ShaderMacroTable>> permutations;
			permutations.push_back({ shader, macros });
			m_Library.emplace(shader_filename, permutations);
		}
		m_Mutex.unlock();

		return true;
	}

	bool ShaderLibrary::LoadShader2(const std::string& name, std::vector<std::string> entry_point_names, const ShaderMacroTable& macros /*= {}*/)
	{
		ShaderCompilationResult compilation_result = {};
		compilation_result.valid = true;

		for (const auto& entry_point_full_name : entry_point_names) {
			uint64 separator_location = entry_point_full_name.find('.');

			std::string module_name = entry_point_full_name.substr(0, separator_location);
			std::string entry_point_name = entry_point_full_name.substr(separator_location + 1);

			std::filesystem::path path = "Resources/Shaders/" + module_name + ".slang";

			for (auto& global_macro : m_GlobalMacros)
				g_ShaderCompiler->AddGlobalMacro(global_macro.first, global_macro.second);

			PerformShaderLoadingChecks(name, path, macros);

			EntryPointCompilationOptions compilation_options = {};
			compilation_options.entry_point_name = entry_point_name;
			compilation_options.module_name = module_name;
			compilation_options.path = path;

			ShaderEntryPointCode compiled_entry_point = LoadShaderEntryPoint(compilation_options, *g_ShaderCompiler);

			std::vector<uint32> bytecode(compiled_entry_point.bytecode.Size() / 4);
			memcpy(bytecode.data(), compiled_entry_point.bytecode.Raw(), compiled_entry_point.bytecode.Size());

			compilation_result.valid &= compiled_entry_point.valid;
			compilation_result.bytecode.emplace(compiled_entry_point.stage, bytecode);
		}
		if (!compilation_result.valid) return false;

		Ref<Shader> shader = Shader::Create(&g_PersistentAllocator, compilation_result.bytecode, "");

		m_Mutex.lock();

		if (m_Library.contains(name)) {
			m_Library[name].push_back({ shader, macros });
		}
		else {
			std::vector<std::pair<Ref<Shader>, ShaderMacroTable>> permutations;
			permutations.push_back({ shader, macros });
			m_Library.emplace(name, permutations);
		}
		m_Mutex.unlock();

		return true;
	}

	bool ShaderLibrary::UnloadShader(std::string name, const ShaderMacroTable& macros)
	{
		if (m_Library.find(name) == m_Library.end())
			return false;

		auto& permutation_table = m_Library.at(name);

		// Delete specific permutation
		uint32 idx = 0;
		bool permutation_found = false;
		for (auto& [shader, permutation] : permutation_table) {
			if (permutation == macros) {
				shader->Destroy();
				permutation_table.erase(permutation_table.begin() + idx);
				return true;
			}
			idx++;
		}

		// Shader found, but didn't find specific permutation
		if (idx == permutation_table.size())
			return false;

		// If there's no permutations of a specific shader left, delete entry from library.
		if(!m_Library.at(name).size())
			m_Library.erase(name);

		return true;
	}

	bool ShaderLibrary::ReloadShader(std::filesystem::path name)
	{
		return false;
	}

	bool ShaderLibrary::HasShader(std::string key, const ShaderMacroTable& macros /*= {}*/)
	{
		if (!m_Library.contains(key))
			return false;

		auto& permutation_list = m_Library.at(key);

		for (auto& permutation : permutation_list) {
			if (permutation.second == macros)
				return true;
		}

		return false;
	}

	Omni::Ref<Omni::Shader> ShaderLibrary::GetShader(std::string key, ShaderMacroTable macros) {
		if (!m_Library.contains(key))
			return nullptr;

		auto& permutation_list = m_Library.at(key);

		for (auto& permutation : permutation_list) {
			auto& permutation_macro_table = permutation.second;
			
			std::string& permutation_id = permutation_macro_table["__OMNI_PIPELINE_LOCAL_HASH"];
			
			// Emplace an id so if everything other than ID matches, then we can freely return this shader
			macros["__OMNI_PIPELINE_LOCAL_HASH"] = permutation_id;

			if (permutation_macro_table == macros)
				return permutation.first;
			else
				macros.erase("__OMNI_PIPELINE_LOCAL_HASH");
		}

		return nullptr;
	}

	ShaderStage ShaderLibrary::EvaluateStage(std::filesystem::path file) const
	{
		if (file.extension().string() == ".vert")
			return ShaderStage::VERTEX;
		if (file.extension().string() == ".frag")
			return ShaderStage::FRAGMENT;
		if (file.extension().string() == ".comp")
			return ShaderStage::COMPUTE;

		return ShaderStage::UNKNOWN;
	}
	
	bool ShaderLibrary::PerformShaderLoadingChecks(const std::string& name, const std::filesystem::path& path, const ShaderMacroTable& macros)
	{
		bool result = HasShader(name, macros);

		if (result)
		{
			OMNIFORCE_CORE_WARNING("Shader \"{0}\" is already loaded.", name);
			return true;
		}

		result = FileSystem::CheckDirectory(path);

		if (!result)
		{
			OMNIFORCE_CORE_ERROR("Shader directory not found: {0}", path.string());
			return false;
		}

		return true;
	}

	ShaderEntryPointCode ShaderLibrary::LoadShaderEntryPoint(const EntryPointCompilationOptions& options, ShaderCompiler& compiler)
	{
		compiler.LoadModule(options.path);
		ShaderEntryPointCode entry_point_code = compiler.GetEntryPointCode(&g_PersistentAllocator, fmt::format("{}.{}", options.module_name, options.entry_point_name));
		return entry_point_code;
	}

}