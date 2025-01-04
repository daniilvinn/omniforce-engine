#include <Foundation/Common.h>
#include <Platform/Win64/Win64WindowSystem.h>

#include <Threading/JobSystem.h>

#include <GLFW/glfw3.h>

namespace Omni {

	Win64WindowSystem::Win64WindowSystem()
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwInit();
		glfwSetErrorCallback(GLFWErrorCallback);
	}

	Win64WindowSystem::~Win64WindowSystem()
	{
		glfwTerminate();
	}

	void Win64WindowSystem::AddWindow(const WindowConfig& config)
	{
		AppWindow::Config window_config;
		window_config.event_callback = config.event_callback;
		window_config.title = config.title;
		window_config.width = config.width;
		window_config.height = config.height;
		window_config.fullscreen = config.fs_exclusive;

		Shared<AppWindow> window = AppWindow::Create(window_config);
		m_ActiveWindows.emplace(config.tag, window);
	}

	void Win64WindowSystem::RemoveWindow(const std::string& tag)
	{
		m_ActiveWindows.erase(tag);
	}

	Shared<AppWindow> Win64WindowSystem::GetWindow(const std::string& tag) const
	{
		auto found_value = m_ActiveWindows.find(tag);
		if (found_value == m_ActiveWindows.end()) [[unlikely]]
		{
			OMNIFORCE_CORE_CRITICAL("Cannot access window with key \"{0}\". Check specified key", tag);
			return nullptr;
		}
		return found_value->second;
	}

	void Win64WindowSystem::ProcessEvents()
	{
		for (auto& wnd : m_ActiveWindows) {
			wnd.second->FlushEventBuffer();
		}
	}

	void Win64WindowSystem::GLFWErrorCallback(int error, const char* description)
	{
		OMNIFORCE_CORE_CRITICAL("GLFW error ({0}): {1}", error, description);
	}

	void Win64WindowSystem::PollEvents()
	{
		glfwPollEvents();
	}

}