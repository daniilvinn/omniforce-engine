#pragma once

#include "RendererCommon.h"
#include "Shader.h"
#include <robin_hood.h>
#include <shared_mutex>

namespace Omni {


	/*
	*	@brief provides tools to store all shaders in centralized manner. Also gives toolset to load / unload shader if necessary.
	*/
	class OMNIFORCE_API ShaderLibrary {
	public:
		ShaderLibrary();
		~ShaderLibrary();

		static void Init();
		static void Destroy();
		static ShaderLibrary* Get() { return s_Instance; };

		// @return true if loaded successfully, false if not. It can happen due to incorrect shader path or invalid shader code
		bool Load(std::filesystem::path path);

		// @return true if shader was successfully unloaded from library and destroyed, false if no shader with such name was found
		bool Unload(std::string name);

		// @return true if shader was successfully found and recompiled, false if shader not found / contains errors in its code
		bool Reload(std::filesystem::path name);

		/*
		*  @brief tries to find shader with matching name. Tries to find both with or without shader extension (e.g. .vs, .fs and so on)
		*  @return true if shader is present in library, otherwise returns false
		*/
		bool Has(std::string key);

		/*
		*  @brief Acquire shader from library. Read-only.
		*  @return a shared pointer to shader. If shader is not present, returns nullptr.
		*/
		Shared<Shader> GetShader(std::string key);

		/*
		*  @return a whole shader library. Can be used to iterate through, e.g. for using with ImGui to list all shaders
		*/
		const robin_hood::unordered_map<std::string, Shared<Shader>>* const GetShaders() const { return &m_Library; }

		ShaderStage EvaluateStage(std::filesystem::path file) const;

	private:
		static ShaderLibrary* s_Instance;
		robin_hood::unordered_map<std::string, Shared<Shader>> m_Library;
		std::shared_mutex m_Mutex;
	};

}