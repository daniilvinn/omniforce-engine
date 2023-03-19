#pragma once

#include <Core/Windowing/AppWindow.h>
#include <Core/Events/Event.h>
#include <unordered_map>

namespace Cursed {

	class WindowSystem {
	public:
		struct WindowConfig {
			EventCallback event_callback;
			std::string tag;
			std::string title;
			uint16 width;
			uint16 height;
			bool fs_exclusive;
			bool vsync;
		};

		static WindowSystem* Init();

		void Shutdown() { delete s_Instance; };
		static WindowSystem* Get() { return s_Instance; }

		virtual void AddWindow(const WindowConfig& config) = 0;
		virtual void RemoveWindow(const std::string& tag) = 0;

		virtual Shared<AppWindow> GetWindow(const std::string& tag) const = 0;

		virtual void PollEvents() = 0;
		virtual void ProcessEvents() = 0;

	protected:
		static WindowSystem* s_Instance;
		std::unordered_map<std::string, Shared<AppWindow>> m_ActiveWindows;
	};

}