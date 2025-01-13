#pragma once

#include <Foundation/Common.h>
#include <Core/Events/Event.h>

#include <shared_mutex>

namespace Omni {

	class OMNIFORCE_API AppWindow {
	public:

		struct Config {
			EventCallback event_callback = nullptr;
			std::string title = "Blank";
			uint16 width = 1600;
			uint16 height = 900;
			bool fullscreen = false;
		};

		virtual ~AppWindow() {};
		static Ref<AppWindow> Create(IAllocator* allocator, const Config& config);

		virtual void* Raw() const = 0;

		template<typename T, typename... Args>
		T* AllocateEvent(Args&&... args)
		{
			MemoryAllocation e = m_EventAllocator->Allocate<T>(std::forward<Args>(args)...);
			m_EventBuffer.Add(e.As<T>());
			return e.As<T>();
		}

		void FlushEventBuffer() { 
			for (const auto& e : m_EventBuffer)
				m_EventCallback(e);
			m_EventBuffer.Clear();
			m_EventAllocator->Clear();
		}

		bool Minimized() const { return m_Minimized; }
		void SetMinimized(bool minimized) { m_Minimized = minimized; }

	protected:
		AppWindow() 
			: m_EventAllocator(&g_TransientAllocator)
			, m_EventBuffer(&g_PersistentAllocator)
		{ 
			m_EventBuffer.Reallocate(8096); 
		};
		AppWindow(const AppWindow&) = delete;

		TransientAllocator<true>* m_EventAllocator;
		EventCallback m_EventCallback;
		Array<Event*> m_EventBuffer;
		std::shared_mutex m_EventMutex;
		bool m_Minimized = false;
	};

}