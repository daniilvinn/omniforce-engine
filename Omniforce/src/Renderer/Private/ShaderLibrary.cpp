#include "../ShaderLibrary.h"

#include <Log/Logger.h>
#include <Filesystem/Filesystem.h>
#include <Threading/JobSystem.h>
#include "../ShaderCompiler.h"
#include "../Renderer.h"

#include <fstream>

namespace Omni {

	ShaderLibrary::ShaderLibrary()
	{
		m_GlobalMacros.push_back({ "_OMNI_SCENE_DESCRIPTOR_SET", "0" });
		m_GlobalMacros.push_back({ "_OMNI_PASS_DESCRIPTOR_SET", "1" });
		m_GlobalMacros.push_back({ "_OMNI_DRAW_CALL_DESCRIPTOR_SET", "2" });
		m_GlobalMacros.push_back({ "__OMNI_TASK_SHADER_PREFERRED_WORK_GROUP_SIZE", std::to_string(Renderer::GetDeviceOptimalTaskWorkGroupSize()) });
		m_GlobalMacros.push_back({ "__OMNI_MESH_SHADER_PREFERRED_WORK_GROUP_SIZE", std::to_string(Renderer::GetDeviceOptimalMeshWorkGroupSize()) });
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

		std::string shader_source;
		std::string line;
		std::ifstream input_stream(path);
		while (std::getline(input_stream, line)) shader_source.append(line + '\n');

		ShaderCompilationResult compilation_result = shader_compiler.Compile(shader_source, path.filename().string(), macros);

		if (!compilation_result.valid) return false;

		Shared<Shader> shader = Shader::Create(compilation_result.bytecode, path);

		m_Mutex.lock();
		auto shader_filename = path.filename().string();

		ShaderMacroTable macros_without_uuid = macros;
		macros_without_uuid.erase("__OMNI_PIPELINE_LOCAL_HASH");

		if (m_Library.contains(shader_filename)) {
			m_Library[shader_filename].push_back({ shader, macros_without_uuid });
		}
		else {
			std::vector<std::pair<Shared<Shader>, ShaderMacroTable>> permutations;
			permutations.push_back({ shader, macros_without_uuid });
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

	Shared<Shader> ShaderLibrary::GetShader(std::string key, const ShaderMacroTable& macros)
	{
		if (!m_Library.contains(key))
			return nullptr;

		auto& permutation_list = m_Library.at(key);

		for (auto& permutation : permutation_list) {
			if (permutation.second.size() == 1 && macros.size() == 0)
				return permutation.first;

			if (permutation.second == macros)
				return permutation.first;
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
	
}