#pragma once

#include <Core/Windowing/AppWindow.h>
#include <Core/Events/Event.h>
#include <GLFW/glfw3.h>

namespace Cursed {

	class Win64AppWindow final : public AppWindow {
	public:
		Win64AppWindow(const AppWindow::Config& config);
		~Win64AppWindow() override;

		void* Raw() const override { return (void*)m_WindowHandle; }

		void FlushEventBuffer() { m_Allocator.Free(); }

		std::vector<Event*>& GetEventBuffer() { return m_EventBuffer; }

	private:
		GLFWwindow* m_WindowHandle = nullptr;
	};

}