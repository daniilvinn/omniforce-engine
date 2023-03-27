#include "../ShaderLibrary.h"

#include <Log/Logger.h>
#include <Filesystem/Filesystem.h>
#include <Threading/JobSystem.h>

namespace Omni {

	Omni::ShaderLibrary* ShaderLibrary::s_Instance;

	Omni::ShaderCompiler thread_local ShaderLibrary::m_Compiler;

	void ShaderLibrary::Init()
	{
		s_Instance = new ShaderLibrary;
	}

	void ShaderLibrary::Destroy()
	{
		delete s_Instance;
	}

	bool ShaderLibrary::Load(std::filesystem::path path)
	{
		std::stringstream sstream;
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

		result = m_Compiler.ReadShaderFile(path, &sstream);

		if (!result)
		{
			OMNIFORCE_CORE_ERROR("Failed to read shared file: {0}", path.string());
			return false;
		}
		
		std::vector<uint32> binary_data;

		if (m_Compiler.Compile(sstream, this->EvaluateStage(path), path.filename().string(), &binary_data)) {
			OMNIFORCE_CORE_INFO("Shader \"{0}\" loaded succesfully", path.filename().string());
		};

		Shared<Shader> shader = Shader::Create(this->EvaluateStage(path), binary_data, path);

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

	Shared<Shader> ShaderLibrary::Get(std::string key)
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