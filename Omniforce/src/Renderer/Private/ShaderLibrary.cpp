#include "../ShaderLibrary.h"

#include <Log/Logger.h>
#include <Filesystem/Filesystem.h>
#include <Threading/JobSystem.h>
#include "../ShaderCompiler.h"

#include <fstream>

namespace Omni {

	Omni::ShaderLibrary* ShaderLibrary::s_Instance;

	Omni::ShaderCompiler m_Compiler;

	ShaderLibrary::ShaderLibrary()
	{

	}

	ShaderLibrary::~ShaderLibrary()
	{
		for (auto& [key, shader] : m_Library)
			shader->Destroy();
	}

	void ShaderLibrary::Init()
	{
		s_Instance = new ShaderLibrary;
		m_Compiler.AddGlobalMacro("_OMNI_SCENE_DESCRIPTOR_SET", "0");
		m_Compiler.AddGlobalMacro("_OMNI_PASS_DESCRIPTOR_SET", "1");
		m_Compiler.AddGlobalMacro("_OMNI_DRAW_CALL_DESCRIPTOR_SET", "2");
	}

	void ShaderLibrary::Destroy()
	{
		delete s_Instance;
	}

	bool ShaderLibrary::Load(std::filesystem::path path)
	{
		bool result = m_Library.find(path.filename().string()) != m_Library.end();

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

		ShaderCompilationResult compilation_result = m_Compiler.Compile(shader_source, path.filename().string());

		if (!compilation_result.valid) return false;

		Shared<Shader> shader = Shader::Create(compilation_result.bytecode, path);

		m_Mutex.lock();
		m_Library.emplace(path.filename().string(), shader);
		m_Mutex.unlock();
	}

	bool ShaderLibrary::Unload(std::string name)
	{
		if (m_Library.find(name) == m_Library.end())
			return false;

		m_Library.find(name)->second->Destroy();
		m_Library.erase(name);

		return true;
	}

	bool ShaderLibrary::Reload(std::filesystem::path name)
	{
		return false;
	}

	bool ShaderLibrary::Has(std::string key)
	{
		std::scoped_lock<std::shared_mutex> lock(m_Mutex);
		return m_Library.find(key) != m_Library.end();
	}

	Shared<Shader> ShaderLibrary::GetShader(std::string key)
	{
		return m_Library.find(key)->second;
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