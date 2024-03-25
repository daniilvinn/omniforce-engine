#include "Application.h"

#include <Core/Events/ApplicationEvents.h>
#include <Core/Input/Input.h>
#include <Core/Input/Input.h>
#include <Log/Logger.h>
#include <Renderer/Renderer.h>
#include <Renderer/ShaderLibrary.h>
#include <Asset/AssetManager.h>
#include <Physics/PhysicsEngine.h>
#include <Scripting/ScriptEngine.h>
#include <Audio/AudioEngine.h>
#include <Threading/JobSystem.h>
#include <DebugUtils/DebugRenderer.h>

#include <chrono>

namespace Omni 
{

	Application* Application::s_Instance = nullptr;

	Application::Application() 
		: m_RootSystem(nullptr), m_Running(true), m_WindowSystem(nullptr), m_ImGuiRenderer(nullptr) {}

	Application::~Application() {}

	void Application::Launch(Options& options)
	{
		s_Instance = this;
		m_RootSystem = options.root_system;

		OMNIFORCE_INITIALIZE_LOG_SYSTEM(Logger::Level::LEVEL_TRACE);

		m_WindowSystem = WindowSystem::Init();

		WindowSystem::WindowConfig main_window_config = {};
		main_window_config.event_callback = OMNIFORCE_BIND_EVENT_FUNCTION(Application::OnEvent);
		main_window_config.tag = "main";
		main_window_config.title = "Omniforce Game Engine";
		main_window_config.width = 1920;
		main_window_config.height = 1080;
		main_window_config.fs_exclusive = false;

		m_WindowSystem->AddWindow(main_window_config);

		// Init on main thread
		ScriptEngine::Init();

		RendererConfig renderer_config = {};
		renderer_config.main_window = m_WindowSystem->GetWindow("main").get();
		renderer_config.frames_in_flight = 2;
		renderer_config.vsync = false;

		auto task_executor = JobSystem::GetExecutor();

		tf::Taskflow taskflow;
		auto [renderer_task, input_task, physics_task, audio_task] = taskflow.emplace(
			[renderer_config]() { Renderer::Init(renderer_config); }, 
			Input::Init, 
			PhysicsEngine::Init, 
			AudioEngine::Init
		);

		auto asset_manager_task = taskflow.emplace(AssetManager::Init);

		asset_manager_task.succeed(renderer_task);

		task_executor->run(taskflow).wait();

		m_ImGuiRenderer = ImGuiRenderer::Create();
		m_ImGuiRenderer->Launch(m_WindowSystem->GetWindow("main")->Raw());
		DebugRenderer::Init();
	}

	void Application::Run()
	{
		m_RootSystem->Launch();
		while (m_Running) {
			PreFrame();
			if (!m_WindowSystem->GetWindow("main")->Minimized())
				m_RootSystem->OnUpdate(m_DeltaTimeData.delta_time);
			PostFrame();
		}
		m_RootSystem->Destroy();
	}

	void Application::Destroy()
	{
		m_ImGuiRenderer->Destroy();
		AssetManager::Shutdown();
		DebugRenderer::Shutdown();
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
		if (!m_WindowSystem->GetWindow("main")->Minimized()) {
			m_DeltaTimeData.delta_time = (m_DeltaTimeData.current_frame_time - m_DeltaTimeData.last_frame_time);
			m_DeltaTimeData.last_frame_time = m_DeltaTimeData.current_frame_time;

			Renderer::BeginFrame();

			m_ImGuiRenderer->BeginFrame();
		}
	}

	void Application::PostFrame()
	{
		if (!m_WindowSystem->GetWindow("main")->Minimized()) {
			m_ImGuiRenderer->EndFrame();
			Renderer::Render();
			Renderer::EndFrame();
		}

		m_WindowSystem->PollEvents();
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