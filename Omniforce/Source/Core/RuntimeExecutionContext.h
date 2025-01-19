#pragma once

#include <Foundation/Common.h>

#include <Core/ObjectLifetimeManager.h>

namespace Omni {

	class RuntimeExecutionContext {
	public:
		static void Init() {
			s_Instance = new RuntimeExecutionContext;
			OMNIFORCE_CORE_INFO("Initialized execution context");
		}

		static void Shutdown() {
			delete s_Instance;
			OMNIFORCE_CORE_INFO("Destroyed execution context");
		}

		inline static RuntimeExecutionContext& Get() {
			return *s_Instance;
		}

		inline ObjectLifetimeManager& GetObjectLifetimeManager() { 
			return m_ObjectLifetimeManager; 
		}

		void Update() {
			m_ObjectLifetimeManager.Update();
		}

	private:
		inline static RuntimeExecutionContext* s_Instance = nullptr;

		ObjectLifetimeManager m_ObjectLifetimeManager;
	};

}