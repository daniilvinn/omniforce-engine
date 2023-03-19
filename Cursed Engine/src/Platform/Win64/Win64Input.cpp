#include "Win64Input.h"

#include <Core/Application.h>

#include <GLFW/glfw3.h>
#include <assert.h>

namespace Cursed {

	bool Win64Input::Impl_KeyPressed(KeyCode code, const std::string& window_tag) const
	{
		int32 status = glfwGetKey((GLFWwindow*)Application::Get()->GetWindow(window_tag)->Raw(), (int32)code);
		return status == GLFW_PRESS || status == GLFW_REPEAT ? true : false;
	}

	bool Win64Input::Impl_ButtonPressed(ButtonCode code, const std::string& window_tag) const
	{
		int32 status = glfwGetMouseButton((GLFWwindow*)Application::Get()->GetWindow(window_tag)->Raw(), (int32)code);
		return status == GLFW_PRESS ? true : false;
	}

	float32 Win64Input::Impl_MouseScrolledX(const std::string& window_tag) const
	{
		// Method not implemented.
		assert(false);
		return 0.0f;
	}

	float32 Win64Input::Impl_MouseScrolledY(const std::string& window_tag) const
	{
		// Method not implemented.
		assert(false);
		return 0.0f;
	}

	vec2<int32> Win64Input::Impl_MousePosition(const std::string& window_tag) const
	{
		vec2<float64> pos;
		glfwGetCursorPos((GLFWwindow*)Application::Get()->GetWindow(window_tag)->Raw(), &pos.x, &pos.y);
		return { (int32)pos.x, (int32)pos.y };
	}

}