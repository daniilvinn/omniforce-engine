#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Scene/Entity.h>

#include <filesystem>

#include <robin_hood.h>

namespace Omni {

	class Scene;

	class OMNIFORCE_API ScriptEngine {
	public:
		static void Init();
		static void Shutdown();
		static ScriptEngine* Get() { return s_Instance; }

		void LaunchRuntime(Scene* context);
		void ShutdownRuntime();
		Scene* GetContext() const { return m_Context; }
		bool HasContext() { return m_Context != nullptr; }

		void LoadAssemblies();
		void UnloadAssemblies();
		void ReloadAssemblies();
		bool HasAssemblies() const;

		void CallOnInit(Entity e);
		void CallOnUpdate(Entity e);

	private:
		ScriptEngine();
		~ScriptEngine();

		void InitMonoInternals();		

	private:
		void* ReadAssembly(std::filesystem::path path);

	private:
		inline static ScriptEngine* s_Instance = nullptr;
		Scene* m_Context = nullptr;

	};

}