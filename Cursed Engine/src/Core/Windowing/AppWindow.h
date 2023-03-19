#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Pointers.hpp>
#include <Core/Events/Event.h>

#include <string>
#include <functional>

namespace Cursed {

	class CURSED_API AppWindow {
	public:
		struct Config {
			EventCallback event_callback;
			std::string title;
			uint16 width;
			uint16 height;
			bool fullscreen;
			bool vsync;
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

	protected:
		AppWindow() : m_Allocator(8192) { m_EventBuffer.reserve(512); };
		AppWindow(const AppWindow&) = delete;

		EventCallback m_EventCallback;
		std::vector<Event*> m_EventBuffer;
		EventAlloc m_Allocator;
		std::shared_mutex m_EventMutex;
	};

}