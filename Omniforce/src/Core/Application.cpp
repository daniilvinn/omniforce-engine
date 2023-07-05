#include "Application.h"

#include <Log/Logger.h>
#include <Core/Events/ApplicationEvents.h>
#include <Threading/JobSystem.h>
#include <Core/Input/Input.h>
#include <Renderer/Renderer.h>
#include <Renderer/ShaderLibrary.h>
#include <Asset/AssetManager.h>
#include <Physics/PhysicsEngine.h>
#include <Core/Input/Input.h>

#include <cassert>
#include <chrono>

namespace Omni 
{

	Application* Application::s_Instance = nullptr;

	Application::Application() 
		: m_RootSystem(nullptr), m_Running(true), m_WindowSystem(nullptr), m_ImGuiRenderer(nullptr) {}

	Application::~Application() {}

	void Application::Launch(Options& options)
	{
		assert(s_Instance == nullptr);
		s_Instance = this;
		m_RootSystem = options.root_system;

		JobSystem::Init(JobSystemShutdownPolicy::WAIT);
		JobSystem* js = JobSystem::Get();

		OMNIFORCE_INITIALIZE_LOG_SYSTEM(Logger::Level::LEVEL_TRACE);

		m_WindowSystem = WindowSystem::Init();

		WindowSystem::WindowConfig main_window_config = {};
		main_window_config.event_callback = OMNIFORCE_BIND_EVENT_FUNCTION(Application::OnEvent);
		main_window_config.tag = "main";
		main_window_config.title = "Omniforce Game Engine";
		main_window_config.width = 1600;
		main_window_config.height = 900;
		main_window_config.fs_exclusive = false;

		m_WindowSystem->AddWindow(main_window_config);

		RendererConfig renderer_config = {};
		renderer_config.main_window = m_WindowSystem->GetWindow("main").get();
		renderer_config.frames_in_flight = 2;
		renderer_config.vsync = false;

		js->Execute(Input::Init);
		js->Execute([renderer_config]() {
			Renderer::Init(renderer_config);
		});
		js->Execute(PhysicsEngine::Init);

		js->Wait();

		js->Execute(Renderer::LoadShaderPack);
		js->Execute(AssetManager::Init);

		m_ImGuiRenderer = ImGuiRenderer::Create();
		m_ImGuiRenderer->Launch(m_WindowSystem->GetWindow("main")->Raw());
	}

	void Application::Run()
	{
		m_RootSystem->Launch();
		while (m_Running) {
			PreFrame();
			m_RootSystem->OnUpdate(m_DeltaTimeData.delta_time);
			PostFrame();
		}
		m_RootSystem->Destroy();
	}

	void Application::Destroy()
	{
		m_ImGuiRenderer->Destroy();
		AssetManager::Shutdown();
		Renderer::Shutdown();
		
		OMNIFORCE_CORE_INFO("Engine shutdown success");
	}

	void Application::OnEvent(Event* e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(Application::OnExit));
		dispatcher.Dispatch<WindowResizeEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(Application::OnMainWindowResize));

		m_RootSystem->OnEvent(e);
	}

	void Application::PreFrame()
	{
		m_DeltaTimeData.delta_time = (m_DeltaTimeData.current_frame_time - m_DeltaTimeData.last_frame_time);
		m_DeltaTimeData.last_frame_time = m_DeltaTimeData.current_frame_time;

		m_WindowSystem->PollEvents();
		Renderer::BeginFrame();
		m_ImGuiRenderer->BeginFrame();
	}

	void Application::PostFrame()
	{
		m_ImGuiRenderer->EndFrame();
		Renderer::Render();
		Renderer::EndFrame();
		m_WindowSystem->ProcessEvents();

		m_DeltaTimeData.current_frame_time = Input::Time();
	}

	Shared<AppWindow> Application::GetWindow(const std::string& tag) const
	{
		return m_WindowSystem->GetWindow(tag);
	}

	bool Application::OnExit(WindowCloseEvent* e)
	{
		m_Running = false;
		OMNIFORCE_CORE_TRACE("Exiting engine");
		return true;
	}

	bool Application::OnMainWindowResize(WindowResizeEvent* e) 
	{
		return true;
	};
}