#pragma once

#include <Foundation/Common.h>
#include <RHI/RHICommon.h>

#include <RHI/Shader.h>

#include <map>

#include <slang.h>
#include <slang-com-ptr.h>
#include <shaderc/shaderc.hpp>

namespace Omni {

	struct OMNIFORCE_API ShaderCompilationResult {
		std::map<ShaderStage, std::vector<uint32>> bytecode; // TODO: change bytecode type to `ByteArray`
		bool valid;
	};

	struct ShaderEntryPointCode {
		ByteArray bytecode;
		ShaderStage stage;
		bool valid;
	};

	struct ShaderCompilationRequest {
		stdfs::path path;
		ShaderMacroTable macros;
	};

	class OMNIFORCE_API ShaderCompiler {
	public:
		ShaderCompiler();

		static void Init();

		/*
		*  @brief reads shader file.
		*  @return true if successful, otherwise false (e.g. file not present)
		*  @param [out] std::stringstream* out - a pointer to stringstream to write source code to.
		*/
		bool ReadShaderFile(std::filesystem::path path, std::stringstream* out);
		void AddGlobalMacro(const std::string& key, const std::string& value);

		ShaderCompilationResult Compile(std::string& source, const std::string& filename, const ShaderMacroTable& macros = {});

		bool LoadModule(const stdfs::path& path, const ShaderMacroTable& macros = {});
		ShaderEntryPointCode GetEntryPointCode(IAllocator* allocator, const std::string& entry_point_name);

	private:
		bool ValidateSlangResult(SlangResult result, Slang::ComPtr<slang::IBlob>& diagnosticsBlob);

	private:
		shaderc::Compiler m_Compiler;
		shaderc::CompileOptions m_GlobalOptions;
		
		inline static Slang::ComPtr<slang::IGlobalSession> m_GlobalSession = nullptr;
		inline static Slang::ComPtr<slang::ISession> m_LocalSession = nullptr;
		rhumap<std::string, slang::IModule*> m_LoadedModules;

		std::map<std::string, std::string> m_GlobalMacros;

	};

	inline Ptr<ShaderCompiler> g_ShaderCompiler;

}