#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Scene/Entity.h>
#include "ScriptClass.h"

#include <filesystem>
#include <robin_hood.h>

#include "MonoForwardDecl.h"

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
		bool HasAssemblies() const { return mAssembliesLoaded; }
		MonoImage* GetCoreImage();
		MonoImage* GetAppImage();
		MonoDomain* GetDomain() { return mAppDomain; }
		ScriptClass GetBaseObject() const { return mScriptBase; }

		void LoadAssemblies();
		void UnloadAssemblies();
		void ReloadAssemblies();

	private:
		ScriptEngine();
		~ScriptEngine();

		void InitMonoInternals();		

	private:
		MonoAssembly* ReadAssembly(std::filesystem::path path);
		void ResetData();

	private:
		inline static ScriptEngine* s_Instance = nullptr;
		Scene* m_Context = nullptr;
		bool mAssembliesLoaded = false;

		MonoDomain* mRootDomain = nullptr;
		MonoDomain* mAppDomain = nullptr;
		MonoAssembly* mCoreAssembly = nullptr;
		MonoAssembly* mAppAssembly = nullptr;
		ScriptClass mScriptBase;

		MonoMethod* mInitMethodBase;
		MonoMethod* mUpdateMethodBase;
		MonoMethod* mBaseCtor;
		robin_hood::unordered_map<std::string, ScriptClass> mAvailableClassesList;
		robin_hood::unordered_map<UUID, MonoMethod*> mOnInitMethods;
		robin_hood::unordered_map<UUID, MonoMethod*> mOnUpdateMethods;

		friend class ScriptClass;
		friend class RuntimeScriptInstance;
	};

}