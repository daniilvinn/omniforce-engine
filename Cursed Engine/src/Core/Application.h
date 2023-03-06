#pragma once

#include <Foundation/Macros.h>
#include <Memory/Common.h>
#include <Core/Subsystem.h>
#include <Core/Windowing/WindowSystem.h>

#include <Core/Events/ApplicationEvents.h>
#include <Core/Events/MouseEvents.h>

namespace Cursed {

	class CURSED_API Application
	{
	public:
		// Options which have to be passed into *Application.Launch()* method.
		// Specifies how to launch the application.
		enum class LaunchOptionsFlags
		{
			// Launch without windows, console-only. 
			// To be used for debugging purpose or solving networking issues (not likely to be implemented at all)
			HEADLESS = BIT(0),
		};

		struct Options 
		{
			Subsystem* root_system;
			BitMask flags;
		};

		Application();
		~Application();

		static Application* Get() { return s_Instance; }

		void Launch(Options& options);
		void Run();
		void Destroy();
		void OnEvent(Event* e);

		void PreFrame();
		void PostFrame();

		Shared<AppWindow> GetWindow(const std::string& tag) const;

	private:
		bool OnExit(WindowCloseEvent* e);
		bool OnMainWindowResize(WindowResizeEvent* e);

	private:
		static Application* s_Instance;
		bool m_Running;

		Subsystem* m_RootSystem;
		WindowSystem* m_WindowSystem;
	};

}