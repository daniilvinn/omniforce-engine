#pragma once

#include <Foundation/Common.h>
#include <Renderer/RendererCommon.h>
#include <Renderer/Shader.h>
#include <Renderer/ShaderCompiler.h>

#include <shared_mutex>

#include <robin_hood.h>

// TODO: move shader library to Scene directory
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
		using InternalStorage = rhumap<std::string, std::vector<std::pair<Ref<Shader>, ShaderMacroTable>>>;

		// @return true if loaded successfully, false if not. It can happen due to incorrect shader path or invalid shader code
		bool LoadShader(const std::filesystem::path& path, const ShaderMacroTable& macros = {});

		// @return true if shader was successfully unloaded from library and destroyed, false if no shader with such name was found
		bool UnloadShader(std::string name, const ShaderMacroTable& macros = {});

		// @return true if shader was successfully found and recompiled, false if shader not found / contains errors in its code
		bool ReloadShader(std::filesystem::path name);

		/*
		*  @brief attempts to find shader with matching name. Tries to find both with or without shader extension (e.g. .vs, .fs and so on)
		*  @return true if shader is present in library, otherwise returns false
		*/
		bool HasShader(std::string key, const ShaderMacroTable& macros = {});

		/*
		*  @brief Acquire shader from library. Read-only.
		*  @return a shared pointer to shader. If shader is not present, returns nullptr.
		*/
		Ref<Shader> GetShader(std::string key, ShaderMacroTable macros = {});

		/*
		*  @return a whole shader library. Can be used to iterate through, e.g. for using with ImGui to list all shaders
		*/
		const InternalStorage* const GetShaders() const { return &m_Library; }

		ShaderStage EvaluateStage(std::filesystem::path file) const;

	private:

		/*
		*  Loads a Slang module and extracts Vertex / Fragment stages
		*/

		ShaderCompilationResult LoadSlangShader(const std::filesystem::path& path, ShaderCompiler& compiler);

	private:
		inline static ShaderLibrary* s_Instance;
		InternalStorage m_Library;
		std::shared_mutex m_Mutex;
		std::vector<std::pair<std::string, std::string>> m_GlobalMacros;
	};

}