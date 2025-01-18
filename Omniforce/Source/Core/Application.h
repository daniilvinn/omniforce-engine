#pragma once

#include <Foundation/Common.h>
#include <Core/Subsystem.h>
#include <Core/Windowing/WindowSystem.h>
#include <Core/Events/ApplicationEvents.h>
#include <Core/Events/MouseEvents.h>
#include <Renderer/UI/ImGuiRenderer.h>

namespace Omni {

	class OMNIFORCE_API Application
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
			Ptr<Subsystem> root_system;
			BitMask flags;
		};

		Application();
		~Application();

		static Application* Get() { return s_Instance; }
		float GetDeltaTime() const { return m_DeltaTimeData.delta_time; }

		void Launch(Options& options);
		void Run();
		void Destroy();
		void OnEvent(Event* e);

		void PreFrame();
		void PostFrame();

		Ref<AppWindow> GetWindow(const std::string& tag) const;

	private:
		bool OnExit(WindowCloseEvent* e);
		bool OnMainWindowResize(WindowResizeEvent* e);

	private:
		static Application* s_Instance;
		bool m_Running;

		Ptr<Subsystem> m_RootSystem;
		Ptr<WindowSystem> m_WindowSystem;
		ImGuiRenderer* m_ImGuiRenderer;

		struct DeltaTimeData {
			float32 delta_time = 1.0f / 60.0f;
			float32 last_frame_time = 0.0f;
			float32 current_frame_time = 60.0f;
		} m_DeltaTimeData;
	};

}