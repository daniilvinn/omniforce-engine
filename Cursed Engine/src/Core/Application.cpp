#include "Application.h"
#include <Log/Logger.h>

#include <cassert>
#include <Core/Events/ApplicationEvents.h>
#include <Threading/JobSystem.h>

#include <Core/Input/Input.h>

#include <Renderer/Renderer.h>

namespace Cursed 
{

	Application* Application::s_Instance = nullptr;

	Application::Application() 
		: m_RootSystem(nullptr), m_Running(true) {}

	Application::~Application() {}

	void Application::Launch(Options& options)
	{
		assert(s_Instance == nullptr);
		s_Instance = this;
		m_RootSystem = options.root_system;

		JobSystem::Init(JobSystem::ShutdownPolicy::WAIT);
		JobSystem* js = JobSystem::Get();

		CURSED_INITIALIZE_LOG_SYSTEM(Logger::Level::LEVEL_TRACE);

		m_WindowSystem = WindowSystem::Init();

		WindowSystem::WindowConfig main_window_config = {};
		main_window_config.event_callback = CURSED_BIND_EVENT_FUNCTION(Application::OnEvent);
		main_window_config.tag = "main";
		main_window_config.title = "Cursed Engine";
		main_window_config.width = 1600;
		main_window_config.height = 900;
		main_window_config.fs_exclusive = false;
		main_window_config.vsync = false;

		m_WindowSystem->AddWindow(main_window_config);

		RendererConfig renderer_config = {};
		renderer_config.main_window = m_WindowSystem->GetWindow("main");
		renderer_config.frames_in_flight = 3;

		js->Execute(Input::Init);
		js->Execute([&renderer_config]() {
			Renderer::Init(renderer_config);
		});

		js->Wait();
	}

	void Application::Run()
	{
		m_RootSystem->Launch();
		while (m_Running) {
			PreFrame();
			m_RootSystem->OnUpdate();
			PostFrame();
		}
		m_RootSystem->Destroy();
	}

	void Application::Destroy()
	{
		CURSED_CORE_INFO("Engine shutdown success");
	}

	void Application::OnEvent(Event* e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(CURSED_BIND_EVENT_FUNCTION(Application::OnExit));
		dispatcher.Dispatch<WindowResizeEvent>(CURSED_BIND_EVENT_FUNCTION(Application::OnMainWindowResize));

		m_RootSystem->OnEvent(e);
	}

	void Application::PreFrame()
	{
		m_WindowSystem->ProcessEvents();
	}

	void Application::PostFrame()
	{
		m_WindowSystem->PollEvents();
	}

	Shared<AppWindow> Application::GetWindow(const std::string& tag) const
	{
		return m_WindowSystem->GetWindow(tag);
	}

	bool Application::OnExit(WindowCloseEvent* e)
	{
		m_Running = false;
		CURSED_CORE_TRACE("Exiting engine");
		return true;
	}

	bool Application::OnMainWindowResize(WindowResizeEvent* e) 
	{
		vec2<int32> resolution = e->GetResolution();
		CURSED_CORE_TRACE("Window resize: {0}x{1}", resolution.x, resolution.y);
		return true;
	};
}