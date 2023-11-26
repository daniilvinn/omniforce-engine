#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Pointers.hpp>
#include <Core/Events/Event.h>

#include <string>
#include <functional>

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
		static Shared<AppWindow> Create(const Config& config);

		virtual void* Raw() const = 0;

		template<typename T, typename... Args>
		T* AllocateEvent(Args&&... args)
		{
			auto e = m_Allocator.Allocate<T>(std::forward<Args>(args)...);
			m_EventBuffer.push_back(e);
			return e;
		}

		void FlushEventBuffer() { 
			for (auto& e : m_EventBuffer)
				m_EventCallback(e);
			m_Allocator.Free(); 
			m_EventBuffer.clear();
		}

		bool Minimized() const { return m_Minimized; }
		void SetMinimized(bool minimized) { m_Minimized = minimized; }

	protected:
		AppWindow() : m_Allocator(32768) { m_EventBuffer.reserve(2048); };
		AppWindow(const AppWindow&) = delete;

		EventCallback m_EventCallback;
		std::vector<Event*> m_EventBuffer;
		EventAlloc m_Allocator;
		std::shared_mutex m_EventMutex;
		bool m_Minimized = false;
	};

}