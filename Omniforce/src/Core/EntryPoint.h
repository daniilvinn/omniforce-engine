#pragma once

#include <exception>
#include <fstream>

#if (OMNIFORCE_PLATFORM == OMNIFORCE_PLATFORM_WIN64)
	#include <Windows.h>
#endif

extern Omni::Subsystem* ConstructRootSystem();

using namespace Omni;

bool g_EngineRunning;

#ifdef OMNIFORCE_DEBUG
int main()
#else OMNIFORCE_RELEASE
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
#endif
{
	g_EngineRunning = true;

	while (g_EngineRunning) 
	{
		Scope<Application> app = std::make_unique<Application>();
	
		Application::Options options;
		options.root_system = ConstructRootSystem();
		options.flags = 0;

		app->Launch(options);
		app->Run();
		app->Destroy();

		g_EngineRunning = false;
	}
}
