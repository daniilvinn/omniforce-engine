#pragma once

#include "MonoForwardDecl.h"

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Scene/Entity.h>
#include <Memory/Allocator.h>
#include <Core/Timer.h>
#include "ScriptClass.h"
#include "PendingCallbackInfo.h"

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
		bool HasAssemblies() const { return mAssembliesLoaded; }
		MonoImage* GetCoreImage();
		MonoImage* GetAppImage();
		MonoDomain* GetDomain() { return mAppDomain; }
		ScriptClass GetBaseObject() const { return mScriptBase; }
		TransientAllocator<false>& GetCallbackArgsAllocator() { return m_CallbackArgsAllocator; }

		void LoadAssemblies();
		void UnloadAssemblies();
		void ReloadAssemblies();

		inline void AddPendingCallback(PendingCallbackInfo info) { m_PendingCallbacks.push_back(info); }
		void OnUpdate();
		void DispatchExceptionHandling(MonoObject* exception);

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
		robin_hood::unordered_map<std::string, ScriptClass> mAvailableClassesList;

		MonoMethod* mInitMethodBase;
		MonoMethod* mUpdateMethodBase;
		MonoMethod* mBaseCtor;
		robin_hood::unordered_map<UUID, MonoMethod*> mOnInitMethods;
		robin_hood::unordered_map<UUID, MonoMethod*> mOnUpdateMethods;

		TransientAllocator<false> m_CallbackArgsAllocator;
		std::vector<PendingCallbackInfo> m_PendingCallbacks;
		Timer m_GCTimer;

		friend class ScriptClass;
		friend class RuntimeScriptInstance;

		
	};

}