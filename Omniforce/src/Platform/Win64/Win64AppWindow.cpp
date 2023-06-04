#include "Win64AppWindow.h"
#include <Log/Logger.h>

#include <Core/Events/ApplicationEvents.h>
#include <Core/Events/KeyEvents.h>
#include <Core/Events/MouseEvents.h>

namespace Omni {

	Win64AppWindow::Win64AppWindow(const AppWindow::Config& config)
	{
		m_EventCallback = config.event_callback;
		m_Minimized = false;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // for Vulkan

		m_WindowHandle = glfwCreateWindow(
			config.width,
			config.height,
			config.title.c_str(),
			config.fullscreen ? glfwGetPrimaryMonitor() : nullptr,
			nullptr
		);

		glfwSetWindowUserPointer(m_WindowHandle, this);

		glfwSetWindowSizeCallback(m_WindowHandle, [](GLFWwindow* window, int width, int height) {
			vec2<int32> resolution = { width, height };

			auto impl_window = (Win64AppWindow*)glfwGetWindowUserPointer(window);
			impl_window->AllocateEvent<WindowResizeEvent>(resolution);

			if (resolution.x || resolution.y)
				impl_window->SetMinimized(true);
			else
				impl_window->SetMinimized(false);
		});

		glfwSetKeyCallback(m_WindowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			KeyCode code = static_cast<KeyCode>(key);

			auto impl_window = (Win64AppWindow*)glfwGetWindowUserPointer(window);
	
			switch (action)
			{
			case GLFW_PRESS:
				impl_window->AllocateEvent<KeyPressedEvent>(code, 0);
				break;
			case GLFW_REPEAT:
				impl_window->AllocateEvent<KeyPressedEvent>(code, 1);
				break;
			case GLFW_RELEASE:
				impl_window->AllocateEvent<KeyReleasedEvent>(code);
				break;
			default:
				break;
			}
		});

		glfwSetCursorPosCallback(m_WindowHandle, [](GLFWwindow* window, double xpos, double ypos) {
			vec2<float32> pos = { xpos, ypos };
			
			auto impl_window = (Win64AppWindow*)glfwGetWindowUserPointer(window);
			impl_window->AllocateEvent<MouseMovedEvent>(pos);
		});

		glfwSetWindowCloseCallback(m_WindowHandle, [](GLFWwindow* window) {
			auto impl_window = (Win64AppWindow*)glfwGetWindowUserPointer(window);
			impl_window->AllocateEvent<WindowCloseEvent>();
		});

		glfwSetMouseButtonCallback(m_WindowHandle, [](GLFWwindow* window, int button, int action, int mods) {
			ButtonCode code = static_cast<ButtonCode>(button);

			auto impl_window = (Win64AppWindow*)glfwGetWindowUserPointer(window);
			switch (action)
			{
			case GLFW_PRESS:
				impl_window->AllocateEvent<MouseButtonPressedEvent>(code);
				break;
			case GLFW_REPEAT:
				impl_window->AllocateEvent<MouseButtonReleasedEvent>(code);
				break;
			default:
				break;
			}
		});

		glfwSetScrollCallback(m_WindowHandle, [](GLFWwindow* window, double xoffset, double yoffset) {
			vec2<float32> offset = { xoffset, yoffset };
			
			auto impl_window = (Win64AppWindow*)glfwGetWindowUserPointer(window);
			impl_window->AllocateEvent<MouseScrolledEvent>(offset);
		});

		glfwSetCharCallback(m_WindowHandle, [](GLFWwindow* window, unsigned int character) {
			KeyCode code = static_cast<KeyCode>(character);

			auto impl_window = (Win64AppWindow*)glfwGetWindowUserPointer(window);
			impl_window->AllocateEvent<KeyTypedEvent>(code);

		});
	}

	Win64AppWindow::~Win64AppWindow()
	{
		glfwDestroyWindow(m_WindowHandle);
	}

}