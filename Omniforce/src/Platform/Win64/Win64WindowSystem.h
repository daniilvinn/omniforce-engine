#pragma once

#include <Core/Windowing/WindowSystem.h>
#include <Windows.h>

namespace Omni {

	class Win64WindowSystem : public WindowSystem {
	public:
		Win64WindowSystem();
		~Win64WindowSystem();

		void PollEvents() override;

		void AddWindow(const WindowConfig& config) override;

		void RemoveWindow(const std::string& tag) override;

		Shared<AppWindow> GetWindow(const std::string& tag) const override;

		void ProcessEvents() override;

		static void GLFWErrorCallback(int error, const char* description);

	};

}
