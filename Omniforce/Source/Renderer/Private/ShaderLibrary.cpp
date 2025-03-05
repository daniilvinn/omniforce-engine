#include <Foundation/Common.h>
#include <Renderer/ShaderLibrary.h>

#include <Renderer/ShaderCompiler.h>
#include <Renderer/Renderer.h>
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
		ShaderCompiler shader_compiler;
		ShaderCompilationResult compilation_result;

		OMNIFORCE_ASSERT_TAGGED(macros.contains("__OMNI_PIPELINE_LOCAL_HASH"), "No pipeline ID macro provided");

		for (auto& global_macro : m_GlobalMacros)
			shader_compiler.AddGlobalMacro(global_macro.first, global_macro.second);

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

		if (path.extension() == ".slang")
		{
			compilation_result = LoadSlangShader(path, shader_compiler);
		}
		else {
			std::string shader_source;
			std::string line;
			std::ifstream input_stream(path);
			while (std::getline(input_stream, line)) shader_source.append(line + '\n');

			compilation_result = shader_compiler.Compile(shader_source, path.filename().string(), macros);
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
			
			std::string& permutation_id = permutation_macro_table.at("__OMNI_PIPELINE_LOCAL_HASH");
			
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
	
	ShaderCompilationResult ShaderLibrary::LoadSlangShader(const std::filesystem::path& path, ShaderCompiler& compiler)
	{
		compiler.LoadModule(path);

		ByteArray vertex_shader_code = compiler.GetEntryPointCode(&g_PersistentAllocator, fmt::format("{}.{}", path.stem().string(), "VertexStage"));
		ByteArray fragment_shader_code = compiler.GetEntryPointCode(&g_PersistentAllocator, fmt::format("{}.{}", path.stem().string(), "FragmentStage"));

		std::vector<uint32> vertex_bytecode(vertex_shader_code.Size() / 4);
		std::vector<uint32> fragment_bytecode(fragment_shader_code.Size() / 4);

		memcpy(vertex_bytecode.data(), vertex_shader_code.Raw(), vertex_shader_code.Size());
		memcpy(fragment_bytecode.data(), fragment_shader_code.Raw(), fragment_shader_code.Size());

		ShaderCompilationResult result = {};
		result.valid = vertex_shader_code.Size() && fragment_shader_code.Size();

		result.bytecode.emplace(ShaderStage::VERTEX, std::move(vertex_bytecode));
		result.bytecode.emplace(ShaderStage::FRAGMENT, std::move(fragment_bytecode));

		return result;
	}

}