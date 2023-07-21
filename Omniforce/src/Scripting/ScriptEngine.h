#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <filesystem>

#include <robin_hood.h>

namespace Omni {

	class Scene;

	class OMNIFORCE_API ScriptEngine {
	public:
		static void Init();
		static void Shutdown();
		static ScriptEngine* Get() { return s_Instance; }

		void LoadAssemblies();
		void UnloadAssemblies();
		void ReloadAssemblies();

		bool HasAssemblies() const;

		void LaunchRuntime(Scene* context);
		void ShutdownRuntime();

	private:
		ScriptEngine();
		~ScriptEngine();

		void InitMonoInternals();		

	private:
		void* ReadAssembly(std::filesystem::path path);

	private:
		inline static ScriptEngine* s_Instance = nullptr;
		Scene* m_Context;

	};

}