#pragma once

#include <exception>
#include <fstream>

#if (OMNIFORCE_PLATFORM == OMNIFORCE_PLATFORM_WIN64)
	#include <Windows.h>
#endif

extern Omni::Subsystem* ConstructRootSystem();

using namespace Omni;

bool GEngineRunning;

#ifdef OMNIFORCE_RELEASE
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
#else
int main()
#endif
{
	GEngineRunning = true;

	OMNIFORCE_INITIALIZE_LOG_SYSTEM(Logger::Level::LEVEL_TRACE);
	OMNIFORCE_CORE_INFO("Entering `main` function");

	while (GEngineRunning) 
	{
		Scope<Application> app = std::make_unique<Application>();
	
		Application::Options options;
		options.root_system = ConstructRootSystem();
		options.flags = 0;

		app->Launch(options);
		app->Run();
		app->Destroy();

		GEngineRunning = false;
	}
}
